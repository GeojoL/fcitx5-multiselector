# fcitx5-multiselector

**fcitx5 候选词展开插件**：打字时候选只有一行，按 **↓** 就能把它展开成 **4 行 × 8 列**，一屏看到 32 个候选，再用方向键上下左右挑选。操作方式和 Windows / macOS 上的搜狗、微信输入法一样。

> 这是一个 fcitx5 **插件**，不是新的输入法，也不会替换你现在用的输入法。装上之后，拼音照常用，只是多了「↓ 展开」这一个功能。当前版本 v0.0.1。

[English](#english)

## 为什么需要它

在 Windows、macOS 上用过搜狗或微信输入法的人，换到 Linux 的 fcitx5 后常会遇到这个问题：

- 候选框只有一行，每页 5～10 个词。要找的词（生僻字、同音字、人名用字）不在第一页时，只能按 `=` 一页一页往后翻，看不到全貌。
- 搜狗、微信输入法按 ↓ 可以把候选展开成多行，一眼扫过二三十个词，再用方向键直接选。Linux 上没有这个功能。

现有的办法都解决不了：

- **fcitx5 自带的候选框**（经典界面）只有「横排」和「竖排」两种布局。改成竖排、调大每页数量，只是一列变长，并不是多行网格。
- **搜狗输入法 Linux 版**基于旧的 fcitx4，在 Wayland 下基本用不了，现在主流桌面默认都是 Wayland。
- **Rime 等其他输入法引擎**的候选框同样由 fcitx5 绘制，所以也没有这个功能。

## 它解决了什么

- **一屏看到更多候选**：按 ↓ 展开成 4×8 网格，方向键移动，空格、回车或数字键上屏，还能整页翻。
- **不改变原来的输入习惯**：不按 ↓ 时一切照旧；展开后按其他键（比如继续打字）会自动收起。
- **不影响输入法本身**：选词时调用的还是输入法自己的候选，用户词频、整句选词照常工作。插件不改任何配置和系统文件，卸载后即恢复原样。

## 适用范围

- 需要 fcitx5 ≥ 5.1.20，使用默认的经典界面（classicui）。
- 已测试：Bazzite（Fedora 44）+ KDE Plasma Wayland + fcitx5 5.1.22 + fcitx5 拼音。
- 理论上也适用于其他提供完整候选列表的 fcitx5 输入法（比如 Rime），但还没测试过，欢迎反馈。

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
- 只要输入法提供完整的候选列表（BulkCandidateList），插件就能工作。

## 编译和安装

需要 fcitx5 的开发头文件、cmake 和 g++：

| 发行版 | 依赖包 |
|---|---|
| Fedora | `sudo dnf install fcitx5-devel cmake gcc-c++` |
| Arch | `sudo pacman -S fcitx5 cmake gcc`（头文件包含在 fcitx5 包里） |
| Debian / Ubuntu | `sudo apt install libfcitx5core-dev cmake g++` |

Fedora Atomic、Bazzite 这类不可变系统上，可以在 toolbox 里编译（容器版本要和系统一致）：

```bash
toolbox create fcitx5-build
toolbox run -c fcitx5-build sudo dnf install -y fcitx5-devel cmake gcc-c++
```

然后在项目目录里运行：

```bash
./install.sh
```

本机装了 cmake 时，`install.sh` 直接在本机编译；没装时，改用名为 `fcitx5-build` 的 toolbox 编译（可以用环境变量 `TOOLBOX=名字` 指定别的容器）。

安装后会多出两个文件，都在用户目录里：
- `~/.local/lib/fcitx5/multiselector.so`
- `~/.local/share/fcitx5/addon/multiselector.conf`（里面写的是 .so 的绝对路径，所以不需要设置环境变量）

装好后**重启 fcitx5** 生效。最省事的办法是注销后重新登录。

### KDE Wayland 下手动重启 fcitx5

KDE Wayland 下 fcitx5 由 KWin 启动，`fcitx5 -r` 和 kill 都不能让它干净地重新启动。可以让 KWin 重新拉起它：

```bash
pkill -x fcitx5
kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod ""
kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod /usr/share/applications/org.fcitx.Fcitx5.desktop
```

## 卸载

```bash
./uninstall.sh   # 然后重启 fcitx5
```

## 已知限制

- 网格收起后，拼音的 Tab 笔画筛选不生效，接着打字就恢复正常。
- 列宽按全角字符补齐，英文和 emoji 候选可能对不齐。
- 行数和列数写死在 `multiselector.cpp` 的 `kRows` / `kCols`。
- fcitx5 升级大版本后需要重新编译。

## 许可证

LGPL-2.1-or-later，与 fcitx5 一致。见 [LICENSE](LICENSE)。

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
behaviour (user history, partial selection) is unchanged and no system files
or IME settings are modified. Requires fcitx5 ≥ 5.1.20. Build and install with
`./install.sh`; remove with `./uninstall.sh`. Tested on KDE Plasma Wayland
with fcitx5 Pinyin.
