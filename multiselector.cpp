// SPDX-License-Identifier: LGPL-2.1-or-later
// Grid candidate expansion for fcitx5.
//
// While the regular one-line candidate list is shown, Down expands it into a
// kRows x kCols grid (like Sogou / WeType). Arrow keys move the cursor,
// Space/Enter commits, 1..kCols commits a column in the current row,
// PageUp/PageDown (or -/= and [/]) flip grid pages, Esc or Up on the first row
// collapses back. Any other key collapses and is passed to the input method.
//
// The grid is only a view: the input method's own candidate list is kept
// alive and is what actually commits, so the engine's behaviour is unchanged.

#include <fcitx-utils/i18n.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/textformatflags.h>
#include <fcitx-utils/trackableobject.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonmanager.h>
#include <fcitx/candidatelist.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/text.h>
#include <fcitx/userinterface.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace fcitx;

constexpr int kRows = 4;
constexpr int kCols = 8;
constexpr int kPage = kRows * kCols;
// Cells are padded to the widest word on the page, measured in half-width
// units (CJK = 2, Latin = 1), so columns line up; capped at this many units.
constexpr int kMaxCellWidth = 12;
const char *const kFullWidthSpace = "　";
// Roughly half of a CJK character, used for an odd half-width unit.
const char *const kHalfWidthSpace = " ";

bool isWide(uint32_t c) {
    return (c >= 0x1100 && c <= 0x115F) || (c >= 0x2E80 && c <= 0xA4CF) ||
           (c >= 0xAC00 && c <= 0xD7A3) || (c >= 0xF900 && c <= 0xFAFF) ||
           (c >= 0xFE30 && c <= 0xFE4F) || (c >= 0xFF00 && c <= 0xFF60) ||
           (c >= 0xFFE0 && c <= 0xFFE6) || (c >= 0x1F300 && c <= 0x1FAFF) ||
           (c >= 0x20000 && c <= 0x3FFFD);
}

int displayWidth(const std::string &word) {
    if (!utf8::validate(word)) {
        return static_cast<int>(word.size());
    }
    int width = 0;
    for (auto c : utf8::MakeUTF8CharRange(word)) {
        width += isWide(c) ? 2 : 1;
    }
    return width;
}

// Re-exposes the engine's original list after the grid closes. It borrows
// every interface pointer of the original, so paging, cursor movement and
// candidate actions keep working on the real list.
class ProxyCandidateList : public CandidateList {
public:
    explicit ProxyCandidateList(std::shared_ptr<CandidateList> orig)
        : orig_(std::move(orig)) {
        setPageable(orig_->toPageable());
        setBulk(orig_->toBulk());
        setModifiable(orig_->toModifiable());
        setCursorMovable(orig_->toCursorMovable());
        setCursorModifiable(orig_->toCursorModifiable());
        setBulkCursor(orig_->toBulkCursor());
        setActionable(orig_->toActionable());
#ifdef MULTISELECTOR_HAS_TABBED
        setTabbed(orig_->toTabbed());
#endif
    }

    const Text &label(int idx) const override { return orig_->label(idx); }
    const CandidateWord &candidate(int idx) const override {
        return orig_->candidate(idx);
    }
    int size() const override { return orig_->size(); }
    int cursorIndex() const override { return orig_->cursorIndex(); }
    CandidateLayoutHint layoutHint() const override {
        return orig_->layoutHint();
    }

private:
    std::shared_ptr<CandidateList> orig_;
};

// One row of the grid per entry, laid out vertically; the selected cell is
// marked with a HighLight text segment instead of a whole-row highlight.
class GridCandidateList : public CandidateList {
public:
    explicit GridCandidateList(std::vector<Text> rows) {
        for (auto &row : rows) {
            words_.push_back(
                std::make_unique<DisplayOnlyCandidateWord>(std::move(row)));
        }
    }

