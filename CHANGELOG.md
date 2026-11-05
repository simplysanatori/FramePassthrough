# Changelog

## 0.2.0 (November 2026)

The first release of FramePassthrough, a fork of KominoVR's Frame Passthrough Shortcuts 0.1.1 for the Valve Steam Frame.

- Restored 0.1.0's camera switch: hold either stick click for one second to switch between the Arcturus RGB colour camera and the monochrome cameras. The RGB and monochrome actions are back in the SteamVR bindings.
- The camera choice is saved and set again when passthrough opens. It is only checked and set while passthrough is showing.
- The gestures only act while passthrough is showing. A gesture started before passthrough opened is ignored.
- New `config.json` options: `disable_while_game_running` (default off) turns the gestures off while a VR game runs; `passthrough_signal` (`auto` default, `setting`, `events`, `steamvr_log`, `off`) picks how passthrough is detected. `--set disable-while-game-running on|off` sets the first. `config.json` is reloaded within a second of an edit.
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
