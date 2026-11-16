# FramePassthrough

Passthrough shortcuts on the Steam Frame's thumbsticks. A fork of KominoVR's [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts) 0.1.1.

## Features

- Double-press a stick for translucent / opaque passthrough
- Hold a stick for colour / mono cameras
- Only reacts while passthrough is showing
- Remembers your camera choice
- Nothing added to your Steam library

## How it works

It runs in the background on the Frame and starts with SteamVR. It reads your thumbstick clicks and asks SteamVR to switch the passthrough style or camera.

## Requirements

- A Steam Frame with SteamVR, run at least once
- For the colour camera only: the Arcturus RGB camera

## Install

1. Download `framepassthrough-0.2.0-linux-arm64.tar.gz` from [Releases](https://github.com/simplysanatori/FramePassthrough/releases).
2. Unpack and install it from the download folder:

   ```sh
   mkdir -p fp && tar -xzf framepassthrough-0.2.0-linux-arm64.tar.gz -C fp
   ./fp/install.sh
   ```

With [Frametanium](https://github.com/Cogential/frametanium): add `simplysanatori/FramePassthrough`.

Have the original? Install over it, then remove its Steam library entry.

## Controls

With passthrough showing (side button or Steam button) and the dashboard closed, on either thumbstick:

| Stick click | Action |
| --- | --- |
| Double-press | Translucent / opaque passthrough |
| Hold for one second | Colour / mono cameras |

Remap them in **SteamVR controller bindings → Frame Passthrough Shortcuts**.

## Update

Repeat the install steps with the new release. Frametanium updates it for you.

## Uninstall

```sh
~/.local/share/FramePassthrough/app/uninstall.sh
```

## Troubleshooting

- Gestures do nothing: open passthrough and close the SteamVR dashboard.
- Still nothing: choose the **Frame Passthrough Shortcuts** binding in SteamVR controller bindings.
- To turn the gestures off in games: run `~/.local/share/FramePassthrough/app/frame-passthrough-shortcuts --set disable-while-game-running on`.

More fixes: [docs/TECHNICAL.md](docs/TECHNICAL.md#troubleshooting).

## Reporting a problem

Open an [issue](https://github.com/simplysanatori/FramePassthrough/issues) saying what you did and what happened. Attach `~/.config/frame-passthrough-shortcuts/application.log`. It contains your user name, so read it before sharing.

## More

- [docs/TECHNICAL.md](docs/TECHNICAL.md): settings, command line, files and [building](docs/TECHNICAL.md#building)
- [AGENTS.md](AGENTS.md): for developers and AI agents
- [Changelog](CHANGELOG.md)

## Support

Star KominoVR's [original](https://github.com/KominoVR/frame-passthrough-shortcuts) and send improvements upstream.

For more Steam Frame ports, [buy me a coffee](https://ko-fi.com/sana748653).

## Credits

- [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts) by [KominoVR](https://github.com/KominoVR): the original app
- [simplysanatori](https://github.com/simplysanatori): this fork
- [OpenVR SDK](https://github.com/ValveSoftware/openvr) by Valve: talks to SteamVR
- [JSON for Modern C++](https://github.com/nlohmann/json) by Niels Lohmann: JSON parsing

Unofficial, not affiliated with Valve or KominoVR.

## Licence

MIT, same as upstream: [LICENSE](LICENSE). Bundled libraries: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## How this was made

I started this project and used AI (Claude) to speed up development. I write, review and fix code by hand all the way through, keep the AI's work checked and reined in, and aim to keep the tool as lightweight as possible. I test it on my own Steam Frame.
