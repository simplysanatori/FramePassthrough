# FramePassthrough 0.2.0

Passthrough shortcuts on the Steam Frame's thumbsticks. FramePassthrough is my fork of KominoVR's Frame Passthrough Shortcuts. It is a small SteamVR background app that runs on the Frame. Double-press either thumbstick to switch translucent and opaque passthrough. Hold one down for a second to switch between the colour and mono cameras.

Thank you to KominoVR for the original app. The idea and the groundwork are theirs, and this fork stands on them.

### Colour and mono cameras

- Hold either stick click for one second. Passthrough switches between the Arcturus RGB colour camera and the Frame's mono cameras.
- The original dropped this in 0.1.1 and pointed to Valve's Arcturus Vision Camera app. That app has no shortcuts. Here the hold is back.
- Your choice is saved. It comes back each time you open passthrough.
- 0.1.0 used to set the camera again many times a second, all the time. Now it only does that while passthrough is showing. Other camera tools are left alone.

### Gestures only while passthrough is open

- The gestures only work while passthrough is showing. Open it as usual, with the headset's side button or the Steam button.
- The original acted on every double-press with the dashboard closed. Now, with passthrough off, the app does nothing with your clicks.
- A press you started before passthrough opened is ignored.
- A new option, `disable_while_game_running`, turns the gestures off while a VR game runs. It is off by default. Set it in `config.json`, or run `frame-passthrough-shortcuts --set disable-while-game-running on`.
- Edits to `config.json` apply within a second. No restart.

### Quieter in the background

- It adds nothing to your Steam library. It runs as an internal SteamVR app.
- The log used to fill with other apps' binding errors. Now it only lists its own, with a timestamp on every line.
- Uninstalling keeps your camera choice and your binding choice.
- It is one small native Arm64 program. It needs nothing beyond glibc 2.38, which SteamOS has.

### Install

- It installs and updates from Frametanium.
- It replaces the original in place. It keeps your camera choice, your bindings and SteamVR autostart.

## Download

- `framepassthrough-0.2.0-linux-arm64.tar.gz`
- `framepassthrough-0.2.0-linux-arm64.tar.gz.sha256`

## Install

**With Frametanium.** Add `simplysanatori/FramePassthrough`. Frametanium downloads the release and runs its `install.sh`. It updates it when I publish a new version.

**By hand.** Unpack the tarball into a folder and run `./install.sh` from it. It copies the app to `~/.local/share/FramePassthrough/app`, registers it with SteamVR and turns on autostart. There is nothing to launch. It starts with SteamVR.

If you had the original installed, you can install straight over it. Then remove the original's entry from your library, so it is not launched again by mistake. The [README](https://github.com/simplysanatori/FramePassthrough/blob/steam-frame/README.md) has the full steps, the settings and how to uninstall.

## Known issues

- SteamVR still sends the stick clicks to the game too. It does this for every overlay app, unless its experimental global action set priority setting is on. With passthrough off, my app ignores them. With passthrough open over a game, the game gets the click as well. `disable_while_game_running` stops my app reacting while a game runs.
- The gestures need the SteamVR dashboard closed.
- The camera switch uses SteamVR's internal camera controls. A SteamVR update can change them.
- Choosing the mono cameras does not power off the RGB camera.

## Reporting problems

Open an issue with what you did, what you expected and what happened. Attach `application.log` and `runtime.json` from `~/.config/frame-passthrough-shortcuts`.
