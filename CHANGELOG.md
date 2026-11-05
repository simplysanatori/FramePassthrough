# Changelog

## 0.2.0 (November 2026)

The first release of FramePassthrough, a fork of KominoVR's Frame Passthrough Shortcuts 0.1.1 for the Valve Steam Frame.

### Camera switching

- Hold either stick click for a second to switch between the Arcturus RGB colour camera and the monochrome cameras, as in 0.1.0. Separate RGB and monochrome actions are back in the SteamVR bindings.
- The choice is saved and set again each time passthrough opens.
- The saved camera is only checked and set while passthrough is showing. Other camera tools are left alone the rest of the time.

### Gestures

- The gestures only act while passthrough is showing. While it is off they do nothing.
- A gesture started before passthrough opened is ignored.
- `disable_while_game_running` in `config.json`, off by default, turns the gestures off while a VR game runs. `--set disable-while-game-running on|off` changes it.
- `passthrough_signal` in `config.json` picks how the app tells that passthrough is showing: `auto` (default), `setting`, `events`, `steamvr_log` or `off`.
- `config.json` is reloaded within a second of an edit.

### SteamVR and logs

- Registered as an internal SteamVR app, so nothing is added to the Steam library.
- The log only reports this app's own binding errors, and every line has a timestamp.
- `--install` works with SteamVR closed. The app starts with SteamVR next time.
- `--uninstall` keeps the camera choice and the log, and no longer rewrites SteamVR's settings file, so the binding choice is kept.

### Install

- A `framepassthrough-<version>-linux-arm64.tar.gz` release with `install.sh` and `uninstall.sh` at the top, for Frametanium or by hand.
- Installs to `~/.local/share/FramePassthrough/app`. Keeps the original's app key and `~/.config/frame-passthrough-shortcuts`, so it replaces the original in place with its camera choice, binding choice and autostart.
- `install.sh` keeps an earlier `config.json`, or takes over the original's, and stops any running copy before it swaps the program in.

## 0.1.1 — 2026-11-04

- Double-press either thumbstick to toggle translucent / opaque passthrough.
- Separate translucent and opaque actions remain configurable in SteamVR.

## 0.1.0

Initial release of Frame Passthrough Shortcuts for Steam Frame.
