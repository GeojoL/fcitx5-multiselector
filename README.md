# fcitx5-multiselector

**fcitx5 候选词展开插件**：候选只有一行时，按 **↓** 把它展开成 **4 行 × 8 列**，用方向键挑选，用法和搜狗、微信输入法一样。

平时的样子（一行）：

![展开前](docs/before.png)

按 ↓ 之后（4×8，方向键移动，灰色是当前选中）：

![展开后](docs/grid.png)

> 这是 fcitx5 的一个**插件**，不是新的输入法。装上后你的拼音照常使用，只是多了「↓ 展开」这一个功能。
> 截图来自 fcitx5 默认主题，是在虚拟显示里实际渲染出来的。

[安装](#安装) · [按键](#按键) · [常见问题](#常见问题) · [更新日志](CHANGELOG.md) · [English](#english)

## 为什么做这个

在 Windows、macOS 上，搜狗和微信输入法按 ↓ 就能展开候选，一眼看到二三十个词。换到 Linux 的 fcitx5 后，候选框只有一行（拼音默认每页 7 个）。要找的字不在第一页时，只能按 `=` 一页一页往后翻。

fcitx5 自带的候选框只支持横排和竖排，没有多行网格，所以做了这个插件。它独立于输入法运行，不需要改任何设置。

## 安装

需要 fcitx5 ≥ 5.1.12，以及 cmake 和 g++。

```bash
# Fedora
sudo dnf install fcitx5-devel cmake gcc-c++
# Debian 13
sudo apt install libfcitx5core-dev cmake g++
# Arch（待验证）
sudo pacman -S fcitx5 cmake gcc

git clone https://github.com/GeojoL/fcitx5-multiselector.git
cd fcitx5-multiselector
./install.sh
```

装好后**注销并重新登录**（或者重启 fcitx5）即可生效。卸载：运行 `./uninstall.sh` 后重新登录。

插件只往用户目录装两个文件：`~/.local/lib/fcitx5/multiselector.so` 和 `~/.local/share/fcitx5/addon/multiselector.conf`。不用 sudo 安装，也不改系统文件。

Fedora Atomic、Bazzite 这类不可变系统：本机没有 cmake 时，`install.sh` 会自动在名为 `fcitx5-build` 的 toolbox 里编译（可以用 `TOOLBOX=名字` 指定别的容器）。

## 按键

| 按键 | 作用 |
|---|---|
| ↓（有候选时） | 展开 |
| ↑ ↓ ← → | 移动 |
| 空格 / 回车 | 上屏选中的词 |
| 1–8 | 上屏当前行第 N 个 |
| PgDn / `=`，PgUp / `-` | 翻页（顶部显示页码，如 `1/13`） |
| Esc，或在第一行按 ↑ | 收起 |
| 其他键 | 自动收起，可以接着打字 |

## 适用范围

| 项目 | 状态 |
|---|---|
| Bazzite（Fedora 44）+ KDE Plasma Wayland + fcitx5 5.1.22 + fcitx5 拼音 | ✅ 已实际使用 |
| fcitx5 5.1.12（Debian 13）| ⚠️ 能编译通过，实际使用**待验证** |
| fcitx5 5.1.7（Ubuntu 24.04）及更低版本 | ❌ 编译失败，不支持 |
| GNOME、Sway 等其他桌面 / X11 | **待验证** |
| Rime 等其他 fcitx5 输入法 | **待验证**（按原理应该可用） |
| kimpanel 等非经典界面（classicui）的候选框 | **待验证**（网格靠文字高亮显示，效果可能不同） |

欢迎在 Issue 里反馈你的测试结果。

## 常见问题

**装了按 ↓ 没反应？**
先确认 fcitx5 已经重启过，再看日志里有没有 `Loaded addon multiselector`。比如 KDE 下 fcitx5 由 KWin 启动，可以运行 `journalctl --user -b | grep multiselector`。

**能改成 4×7 或别的大小吗？**
目前需要改 `multiselector.cpp` 开头的 `kRows` / `kCols`，然后重新运行 `./install.sh`。

**和把候选框设成竖排有什么区别？**
竖排只是把一行变成一列，每页的数量不变。网格一屏能看到 32 个候选，可以上下左右跳着选。

**会影响词频、词库吗？**
不会。上屏用的还是输入法自己的候选，用户词频照常学习（已验证）。

**KDE Wayland 下怎么手动重启 fcitx5？**
fcitx5 由 KWin 启动。作者实测：kill 掉后 KWin 不会自动拉起；通过 DBus 重启也不生效（`fcitx5 -r` 没测过）。以下办法实测可行：
```bash
pkill -x fcitx5
kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod ""
kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod /usr/share/applications/org.fcitx.Fcitx5.desktop
```

## 已知限制

- 网格收起后，拼音的 Tab 笔画筛选暂时不生效，接着打字就恢复。
- 英文和 emoji 候选可能对不齐。
- 行数和列数还不能在设置界面里调整。
- fcitx5 升级大版本后需要重新运行 `./install.sh`。

## 原理（给开发者）

插件在输入法之前拦截按键。展开时保存输入法原来的候选列表，界面上换成网格（每行是一个候选项，选中的词用高亮格式标出）。上屏时调用原列表里的候选，收起时用代理对象把原列表放回去。只要输入法提供完整的候选列表（BulkCandidateList），插件就能工作。

## 许可证

LGPL-2.1-or-later，与 fcitx5 一致。

## English

**fcitx5-multiselector** is a fcitx5 addon (not a new input method) that
brings Sogou / WeType-style candidate expansion to Linux. While the usual
one-line candidate list is shown, press **Down** to expand it into a **4×8
grid**, move with the arrow keys, commit with Space / Enter / 1–8, page with
PgUp / PgDn, and collapse with Esc (or just keep typing).

Why: fcitx5's classic UI only offers a horizontal or vertical candidate list,
so picking a word that is not on the first page means paging blindly. Sogou and
WeType let you see 30+ candidates at once; this addon does the same on fcitx5.

It is a pure view over the input method's own candidate list, so engine
behaviour (e.g. user history) is unchanged and no system files
or IME settings are modified. Requires fcitx5 ≥ 5.1.12. Build and install with
`./install.sh`; remove with `./uninstall.sh`. Used daily on KDE Plasma Wayland
with fcitx5 Pinyin (Fedora 44, fcitx5 5.1.22); builds on Debian 13; other
setups are untested.
