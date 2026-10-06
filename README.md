# fcitx5-gridcandidate

给 fcitx5 加上搜狗 / 微信输入法那种「按 ↓ 展开候选」的功能：候选词本来是一行，按 ↓ 后展开成 **4 行 × 8 列**的网格，可以用方向键挑选。

Linux 上没有现成工具能做到这一点，fcitx5 自带的候选框只有横排和竖排两种，所以写了这个插件。

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
