#!/bin/sh
# Remap MAG's tiles: opens port/mag-dawnlike.rec in the remapper (F1 help, s save),
# then rebuilds the web version so web/dist shows the new mapping.
# REMAPPER: the remapper checkout (default ~/Projects/remapper), built if missing.
set -e
cd "$(dirname "$0")/port"
REMAPPER=${REMAPPER:-$HOME/Projects/remapper}
command -v remapper >/dev/null || "$REMAPPER/deploy.sh"
remapper mag-dawnlike.rec
cd .. && sh web/build.sh >/dev/null && echo "web/dist rebuilt with the new mapping"
