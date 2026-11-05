# FramePassthrough

Passthrough shortcuts on the Steam Frame's thumbsticks. A fork of KominoVR's [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts) 0.1.1.

## Features

- Double-press a stick for translucent / opaque passthrough
- Hold a stick for colour / mono cameras
- Only reacts while passthrough is showing
- Remembers your camera choice
- Nothing added to your Steam library

## How it works

It runs on the Frame itself, in the background, and starts with SteamVR. It listens for your thumbstick clicks through SteamVR. Then it asks SteamVR to switch the passthrough style or camera.

## Requirements

- A Steam Frame with SteamVR, run at least once
- The Arcturus RGB camera, for the colour camera (double-press works without it)

## Install

1. Download `framepassthrough-0.2.0-linux-arm64.tar.gz` from [Releases](https://github.com/simplysanatori/FramePassthrough/releases).
2. Unpack and install it:

   ```sh
   mkdir fp && tar -xzf framepassthrough-0.2.0-linux-arm64.tar.gz -C fp
   ./fp/install.sh
   ```

With [Frametanium](https://github.com/Cogential/frametanium): add `simplysanatori/FramePassthrough`. It also updates the app.

Have the original? Install over it, then remove its Steam library entry.

## Controls

With passthrough showing (side button or Steam button) and the dashboard closed, on either thumbstick:

| Stick click | Action |
| --- | --- |
| Double-press | Translucent / opaque passthrough |
| Hold for one second | Colour / mono cameras |

Remap them in **SteamVR controller bindings → Frame Passthrough Shortcuts**.

## Uninstall

```sh
~/.local/share/FramePassthrough/app/uninstall.sh
```

## Troubleshooting

- Gestures do nothing: open passthrough and close the SteamVR dashboard.
- Still nothing: choose the **Frame Passthrough Shortcuts** binding in SteamVR controller bindings.
- Gestures act in games with passthrough open: run `~/.local/share/FramePassthrough/app/frame-passthrough-shortcuts --set disable-while-game-running on`.
- Hold does nothing: you need the Arcturus RGB camera.

More fixes: [docs/TECHNICAL.md](docs/TECHNICAL.md#troubleshooting).

## Reporting a problem

Open an [issue](https://github.com/simplysanatori/FramePassthrough/issues) saying what you did and what happened. Attach `~/.config/frame-passthrough-shortcuts/application.log`. It contains your user name. Read it before sharing.

## More

- [Settings, command line and files](docs/TECHNICAL.md)
- [AGENTS.md](AGENTS.md): for developers and AI agents
- [Building from source](docs/TECHNICAL.md#building)
- [Changelog](CHANGELOG.md)

## Support

Support KominoVR's [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts) first: star and follow it, report bugs there, send improvements upstream.

For more Steam Frame ports, you can [buy me a coffee](https://ko-fi.com/sana748653). It pays for the Claude API credits.

## Credits

- [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts) by [KominoVR](https://github.com/KominoVR): the original app
- Fork by [simplysanatori](https://github.com/simplysanatori)
- Valve's [OpenVR SDK](https://github.com/ValveSoftware/openvr) and Niels Lohmann's [JSON for Modern C++](https://github.com/nlohmann/json); full list in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

Unofficial, not affiliated with Valve or the upstream author.

## Licence

MIT, same as upstream. See [LICENSE](LICENSE).

## How this was made

I made this with partial assistance from Claude.