    const Text &label(int /*idx*/) const override { return empty_; }
    const CandidateWord &candidate(int idx) const override {
        return *words_.at(idx);
    }
    int size() const override { return static_cast<int>(words_.size()); }
    int cursorIndex() const override { return -1; }
    CandidateLayoutHint layoutHint() const override {
        return CandidateLayoutHint::Vertical;
    }

private:
    Text empty_;
    std::vector<std::unique_ptr<CandidateWord>> words_;
};

class MultiSelector : public AddonInstance {
public:
    explicit MultiSelector(Instance *instance) : instance_(instance) {
        handlers_.emplace_back(
            instance_->watchEvent(EventType::InputContextKeyEvent,
                                  EventWatcherPhase::PreInputMethod,
                                  [this](Event &event) {
                                      onKey(static_cast<KeyEvent &>(event));
                                  }));
        for (auto type : {EventType::InputContextFocusOut,
                          EventType::InputContextReset,
                          EventType::InputContextSwitchInputMethod}) {
            handlers_.emplace_back(instance_->watchEvent(
                type, EventWatcherPhase::Default, [this](Event &event) {
                    auto &icEvent = static_cast<InputContextEvent &>(event);
                    if (icEvent.inputContext() == ic_.get()) {
                        clear();
                    }
                }));
        }
    }

private:
    bool active(InputContext *ic) const {
        return ic_.isValid() && ic_.get() == ic && grid_ &&
               ic->inputPanel().candidateList().get() == grid_;
    }

    void clear() {
        ic_.unwatch();
        orig_.reset();
        grid_ = nullptr;
    }

    int total() const { return orig_->toBulk()->totalSize(); }

    void onKey(KeyEvent &event) {
        auto *ic = event.inputContext();
        if (!active(ic)) {
            // The engine may have replaced the grid with a fresh list.
            if (grid_) {
                clear();
            }
            if (event.isRelease() || !event.key().check(FcitxKey_Down)) {
                return;
            }
            auto list = ic->inputPanel().candidateList();
            if (!list || list->empty() || !list->toBulk()) {
                return;
            }
            int totalSize = list->toBulk()->totalSize();
            if (totalSize <= 1) {
                return;
            }
            int start = 0;
            if (auto *bulkCursor = list->toBulkCursor()) {
                start = std::max(bulkCursor->globalCursorIndex(), 0);
            } else if (auto *pageable = list->toPageable();
                       pageable && pageable->currentPage() > 0) {
                start = pageable->currentPage() * list->size();
            }
            orig_ = std::move(list);
            ic_ = ic->watch();
            cursor_ = std::min(start, totalSize - 1);
            event.filterAndAccept();
            render(ic);
            return;
        }

        if (event.isRelease()) {
            return;
        }
        const auto &key = event.key();
        const int n = total();
        int row = (cursor_ % kPage) / kCols;
        int col = cursor_ % kCols;

        if (key.check(FcitxKey_Escape)) {
            event.filterAndAccept();
            collapse(ic);
            return;
        }
        if (key.check(FcitxKey_Return) || key.check(FcitxKey_KP_Enter) ||
            key.check(FcitxKey_space)) {
            event.filterAndAccept();
            commit(ic, cursor_);
            return;
        }
        if (key.isDigit() && !key.hasModifier()) {
            int digit = key.digit();
            if (digit >= 1 && digit <= kCols) {
                event.filterAndAccept();
                int idx = cursor_ - col + digit - 1;
                if (idx < n) {
                    commit(ic, idx);
                }
                return;
            }
        }

        int next = cursor_;
        if (key.check(FcitxKey_Right)) {
            next = cursor_ + 1;
        } else if (key.check(FcitxKey_Left)) {
            next = cursor_ - 1;
        } else if (key.check(FcitxKey_Down)) {
            next = cursor_ + kCols;
            if (next >= n) {
                // Last row: land on the final candidate if it is on a lower
                // row, otherwise stay put.
                next = (n - 1) / kCols > cursor_ / kCols ? n - 1 : cursor_;
            }
        } else if (key.check(FcitxKey_Up)) {
            if (cursor_ < kCols) {
                event.filterAndAccept();
                collapse(ic);
                return;
            }
            next = cursor_ - kCols;
        } else if (key.check(FcitxKey_Page_Down) ||
                   key.check(FcitxKey_equal) ||
                   key.check(FcitxKey_bracketright)) {
            next = (cursor_ / kPage + 1) * kPage;
            if (next >= n) {
                next = cursor_;
            }
        } else if (key.check(FcitxKey_Page_Up) || key.check(FcitxKey_minus) ||
                   key.check(FcitxKey_bracketleft)) {
            next = std::max(cursor_ - kPage, 0);
            next = next / kPage * kPage + row * kCols + col;
            next = std::min(next, n - 1);
        } else {
            // Anything else goes back to the engine with its own list.
            collapse(ic);
            return;
        }
        event.filterAndAccept();
        cursor_ = std::clamp(next, 0, n - 1);
        render(ic);
    }

