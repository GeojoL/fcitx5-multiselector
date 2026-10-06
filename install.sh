#!/bin/bash
# Build in the imegrid toolbox, install into the user's home. Remove with uninstall.sh.
set -euo pipefail
cd "$(dirname "$0")"
toolbox run -c imegrid bash -c 'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build build'
LIBDIR="$HOME/.local/lib/fcitx5"
mkdir -p "$LIBDIR" "$HOME/.local/share/fcitx5/addon"
install -m755 build/multiselector.so "$LIBDIR/multiselector.so"
sed "s|@LIBDIR@|$LIBDIR|" multiselector.conf.in > "$HOME/.local/share/fcitx5/addon/multiselector.conf"
echo "installed; restart fcitx5 to load"
