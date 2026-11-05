# FramePassthrough 0.2.0

My fork of KominoVR's Frame Passthrough Shortcuts, a small SteamVR background app for the Steam Frame. Double-press either thumbstick to switch translucent and opaque passthrough. Hold a stick click for one second to switch between the colour and mono cameras.

Thanks to KominoVR for the original app.

### Changes

- The colour / mono camera switch is restored. The original removed it in 0.1.1 in favour of Valve's Arcturus Vision Camera app, which has no shortcuts.
- Your camera choice is saved and set again each time you open passthrough. 0.1.0 set it many times a second, all the time. Now it only does so while passthrough is showing, so other camera tools are not overridden.
- The gestures only work while passthrough is showing. The original acted on every double-press with the dashboard closed. A press started before passthrough opened is ignored.
- New option `disable_while_game_running`, off by default, turns the gestures off while a VR game runs. Set it in `config.json` or with `frame-passthrough-shortcuts --set disable-while-game-running on`. Edits to `config.json` apply within a second.
- It adds nothing to your Steam library, and its log lists only its own binding errors, with timestamps.
- It replaces the original in place and keeps your camera choice, bindings and SteamVR autostart. Uninstalling keeps the camera and binding choices.

## Download

- `framepassthrough-0.2.0-linux-arm64.tar.gz`
- `framepassthrough-0.2.0-linux-arm64.tar.gz.sha256`

## Install

**With Frametanium.** Add `simplysanatori/FramePassthrough`. Frametanium installs it and updates it when I publish a new version.

**By hand.** Unpack the tarball into a folder and run `./install.sh` from it. The app starts with SteamVR.

If the original is installed, install over it, then remove its entry from your library. The [README](https://github.com/simplysanatori/FramePassthrough/blob/steam-frame/README.md) has the full steps and settings.

## Known issues

- SteamVR still sends the stick clicks to the game too. It does this for every overlay app, unless its experimental global action set priority setting is on. With passthrough off, my app ignores them. With passthrough open over a game, the game gets the click as well. `disable_while_game_running` stops my app reacting while a game runs.
- The gestures need the SteamVR dashboard closed.
- The camera switch uses SteamVR's internal camera controls. A SteamVR update can change them.
- Choosing the mono cameras does not power off the RGB camera.

## Reporting problems

Open an issue with what you did, what you expected and what happened. Attach `application.log` and `runtime.json` from `~/.config/frame-passthrough-shortcuts`.
