# AGENTS.md

This file holds the technical detail moved out of the README, for coding agents and developers.

The full internals (gate logic, passthrough detection, camera source, every `config.json` key, the command line, file locations, installer internals, troubleshooting and building) are in [docs/TECHNICAL.md](docs/TECHNICAL.md). Build instructions: [docs/TECHNICAL.md#building](docs/TECHNICAL.md#building). Upstream's README is kept unchanged as [docs/UPSTREAM-README.md](docs/UPSTREAM-README.md).

## Why the fork

I forked it because the original reacts to the stick gestures all the time, during games too. A game's own thumbstick hold or double-press switches passthrough by accident. FramePassthrough only reacts while passthrough is showing, in Home or in a SteamVR or OpenXR game. With passthrough off, a game's stick controls never trigger it.

I also restored the colour / mono camera switch that the original removed in 0.1.1.

## Status

Version 0.2.0. It runs on my Steam Frame.

Known issues:

- The gestures need the SteamVR dashboard closed.
- With passthrough open in a game, the game also gets the stick clicks. SteamVR does that for every overlay app.
- The camera switch uses SteamVR's internal camera controls. A SteamVR update can change them.
- Choosing the mono cameras does not power off the RGB camera.

## Install details

**With [Frametanium](https://github.com/Cogential/frametanium).** Add `simplysanatori/FramePassthrough`. It installs and updates the app.

It installs to `~/.local/share/FramePassthrough/app` and starts with SteamVR. It shows no window and adds nothing to your Steam library.

If you have the original, install over it. Your camera choice, bindings and autostart are kept. Then remove the original's entry from your library, because launching it registers the original again.

To remove: `~/.local/share/FramePassthrough/app/uninstall.sh`.

## Controls

Whenever passthrough is showing (side button or Steam button), in Home or in a game, with the dashboard closed, on either thumbstick:

| Stick click | Action |
| --- | --- |
| Double-press | Translucent / opaque passthrough |
| Hold for one second | Colour / mono cameras. The choice is saved |

## Settings

`~/.local/share/FramePassthrough/app/config.json`, reloaded within a second of an edit.

`disable_while_game_running` is `false` by default. Set it to `true`, or run `frame-passthrough-shortcuts --set disable-while-game-running on`, to turn the gestures off in games even with passthrough open.

The other keys, the command line and the file locations are in [docs/TECHNICAL.md](docs/TECHNICAL.md).

The README gives this command with the full program path, `~/.local/share/FramePassthrough/app/frame-passthrough-shortcuts`, from the Command line section of TECHNICAL.md.

## Build from source

On the Frame: `scripts/make-release.sh`. It builds, runs the tests and writes the release tarball to `dist/`. See [docs/TECHNICAL.md](docs/TECHNICAL.md).

