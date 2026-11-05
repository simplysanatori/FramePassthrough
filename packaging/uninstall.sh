#!/bin/bash
# Removes FramePassthrough: stops the app, removes its SteamVR registration
# and autostart, and deletes ~/.local/share/FramePassthrough/app.
# Preferences in ~/.config/frame-passthrough-shortcuts (camera-preference.json,
# application.log) are kept for a later reinstall.
# Runs non-interactively (Frametanium runs it with stdin on /dev/null).
#
# FRAMEPASSTHROUGH_APP=DIR removes a trial install made with the same variable.
set -uo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
APP_NAME=frame-passthrough-shortcuts
BASE=$HOME/.local/share/FramePassthrough
DEST=${FRAMEPASSTHROUGH_APP:-$BASE/app}

say() { printf '%s\n' "$*"; }

BIN=
for candidate in "$DEST/$APP_NAME" "$HERE/$APP_NAME"; do
  if [ -x "$candidate" ] && [ -x "$(dirname "$candidate")/frame-passthrough-control" ]; then
    BIN=$candidate
    break
  fi
done

STATUS=0
if [ -n "$BIN" ]; then
  if ! "$BIN" --uninstall; then
    say "uninstall.sh: SteamVR deregistration failed (see above); removing the files anyway." >&2
    STATUS=1
  fi
else
  say "uninstall.sh: no installed program found; nothing to deregister." >&2
fi

# Anything still running from the program folder goes too.
for pid in /proc/[0-9]*; do
  pid=${pid#/proc/}
  exe=$(readlink "/proc/$pid/exe" 2> /dev/null) || continue
  case "$exe" in
    "$DEST"/*) kill -TERM "$pid" 2> /dev/null || true ;;
  esac
done

if [ -d "$DEST" ]; then
  rm -rf "$DEST"
  say "Removed $DEST"
fi
rmdir "$(dirname "$DEST")" 2> /dev/null || true
say "FramePassthrough removed. Preferences in ${XDG_CONFIG_HOME:-$HOME/.config}/frame-passthrough-shortcuts were kept."
exit $STATUS
