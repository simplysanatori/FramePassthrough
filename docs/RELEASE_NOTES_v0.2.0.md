# FramePassthrough 0.2.0

My fork of KominoVR's Frame Passthrough Shortcuts for the Steam Frame. The original stays active during games, so a game's own thumbstick double-press or hold can switch your passthrough. This fork detects SteamVR and OpenXR games and whether passthrough is showing, and only acts in Home with passthrough showing.

Thanks to KominoVR for the original app.

### Changes

- The gestures act only while passthrough is showing and no game is running. The original acted on every double-press with the dashboard closed.
- A press started before passthrough opened is ignored.
- `disable_while_game_running` is on by default. Set it to `false` in `config.json`, or run `frame-passthrough-shortcuts --set disable-while-game-running off`, to let the gestures act in games while passthrough is open.
- The colour / mono camera switch is restored: hold a stick click for one second. The original removed it in 0.1.1.
- Your camera choice is saved and set again when passthrough opens. 0.1.0 set it many times a second, all the time. Now it only does so while passthrough is showing.
- It adds nothing to your Steam library, and its log lists only its own errors.
- It replaces the original in place and keeps your camera choice, bindings and SteamVR autostart.

## Install

**With Frametanium.** Add `simplysanatori/FramePassthrough`.

**By hand.** Download `framepassthrough-0.2.0-linux-arm64.tar.gz`, unpack it into a folder and run `./install.sh` from it.

If the original is installed, install over it, then remove its entry from your library. The [README](https://github.com/simplysanatori/FramePassthrough/blob/steam-frame/README.md) has the details.

## Known issues

- The gestures need the SteamVR dashboard closed.
- If you let the gestures act in games, the game still gets the stick clicks too. SteamVR does that for every overlay app.
- The camera switch uses SteamVR's internal camera controls. A SteamVR update can change them.
- Choosing the mono cameras does not power off the RGB camera.

## Reporting problems

Open an issue with what you did, what happened, and `application.log` from `~/.config/frame-passthrough-shortcuts`.
