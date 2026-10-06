# fcitx5-gridcandidate

[English summary below](#english)

给 fcitx5 加上搜狗 / 微信输入法那种「按 ↓ 展开候选」的功能：候选词本来是一行，按 ↓ 后展开成 **4 行 × 8 列**的网格，可以用方向键挑选。

当前版本：**v0.0.1**（早期版本，只在 Bazzite / KDE Plasma Wayland + fcitx5 5.1.22 拼音上测试过）

## 背景：我们要什么

在 Windows / macOS 上用惯了搜狗、微信输入法，换到 Linux（fcitx5）后最不习惯的就是候选框：

- 候选词只有一行，每页 7 个左右，想要的词不在第一页时只能一页一页往后翻；
- 搜狗和微信输入法可以按 ↓ 把这一行展开成多行（4×7、4×8 左右），一屏能看到二三十个词，再用方向键上下左右挑选；
- 希望在 Linux 上有同样的体验，而且**不能影响现有输入法**：拼音配置、词库、用户词频都不能动，随时能关掉。

## 调研：为什么要自己写

| 方案 | 能否展开多行 | 在 KDE Wayland 上可用 |
|---|---|---|
| fcitx5 拼音 / Rime | ❌ 只有横排或竖排一列 | ✅ |
| 微信输入法 Linux 移植版（fcitx5-wetypex） | ❌ 候选框由 fcitx5 绘制，和原生的一样 | ⚠️ 非官方移植，处于早期阶段 |
| 搜狗 Linux 版 | 不确定 | ❌ 只支持 fcitx4，只能在 X11 下用 |
| 竖排候选 + 加大每页数量 | ❌ 只是一列变长 | ✅ |

结论：Linux 上没有现成工具，原因是 fcitx5 的候选框只有横排和竖排两种布局。

## 本项目解决的问题

- 按 ↓ 把一行候选展开成 4×8 网格，可以上下左右选择，数字键直接选词，并且能翻页；
- 做成独立的 fcitx5 插件：不打补丁、不改配置、不动系统文件，所有文件都装在用户目录里，卸载后即恢复原样；
- 上屏时调用的仍是输入法自己的候选对象，词频学习、整句选词等行为不变。

## 按键

| 按键 | 作用 |
|---|---|
| ↓（有候选时） | 展开网格 |
| ↑ ↓ ← → | 在网格里移动 |
| 空格 / 回车 | 上屏选中的词 |
| 1–8 | 上屏当前行第 N 个词 |
| PgDn / `=`，PgUp / `-` | 翻页（网格下方显示页码，如 `1/7`） |
| Esc，或在第一行按 ↑ | 收回成一行 |
| 其他键 | 自动收起，按键照常交给输入法（可以接着打字） |

## 原理

- 插件在输入法引擎之前拦截按键（`PreInputMethod` 阶段）。
- 展开时保存引擎原来的候选列表，界面上换成网格。网格每一行是一个候选项，选中的词用 HighLight 格式标出。
- 上屏时调用的是原列表里的候选对象，引擎的行为（用户词频、整句选择等）不变。
- 收起时用代理对象把原列表放回去，翻页和光标移动照常可用。
- 不修改 fcitx5、拼音的配置和系统文件。按理任何提供完整候选列表（BulkCandidateList）的输入法都能用，目前只测过 fcitx5 拼音。

## 编译和安装

需要 fcitx5 ≥ 5.1.20 的开发头文件、cmake 和 g++。在 Fedora Atomic / Bazzite 这类不可变系统上，建议在 toolbox 里编译：

```bash
toolbox create --distro fedora --release 44 imegrid
toolbox run -c imegrid sudo dnf install -y fcitx5-devel gcc-c++ cmake
./install.sh
```

`install.sh` 会安装两个文件：
- `~/.local/lib/fcitx5/gridcandidate.so`
- `~/.local/share/fcitx5/addon/gridcandidate.conf`（里面写的是 .so 的绝对路径，所以不需要设置 `FCITX_ADDON_DIRS`）

装好后重启 fcitx5 生效。

### KDE Wayland 下怎么重启 fcitx5

在 KDE Wayland 下，fcitx5 由 KWin 启动：DBus 的 `Restart` 不起作用，kill 掉之后 KWin 也不会自动拉起。可以这样操作：

```bash
pkill -x fcitx5
kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod /usr/share/applications/org.fcitx.Fcitx5.desktop
```

最简单的办法是注销后重新登录。

## 卸载

```bash
./uninstall.sh   # 然后重启 fcitx5
```

## 已知限制

- 网格收起后，拼音的 Tab 笔画筛选不生效，接着打字就恢复正常。
- 列宽按全角字符补齐，英文和 emoji 候选可能对不齐。
- 行数和列数写死在 `gridcandidate.cpp` 的 `kRows` / `kCols`。
- fcitx5 升级大版本后需要重新编译。

## 许可证

LGPL-2.1-or-later，与 fcitx5 一致。见 [LICENSE](LICENSE)。

## English

**fcitx5-gridcandidate** is a small fcitx5 addon that adds Sogou/WeType-style
candidate expansion: while the one-line candidate list is shown, press **Down**
to expand it into a **4×8 grid**, navigate with the arrow keys, commit with
Space/Enter or 1–8, page with PgUp/PgDn, and collapse with Esc. It works as a
pure view on top of the input method's own candidate list, so engine behaviour
(user history, partial selection) is unchanged and no system files or IME
configuration are touched. Build with `./install.sh` (needs fcitx5 ≥ 5.1.20
headers, cmake, g++); remove with `./uninstall.sh`.
