# Changelog

## 0.2.0 (November 2026)

The first release of FramePassthrough, a fork of KominoVR's Frame Passthrough Shortcuts 0.1.1 for the Steam Frame. The original reacts to the stick gestures all the time, so a game's own stick hold or double-press can trigger it. This fork only reacts while passthrough is showing, in Home or in a game.

- The gestures act only while passthrough is showing. A gesture started before passthrough opened is ignored.
- `disable_while_game_running` (default `false`): `true`, or `--set disable-while-game-running on`, turns the gestures off while a SteamVR or OpenXR game runs, even with passthrough open.
- `passthrough_signal` (`auto` default, `setting`, `events`, `steamvr_log`, `off`) picks how passthrough is detected. `config.json` is reloaded within a second of an edit.
- Restored 0.1.0's camera switch: hold either stick click for one second to switch between the Arcturus RGB colour camera and the monochrome cameras. The RGB and monochrome actions are back in the SteamVR bindings.
- The camera choice is saved and set again when passthrough opens. It is only checked while passthrough is showing, and only written when it differs.
- Registered as an internal SteamVR app, so nothing is added to the Steam library.
- The log reports only this app's binding errors, with timestamps.
- `--install` works with SteamVR closed; the app starts with SteamVR next time.
- `--uninstall` keeps the camera choice and the log, and leaves SteamVR's settings file (and the binding choice) alone.
- Release tarball `framepassthrough-<version>-linux-arm64.tar.gz` with `install.sh` and `uninstall.sh`, for Frametanium or by hand. Installs to `~/.local/share/FramePassthrough/app` and keeps the original's app key and `~/.config/frame-passthrough-shortcuts`, so it replaces the original in place. `install.sh` keeps an earlier or the original's `config.json` and stops any running copy first.

## 0.1.1 — 2026-11-04

- Double-press either thumbstick to toggle translucent / opaque passthrough.
- Separate translucent and opaque actions remain configurable in SteamVR.

## 0.1.0

Initial release of Frame Passthrough Shortcuts for Steam Frame.