    void render(InputContext *ic) {
        auto *bulk = orig_->toBulk();
        const int n = bulk->totalSize();
        const int pageStart = cursor_ / kPage * kPage;
        const int pageEnd = std::min(pageStart + kPage, n);

        int width = 1;
        for (int i = pageStart; i < pageEnd; i++) {
            width = std::max(
                width, displayWidth(bulk->candidateFromAll(i).text().toString()));
        }
        width = std::min(width, kMaxCellWidth);

        std::vector<Text> rows;
        for (int rowStart = pageStart; rowStart < pageEnd; rowStart += kCols) {
            Text row;
            for (int i = rowStart; i < std::min(rowStart + kCols, pageEnd);
                 i++) {
                auto word = bulk->candidateFromAll(i).text().toString();
                std::string cell = std::to_string(i - rowStart + 1) + ". " + word;
                int pad = width - displayWidth(word);
                for (; pad >= 2; pad -= 2) {
                    cell += kFullWidthSpace;
                }
                if (pad == 1) {
                    cell += kHalfWidthSpace;
                }
                if (i == cursor_) {
                    row.append(cell, TextFormatFlag::HighLight);
                } else {
                    row.append(cell);
                }
                row.append(kFullWidthSpace);
            }
            rows.push_back(std::move(row));
        }

        const int pages = (n + kPage - 1) / kPage;
        // Page indicator below the grid.
        ic->inputPanel().setAuxDown(
            pages > 1 ? Text(std::to_string(pageStart / kPage + 1) + "/" +
                             std::to_string(pages))
                      : Text());

        auto grid = std::make_unique<GridCandidateList>(std::move(rows));
        grid_ = grid.get();
        ic->inputPanel().setCandidateList(std::move(grid));
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }

    void restore(InputContext *ic) {
        auto orig = std::move(orig_);
        clear();
        ic->inputPanel().setAuxDown(Text());
        ic->inputPanel().setCandidateList(
            std::make_unique<ProxyCandidateList>(orig));
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }

    void collapse(InputContext *ic) {
        auto orig = orig_;
        int cursor = cursor_;
        restore(ic);
        // Keep the collapsed list on the page of the chosen candidate.
        if (auto *pageable = orig->toPageable();
            pageable && orig->size() > 0 && pageable->totalPages() > 0) {
            pageable->setPage(
                std::min(cursor / orig->size(), pageable->totalPages() - 1));
        }
        if (auto *bulkCursor = orig->toBulkCursor()) {
            bulkCursor->setGlobalCursorIndex(cursor);
        }
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }

    void commit(InputContext *ic, int idx) {
        auto orig = orig_;
        restore(ic);
        orig->toBulk()->candidateFromAll(idx).select(ic);
    }

    Instance *instance_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>> handlers_;
    TrackableObjectReference<InputContext> ic_;
    std::shared_ptr<CandidateList> orig_;
    const CandidateList *grid_ = nullptr;
    int cursor_ = 0;
};

class MultiSelectorFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new MultiSelector(manager->instance());
    }
};

} // namespace

FCITX_ADDON_FACTORY_V2_BACKWARDS(multiselector, MultiSelectorFactory)
