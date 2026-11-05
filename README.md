# FramePassthrough

Passthrough shortcuts on the Steam Frame's thumbsticks. A fork of KominoVR's [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts) 0.1.1.

I forked it because the original stays active during games. A game control that uses a thumbstick double-press or hold can switch your passthrough by accident. FramePassthrough checks whether a SteamVR or OpenXR game is running and whether passthrough is showing. It only acts in Home with passthrough showing, so it never conflicts with a game's own stick controls.

I also restored the colour / mono camera switch that the original removed in 0.1.1.

## Status

Version 0.2.0. It runs on my Steam Frame.

Known issues:

- The gestures need the SteamVR dashboard closed.
- If you let the gestures act in games, the game still gets the stick clicks too. SteamVR does that for every overlay app.
- The camera switch uses SteamVR's internal camera controls. A SteamVR update can change them.
- Choosing the mono cameras does not power off the RGB camera.

## Requirements

- A Steam Frame with SteamVR, run at least once.
- The Arcturus RGB camera, for the colour camera. Without it the double-press still works.

## Install

**With [Frametanium](https://github.com/Cogential/frametanium).** Add `simplysanatori/FramePassthrough`. It installs and updates the app.

**By hand.** Download `framepassthrough-0.2.0-linux-arm64.tar.gz` from [Releases](https://github.com/simplysanatori/FramePassthrough/releases), then:

```sh
mkdir fp && tar -xzf framepassthrough-0.2.0-linux-arm64.tar.gz -C fp
./fp/install.sh
```

It installs to `~/.local/share/FramePassthrough/app` and starts with SteamVR. It shows no window and adds nothing to your Steam library.

If you have the original, install over it. Your camera choice, bindings and autostart are kept. Then remove the original's entry from your library, because launching it registers the original again.

To remove: `~/.local/share/FramePassthrough/app/uninstall.sh`.

## Controls

With passthrough showing (side button or Steam button) and the dashboard closed, on either thumbstick:

| Stick click | Action |
| --- | --- |
| Double-press | Translucent / opaque passthrough |
| Hold for one second | Colour / mono cameras. The choice is saved |

Remap them in **SteamVR controller bindings → Frame Passthrough Shortcuts**.

## Settings

`~/.local/share/FramePassthrough/app/config.json`, reloaded within a second of an edit.

`disable_while_game_running` is `true` by default. Set it to `false`, or run `frame-passthrough-shortcuts --set disable-while-game-running off`, to let the gestures act in games while passthrough is open.

The other keys, the command line and the file locations are in [docs/TECHNICAL.md](docs/TECHNICAL.md).

## Reporting a problem

Open an [issue](https://github.com/simplysanatori/FramePassthrough/issues) with what you did and what happened. Attach `~/.config/frame-passthrough-shortcuts/application.log`. It contains your user name in paths, so read it first.

## Build from source

On the Frame: `scripts/make-release.sh`. It builds, runs the tests and writes the release tarball to `dist/`. See [docs/TECHNICAL.md](docs/TECHNICAL.md).

## Support

FramePassthrough exists because of KominoVR's [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts). If you want to support someone, support them first: star and follow it, report what you find there, and send improvements upstream.

If you would like more Steam Frame ports, you can also [buy me a coffee](https://ko-fi.com/sana748653). It pays for the Claude API credits these ports are built with, spent as carefully as I can.

## Credits

- [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts) by [KominoVR](https://github.com/KominoVR), the original app this is built on.
- Fork by [simplysanatori](https://github.com/simplysanatori).
- Valve's [OpenVR SDK](https://github.com/ValveSoftware/openvr) and Niels Lohmann's [JSON for Modern C++](https://github.com/nlohmann/json). The full list is in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

This is an unofficial project, not affiliated with Valve or the upstream author.

## Licence

MIT, as upstream: see [LICENSE](LICENSE) (Copyright (c) 2026 Frame Passthrough Shortcuts contributors). The fork's changes are Copyright (c) 2026 simplysanatori, under the same licence. Upstream's README is kept as [docs/UPSTREAM-README.md](docs/UPSTREAM-README.md).

## How this was made

This is vibe coded, with manual review throughout. I wrote it with Claude Code running on the Frame itself.
