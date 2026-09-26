#!/bin/sh
# Upload web/dist to https://ruzzoli.de/roguelikes/mag/ (RVIP W9).
# Deploy only pushed commits: build from a clean tree first (sh web/build.sh).
set -e
cd "$(dirname "$0")"
cd ..
if [ -n "$(git status --porcelain)" ]; then echo "uncommitted changes: commit and push first" >&2; exit 1; fi
git fetch -q origin main
if [ "$(git rev-parse HEAD)" != "$(git rev-parse origin/main)" ]; then echo "HEAD is not origin/main: push first" >&2; exit 1; fi
cd web
test -f dist/mag-core.wasm && test -f dist/help.html || { echo "no build in web/dist: sh web/build.sh" >&2; exit 1; }
ssh ruzzoli.de 'sudo mkdir -p /var/www/ruzzoli.de/roguelikes/mag && sudo chown -R felix:www-data /var/www/ruzzoli.de/roguelikes/mag'
rsync -rtz --delete dist/ ruzzoli.de:/var/www/ruzzoli.de/roguelikes/mag/
echo "deployed; check: curl -sI https://ruzzoli.de/roguelikes/mag/mag-core.wasm"
