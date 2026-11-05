#!/bin/bash
# Builds FramePassthrough natively on a Steam Frame (Linux arm64) and packs
#   dist/framepassthrough-<version>-linux-arm64.tar.gz
# with install.sh and uninstall.sh at the top level (Frametanium installer
# mode runs them from the unpacked folder).
#   scripts/make-release.sh [build-dir]       (default: ../build-release)
# Waits while ~/.cache/headset-in-use exists so a build never disturbs a
# headset session.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${1:-$ROOT/../build-release}
DIST=$ROOT/dist
VERSION=$(sed -n 's/^project(FramePassthroughShortcuts VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")
[ -n "$VERSION" ] || { echo "cannot read the version from CMakeLists.txt" >&2; exit 1; }
NAME=framepassthrough-$VERSION-linux-arm64

while [ -e "$HOME/.cache/headset-in-use" ]; do
  echo "headset in use; waiting before building..."
  sleep 60
done

cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DFPS_BUILD_PROBE=OFF > /dev/null
nice -n 10 cmake --build "$BUILD"
ctest --test-dir "$BUILD" --output-on-failure

# The binary must load on a stock Frame: refuse to ship one that needs a
# newer glibc than MAX_GLIBC (override in the environment).
MAX_GLIBC=${MAX_GLIBC:-2.38}
NEEDED_GLIBC=$(objdump -T "$BUILD/bin/frame-passthrough-shortcuts" | grep -o 'GLIBC_[0-9][0-9.]*' | sed 's/GLIBC_//' | sort -uV | tail -1)
echo "highest glibc symbol version needed: GLIBC_$NEEDED_GLIBC (limit $MAX_GLIBC)"
if [ "$(printf '%s\n%s\n' "$NEEDED_GLIBC" "$MAX_GLIBC" | sort -V | tail -1)" != "$MAX_GLIBC" ]; then
  echo "the binary needs GLIBC_$NEEDED_GLIBC, newer than MAX_GLIBC=$MAX_GLIBC" >&2
  exit 1
fi

STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
P=$STAGE/$NAME
mkdir -p "$P/bindings" "$P/third_party/openvr" "$P/third_party/json"
# One stripped binary, copied: SteamVR tells apps apart by executable path,
# so management commands run through an identical frame-passthrough-control.
strip -o "$P/frame-passthrough-shortcuts" "$BUILD/bin/frame-passthrough-shortcuts"
cp "$P/frame-passthrough-shortcuts" "$P/frame-passthrough-control"
chmod 755 "$P/frame-passthrough-shortcuts" "$P/frame-passthrough-control"
cp "$ROOT/assets/actions.json" "$ROOT/assets/config.json" "$P/"
cp "$ROOT/assets/bindings/"*.json "$P/bindings/"
install -m 755 "$ROOT/packaging/install.sh" "$ROOT/packaging/uninstall.sh" "$P/"
cp "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$P/"
cp "$ROOT/third_party/openvr/LICENSE" "$P/third_party/openvr/"
cp "$ROOT/third_party/json/LICENSE.MIT" "$P/third_party/json/"
echo "$VERSION" > "$P/VERSION"

"$P/frame-passthrough-shortcuts" --validate > /dev/null
bash -n "$P/install.sh"
bash -n "$P/uninstall.sh"

mkdir -p "$DIST"
tar -C "$P" --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -cf - . | gzip -9n > "$DIST/$NAME.tar.gz"
(cd "$DIST" && sha256sum "$NAME.tar.gz" > "$NAME.tar.gz.sha256")
echo "$DIST/$NAME.tar.gz"
cat "$DIST/$NAME.tar.gz.sha256"
