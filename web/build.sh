#!/bin/sh
# Build MAG for the browser (Emscripten + Asyncify) into web/dist.
# port/fe_web.c + web/mag.js draw the screen; saves in IndexedDB (IDBFS).
# Deploy with web/deploy.sh. Needs emcc on PATH (web/toolchain.sh).
set -e
cd "$(dirname "$0")/../port"
OUT=../web/dist
rm -rf "$OUT" && mkdir -p "$OUT"
CFLAGS="-O2 -std=gnu89 -fcommon -w -funsigned-char -DPORT -Iinc -I. -include port.h \
	-Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion \
	-Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration ${EMCFLAGS:-}"
OBJ=../web/obj
mkdir -p "$OBJ"
# c-rec reads the tile mapping (the remapper's rec file) at startup
CREC=${CREC:-$HOME/Projects/c-rec}
CFLAGS="$CFLAGS -I$CREC"
REC=${MAG_REC:-mag-dawnlike.rec}
for f in ../src/*.C pcvideo.c rvip.c menu.c tiles.c fe_web.c "$CREC/rec.c"; do
	# the game's .C files are C (emcc would take them for C++)
	emcc $CFLAGS -x c -c "$f" -o "$OBJ/$(basename "$f" | sed 's/\.[cC]$//').o" &
done
wait
emcc -O2 "$OBJ"/*.o \
	--preload-file ../data/help@/magdata/help --preload-file ../data/pics@/magdata/pics \
	--preload-file "$REC@/magdata/tiles.rec" \
	-o "$OUT/mag-core.js" \
	-sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sSTACK_SIZE=1048576 \
	-sALLOW_MEMORY_GROWTH -sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU8,HEAP16,HEAPU16,HEAP32,UTF8ToString,addRunDependency,removeRunDependency \
	-sEXPORTED_FUNCTIONS=_main,_web_set_auto_more,_web_set_icons,_web_vram \
	-sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web -sEXIT_RUNTIME=0 ${EMFLAGS:-}
rm -rf "$OBJ"
RVIP_WM=${RVIP_WM:-$HOME/Games/rvip-tools/web}
cp ../web/index.html ../web/mag.js tiles-dawn.png tiles-dawn-1.png "$OUT/"
# the rec's sheets (file:, anim_file:), relative to the rec
sed -n 's/^\(anim_\)\{0,1\}file: *//p' "$REC" | while read -r s; do cp "$(dirname "$REC")/$s" "$OUT/"; done
# sound effects, synthesized for the events the port names (port_sound())
(cd .. && python3 web/mksounds.py web/dist/sound)

# font choosers: the index page's fonts/*.woff (loaded from ../fonts/)
FONTS=${RVIP_FONTS:-$HOME/Games/roguelikes-index/fonts}
(cd "$FONTS" 2>/dev/null && ls *.woff 2>/dev/null | sed 's/\.woff$//') |
	python3 -c 'import json,sys; print(json.dumps(sys.stdin.read().split()))' > "$OUT/fonts.json"
python3 ../web/make-help.py > "$OUT/help.html"
ls -la "$OUT"
