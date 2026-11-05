# FramePassthrough 0.2.0

My fork of KominoVR's Frame Passthrough Shortcuts, a small SteamVR background app for the Steam Frame. Double-press either thumbstick to switch translucent and opaque passthrough. Hold a stick click for one second to switch between the colour and mono cameras.

Thanks to KominoVR for the original app.

### Changes

- The colour / mono camera switch is restored. The original removed it in 0.1.1.
- Your camera choice is saved and set again when passthrough opens. 0.1.0 set it many times a second, all the time. Now it only does so while passthrough is showing.
- The gestures only work while passthrough is showing, and ignore a press started before it opened. The original acted on every double-press with the dashboard closed.
- New option `disable_while_game_running` turns the gestures off while a VR game runs. Set it in `config.json` or with `frame-passthrough-shortcuts --set disable-while-game-running on`.
- It adds nothing to your Steam library, and its log lists only its own errors.
- It replaces the original in place and keeps your camera choice, bindings and SteamVR autostart.

## Install

**With Frametanium.** Add `simplysanatori/FramePassthrough`.

**By hand.** Download `framepassthrough-0.2.0-linux-arm64.tar.gz`, unpack it into a folder and run `./install.sh` from it.

If the original is installed, install over it, then remove its entry from your library. The [README](https://github.com/simplysanatori/FramePassthrough/blob/steam-frame/README.md) has the full steps and settings.

## Known issues

- SteamVR still sends the stick clicks to the game too. It does this for every overlay app, unless its experimental global action set priority setting is on. With passthrough off, my app ignores them. With passthrough open over a game, the game gets the click as well. `disable_while_game_running` stops my app reacting while a game runs.
- The gestures need the SteamVR dashboard closed.
- The camera switch uses SteamVR's internal camera controls. A SteamVR update can change them.
- Choosing the mono cameras does not power off the RGB camera.

## Reporting problems

Open an issue with what you did, what happened, and `application.log` from `~/.config/frame-passthrough-shortcuts`.
