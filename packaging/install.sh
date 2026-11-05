#!/bin/bash
# Installs FramePassthrough (passthrough shortcuts for Steam Frame) from an
# unpacked release: copies it to ~/.local/share/FramePassthrough/app and
# registers it with SteamVR as an autostarting overlay app.
#
# Safe to run again (updates in place). It
# - keeps the config.json of an earlier install, or takes over the one from
#   the original Frame Passthrough Shortcuts folder
#   (~/devkit-game/Frame_Passthrough_Shortcuts) on first install;
# - never touches ~/.config/frame-passthrough-shortcuts/camera-preference.json
#   or the SteamVR binding choice (same app key as the original app);
# - stops any running copy of the shortcuts app, old or new, before the
#   swap, so only one copy ever runs;
# - registers what it can when SteamVR is not running; the shortcuts then
#   start with SteamVR next time.
# Runs non-interactively (Frametanium runs it with stdin on /dev/null).
#
# For a trial install, FRAMEPASSTHROUGH_APP=DIR puts the program elsewhere.
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
APP_NAME=frame-passthrough-shortcuts
BASE=$HOME/.local/share/FramePassthrough
DEST=${FRAMEPASSTHROUGH_APP:-$BASE/app}
STATE=${XDG_CONFIG_HOME:-$HOME/.config}/frame-passthrough-shortcuts
OLD_APP=$HOME/devkit-game/Frame_Passthrough_Shortcuts
VERSION=$(cat "$HERE/VERSION" 2> /dev/null || echo unknown)

say() { printf '%s\n' "$*"; }
fail() { printf 'install.sh: %s\n' "$*" >&2; exit 1; }

for f in "$APP_NAME" frame-passthrough-control actions.json config.json bindings/frame_controller.json; do
  [ -e "$HERE/$f" ] || fail "$f is missing next to install.sh; unpack the whole release archive."
done

say "Installing FramePassthrough $VERSION to $DEST"

# --- Stage the new copy beside the destination -----------------------------
PARENT=$(dirname "$DEST")
mkdir -p "$PARENT"
STAGE=$(mktemp -d "$PARENT/.app.new.XXXXXX")
trap 'rm -rf "$STAGE"' EXIT
cp -a "$HERE/$APP_NAME" "$HERE/frame-passthrough-control" "$HERE/actions.json" "$HERE/bindings" "$STAGE/"
cp -a "$HERE/config.json" "$STAGE/config.default.json"
for f in VERSION LICENSE THIRD_PARTY_NOTICES.md install.sh uninstall.sh third_party; do
  [ -e "$HERE/$f" ] && cp -a "$HERE/$f" "$STAGE/"
done
chmod 755 "$STAGE/$APP_NAME" "$STAGE/frame-passthrough-control"

# --- Keep the user's config ------------------------------------------------
valid_config() {
  "$STAGE/$APP_NAME" --validate --config "$1" > /dev/null 2>&1
}
CONFIG_FROM=
for candidate in "$DEST/config.json" "$OLD_APP/config.json"; do
  if [ -f "$candidate" ]; then
    if valid_config "$candidate"; then
      CONFIG_FROM=$candidate
      break
    fi
    say "Ignoring $candidate: this version cannot read it."
  fi
done
if [ -n "$CONFIG_FROM" ]; then
  cp "$CONFIG_FROM" "$STAGE/config.json"
  say "Kept settings from $CONFIG_FROM"
else
  cp "$STAGE/config.default.json" "$STAGE/config.json"
fi

# --- Stop every running copy (old devkit copy, this app, any other) -------
running_copies() {
  local pid exe
  for pid in /proc/[0-9]*; do
    pid=${pid#/proc/}
    exe=$(readlink "/proc/$pid/exe" 2> /dev/null) || continue
    case "$exe" in
      */$APP_NAME|*/$APP_NAME" (deleted)") ;;
      *) continue ;;
    esac
    # Only our own user's processes from a known install, or the one that
    # wrote runtime.json.
    case "$exe" in
      "$DEST"/*|"$OLD_APP"/*|"$PARENT"/.app.old.*) echo "$pid"; continue ;;
    esac
    if [ -f "$STATE/runtime.json" ] && grep -q "\"pid\":$pid[,}]" "$STATE/runtime.json"; then
      echo "$pid"
    fi
  done
}
PIDS=$(running_copies || true)
if [ -n "$PIDS" ]; then
  say "Stopping the running shortcuts app (pid $(echo $PIDS))"
  kill -TERM $PIDS 2> /dev/null || true
  for _ in $(seq 50); do
    sleep 0.1
    LEFT=
    for p in $PIDS; do [ -d "/proc/$p" ] && LEFT="$LEFT $p"; done
    [ -z "$LEFT" ] && break
  done
  if [ -n "${LEFT:-}" ]; then
    kill -KILL $LEFT 2> /dev/null || true
    sleep 0.5
  fi
fi
# The app holds an exclusive lock while it runs. If something still holds
# it, a copy we do not know about is running: do not start a second one.
if [ -f "$STATE/instance.lock" ] && command -v flock > /dev/null; then
  flock -n "$STATE/instance.lock" true || fail "another copy of the shortcuts app is still running (it holds $STATE/instance.lock); stop it and run install.sh again."
fi

# --- Swap the program folder in -------------------------------------------
OLD=
if [ -e "$DEST" ]; then
  OLD=$(mktemp -u "$PARENT/.app.old.XXXXXX")
  mv "$DEST" "$OLD"
fi
mv "$STAGE" "$DEST"
trap - EXIT
[ -n "$OLD" ] && rm -rf "$OLD"

# --- Register with SteamVR -------------------------------------------------
if ! "$DEST/$APP_NAME" --install; then
  fail "SteamVR registration failed (see the message above). The files are installed in $DEST; run $DEST/$APP_NAME --install to retry."
fi

if pgrep -x vrserver > /dev/null 2>&1; then
  sleep 2
  COUNT=$(running_copies | wc -l)
  if [ "$COUNT" -eq 0 ]; then
    # SteamVR may not have noticed the old copy exit yet; ask once more.
    "$DEST/$APP_NAME" --install > /dev/null 2>&1 || true
    sleep 2
    COUNT=$(running_copies | wc -l)
  fi
  [ "$COUNT" -eq 1 ] || say "warning: $COUNT copies of the shortcuts app are running (expected 1)."
fi
say "FramePassthrough $VERSION installed. Double-press either thumbstick to switch translucent / opaque"
say "and hold one for a second to switch RGB / monochrome, while passthrough is showing."
