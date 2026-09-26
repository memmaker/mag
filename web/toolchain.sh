#!/bin/sh
# Toolchain for the MAG web port, as installed in the RVIP cloud session
# (2026-09-26, Ubuntu 24.04 container). Run once; then `. $EMSDK/emsdk_env.sh`.
set -e
EMSDK=${EMSDK:-$HOME/tools/emsdk}
# Emscripten: emsdk "latest" gave emcc 6.0.10 (d6c521a7), ~2 min download
git clone --depth 1 https://github.com/emscripten-core/emsdk "$EMSDK"
(cd "$EMSDK" && ./emsdk install latest && ./emsdk activate latest)
. "$EMSDK/emsdk_env.sh"
emcc --version | head -1
# Native test build (port/Makefile): gcc 13.3 with ASan/UBSan
# (Ubuntu clang 18 has no ASan runtime installed here; gcc's libasan works)
gcc --version | head -1
# Browser tests: Playwright 1.56.1 (global npm) with the preinstalled Chromium
# in /opt/pw-browsers (PLAYWRIGHT_BROWSERS_PATH); no `playwright install`.
# Tiles: python3 + Pillow (port/mktiles.py), node 22 for the test scripts.
# Stage 4 tiles (2026-09-26, resumed session): Pillow for port/mkdawn.py and
# the DawnLike sprites named by DawnLikeAtlas
pip install pillow || pip install --break-system-packages pillow
git clone --depth 1 https://github.com/tommyettinger/DawnLikeAtlas "$HOME/tools/DawnLikeAtlas"
python3 port/mkdawn.py "$HOME/tools/DawnLikeAtlas"
# Mac (maintainer, 2026-09-26): Homebrew emscripten (`brew install emscripten`,
# emcc 6.0.10 in /opt/homebrew/bin), Python 3 + Pillow 12. build.sh copies
# rvip-wm.js and rvip-sound.js from ~/Games/rvip-tools/web (RVIP_WM=...), not
# from rvip/; make-help.py reads the Docs entry mag.html.
