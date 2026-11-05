# FramePassthrough

Passthrough shortcuts on the thumbsticks of the Valve Steam Frame.

FramePassthrough is a fork of KominoVR's [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts) (v0.1.1). It is a small SteamVR background app that runs on the Frame itself. Double-press a thumbstick to switch between translucent and opaque passthrough. Hold one for a second to switch between the Arcturus RGB colour camera and the Frame's own monochrome cameras.

The gestures only act while passthrough is showing. While passthrough is off, the app does nothing with your controllers.

## Status

Version 0.2.0, the first release of this fork. It is upstream 0.1.1 with 0.1.0's colour / mono camera switching brought back, plus a few changes for living with it every day. It runs on my Steam Frame.

**Compared with the original.** Upstream dropped camera switching in 0.1.1 and points you to Valve's [Arcturus Vision Camera](https://store.steampowered.com/app/4103250/Arcturus_Vision_Camera/) app instead. That app has no controller shortcuts. Here the one-second hold switches cameras again, and the choice is saved. The original's gestures acted whenever the SteamVR dashboard was closed, with or without passthrough. Here they only act while passthrough is showing, and an option turns them off while a game is running. 0.1.0 also re-applied the saved camera many times a second, all the time. Here the camera is left alone while passthrough is off, so other camera tools are not overridden.

Known issues:

- SteamVR still sends the stick clicks to the game too. It does that for every overlay app, unless SteamVR's experimental global action set priority setting is on. With passthrough off, FramePassthrough does nothing with them. With passthrough showing over a game, a double-press or a hold also reaches the game. Turn on `disable_while_game_running` if that gets in the way.
- The gestures need the SteamVR dashboard closed, as in the original.
- Camera switching uses SteamVR's internal camera controls. A SteamVR update can change them.
- Choosing the monochrome cameras does not power off the Arcturus RGB camera. It only changes which camera passthrough shows.

## What this adds over Frame Passthrough Shortcuts 0.1.1

- The colour / mono camera switch from 0.1.0: hold either stick click for a second. The choice is saved in `camera-preference.json` and set again each time passthrough opens.
- The gestures only act while passthrough is showing. A gesture you started before passthrough opened is ignored.
- `disable_while_game_running` in `config.json` (off by default). While a VR game is running, the gestures do nothing, even with passthrough open. `--set disable-while-game-running on|off` changes it from a terminal.
- `config.json` is reloaded within a second of an edit. No restart.
- `passthrough_signal` in `config.json`, for choosing how the app tells that passthrough is showing (see [Settings](#settings)).
- The saved camera is only checked and set while passthrough is showing.
- It is registered as an internal SteamVR app, so it adds nothing to your Steam library.
- Its log only reports its own binding errors, not every other app's, and every line has a timestamp.
- Uninstalling keeps your camera choice and the log, and leaves SteamVR's settings file alone, so your binding choice survives a reinstall.
- `--install` works with SteamVR closed. The app then starts with SteamVR next time.
- A release tarball with `install.sh` and `uninstall.sh`, so Frametanium can install and update it.
- It uses the original's app key (`local.frame.passthrough.shortcuts`) and settings folder (`~/.config/frame-passthrough-shortcuts`). It replaces the original in place and keeps your camera choice, your binding choice and SteamVR autostart.

## Requirements

- A Steam Frame with SteamVR. SteamVR must have run at least once.
- The Arcturus RGB camera, for colour passthrough. It is optional. Without it, the double-press works and the hold does nothing.
- A terminal on the Frame for a manual install: Konsole in desktop mode, or SSH. Frametanium needs none.

Nothing else. The app is one small native Arm64 binary that needs only glibc 2.38 or newer, which SteamOS has.

## Install

### With Frametanium

If you use [Frametanium](https://github.com/Cogential/frametanium), add `simplysanatori/FramePassthrough` there. It downloads the release, runs its `install.sh` and updates it when I publish a new version.

### By hand

Download `framepassthrough-0.2.0-linux-arm64.tar.gz` and its `.sha256` file from the [Releases](https://github.com/simplysanatori/FramePassthrough/releases) page. In the folder you downloaded them to:

```sh
sha256sum -c framepassthrough-0.2.0-linux-arm64.tar.gz.sha256
mkdir framepassthrough-0.2.0
tar -xzf framepassthrough-0.2.0-linux-arm64.tar.gz -C framepassthrough-0.2.0
./framepassthrough-0.2.0/install.sh
```

The installer:

- copies the program to `~/.local/share/FramePassthrough/app`;
- keeps the `config.json` of an earlier install, or takes over the original app's `config.json` from `~/devkit-game/Frame_Passthrough_Shortcuts` on the first install;
- stops any running copy of the shortcuts app, the original or this one, so only one copy runs;
- registers the app with SteamVR and turns on autostart. If SteamVR is running, the app starts at once. If not, it starts with SteamVR next time.

It never touches `camera-preference.json` or your SteamVR binding choice. Running it again is safe. The unpacked folder can be deleted afterwards.

### Coming from the original app

Install as above. You do not need to uninstall the original first. FramePassthrough takes its place under the same app key and keeps its settings. The original's folder in `~/devkit-game/Frame_Passthrough_Shortcuts` stays where it is but no longer runs.

Remove the original's **Frame Passthrough Shortcuts** entry from your library, or from FrameDrop if you installed it there. Launching that entry registers the original app again. Do not run the original's `--uninstall` either. It would remove this app's registration and delete your camera choice. If any of that happens, run `install.sh` again.

### Updating and removing

To update by hand, unpack the new release and run its `install.sh`. Your `config.json` and camera choice are kept.

To remove FramePassthrough:

```sh
~/.local/share/FramePassthrough/app/uninstall.sh
```

It stops the app, removes it from SteamVR and turns off autostart, and deletes `~/.local/share/FramePassthrough/app`. It keeps `~/.config/frame-passthrough-shortcuts` (your camera choice and the log) for a later reinstall. Delete that folder too if you want everything gone.

## First run

There is nothing to launch. The app starts in the background with SteamVR and shows no window.

1. Put the headset on.
2. Open passthrough the usual way: double-press the headset's side button, or press the Steam button on either controller.
3. With the SteamVR dashboard closed, double-press either thumbstick. Passthrough switches between translucent and opaque.
4. With the Arcturus RGB camera fitted, hold either thumbstick down for a second. Passthrough switches between colour and mono.

To check that it is running, from a terminal:

```sh
~/.local/share/FramePassthrough/app/frame-passthrough-shortcuts --status
```

It prints `"installed":true`, `"autolaunch":true`, `"running":true` and `"ready":true` when all is well.

## Controls

| Either thumbstick click | What it does |
| --- | --- |
| Double-press | Switch between translucent and opaque passthrough |
| Hold for one second | Switch between the Arcturus RGB colour camera and the monochrome cameras. The choice is saved |

- The gestures only act while passthrough is showing, and with the SteamVR dashboard closed.
- A gesture you started before passthrough opened is ignored. Let go and start again.
- With `disable_while_game_running` on, they also do nothing while a VR game runs.
- The game still receives the clicks (see [Status](#status)).

You can remap them in **SteamVR controller bindings → Frame Passthrough Shortcuts**. There are also separate actions for translucent, opaque, RGB and monochrome, unbound by default.

To open that binding page from a terminal, stop the app first, then start it again afterwards:

```sh
APP=~/.local/share/FramePassthrough/app/frame-passthrough-shortcuts
$APP --stop
$APP --bindings
$APP --install
```

## Settings

The settings are in `~/.local/share/FramePassthrough/app/config.json`. The app reloads the file within a second of a change. A broken edit is logged and the previous settings stay in force. A copy of the defaults sits next to it as `config.default.json`.

| Key | Default | What it does |
| --- | --- | --- |
| `disable_while_game_running` | `false` | `true` turns the gestures off while a VR game is running, even with passthrough showing. SteamVR Home and SteamVR's own tools do not count as games |
| `passthrough_signal` | `"auto"` | How the app tells that passthrough is showing: `auto`, `setting`, `events`, `steamvr_log` or `off`. Leave it on `auto`. `off` lets the gestures act at any time, as in the original |
| `poll_interval_ms` | `20` | How often the app reads the controllers, in milliseconds (10 to 1000) |
| `debounce_ms` | `250` | The shortest time between two firings of the same action, in milliseconds (0 to 5000) |

`passthrough_signal` is not in the default file. Add it only if you need to change it. An `rgb` block from an older config file is accepted and ignored. On the Frame the camera switch is always set up the same way.

You can change `disable_while_game_running` without an editor:

```sh
~/.local/share/FramePassthrough/app/frame-passthrough-shortcuts --set disable-while-game-running on
```

`off` turns it back off. The running app picks it up within a second.

Other commands of the same program:

| Command | What it does |
| --- | --- |
| `--status` | Prints whether the app is registered, set to autostart and running, as JSON |
| `--install` | Registers the app with SteamVR, turns on autostart and starts it. This is also what runs with no command |
| `--stop` | Stops the app until SteamVR starts again. Autostart stays on |
| `--uninstall` | Removes the SteamVR registration and autostart. Keeps your preferences |
| `--bindings` | Opens the SteamVR binding page for the app. Run `--stop` first |
| `--doctor` | Reads the passthrough style and the camera source and reports them. Changes nothing. Run `--stop` first, with passthrough open |
| `--action NAME` | Runs one action once: `toggle-style`, `translucent`, `opaque`, `toggle-rgb`, `rgb` or `monochrome`. Run `--stop` first |
| `--self-test` | Switches the passthrough style through translucent and opaque, then puts it back. Run `--stop` first |
| `--validate` | Checks `config.json` and the bundled action files. Needs no SteamVR |
| `--config PATH` | Uses another config file |

After `--stop` and any of the one-shot commands, run `--install` to start the app again.

## Performance

FramePassthrough draws nothing. It creates no overlay surface and no window. It wakes every 20 ms to read the controller actions, and twice a second to check the passthrough state and the running app. It only reads the camera state while passthrough is showing.

## Files

| What | Where |
| --- | --- |
| Program, `config.json`, bindings | `~/.local/share/FramePassthrough/app` |
| Camera choice | `~/.config/frame-passthrough-shortcuts/camera-preference.json` |
| Log | `~/.config/frame-passthrough-shortcuts/application.log` |
| Live state (gate, passthrough, running app) | `~/.config/frame-passthrough-shortcuts/runtime.json` |
| SteamVR manifest and launch record | `~/.config/frame-passthrough-shortcuts/app.vrmanifest`, `launch.json` |
| SteamVR autostart | `~/.config/openvr/config/vrappconfig/local.frame.passthrough.shortcuts.vrappconfig` |
| SteamVR app list, only when installed with SteamVR closed | `~/.config/openvr/config/appconfig.json` |

The installer and the app change nothing else on the Frame. The binding choice lives in SteamVR's own settings, which the app does not edit.

## Troubleshooting

- **A double-press does nothing.** Passthrough must be showing and the SteamVR dashboard closed. Run `--status` to see that the app is running. `application.log` has a `Shortcuts enabled` or `Shortcuts disabled` line with the reason each time that changes.
- **`application.log` says "No active bindings".** Choose the Frame Passthrough Shortcuts binding in SteamVR's controller bindings (see [Controls](#controls)).
- **The hold does nothing.** The log says `Arcturus RGB camera not detected` when the RGB camera is not connected. The monochrome cameras are then the only ones.
- **The gestures do nothing in a game.** Check `disable_while_game_running` in `config.json`. `runtime.json` shows the gate's current reason.
- **A game reacts to the stick click as well.** That is SteamVR sending the click to both. See [Status](#status).
- **The original app came back.** Launching its old library entry registers it again. Run `install.sh` again and remove the old entry.
- **`install.sh` says another copy is still running.** Something holds `~/.config/frame-passthrough-shortcuts/instance.lock`. Restart SteamVR, or the Frame, and run it again.
- **`install.sh` says it ignored a `config.json`.** The old file could not be read, so the defaults are used. Edit `~/.local/share/FramePassthrough/app/config.json` if you had changed anything.
- **`--install` says SteamVR has never run for this user.** Start SteamVR once, then install again.

## Reporting a problem

Open an [issue](https://github.com/simplysanatori/FramePassthrough/issues) with what you did, what you expected and what happened. Attach:

- `~/.config/frame-passthrough-shortcuts/application.log`;
- `~/.config/frame-passthrough-shortcuts/runtime.json`;
- the output of `~/.local/share/FramePassthrough/app/frame-passthrough-shortcuts --status`.

The log contains paths with your user name, and the keys of the apps you ran. Read it before you attach it.

## Build from source

On the Frame, or any Linux Arm64 machine, with CMake, Ninja, a C++17 compiler and `strip`:

```sh
scripts/make-release.sh
```

It builds a release, runs the unit tests with `ctest` and writes `dist/framepassthrough-<version>-linux-arm64.tar.gz` and its `.sha256`. The OpenVR loader and libstdc++ are linked in statically. For a plain build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

That also builds `fp-probe` from `tools/probe`, a read-only SteamVR state logger for development. `-DFPS_BUILD_PROBE=OFF` skips it. It is not part of the release.

Upstream's Node packaging script and Windows cross-build script are still in `scripts/`. Their notes are in [docs/UPSTREAM-README.md](docs/UPSTREAM-README.md).

## Support

FramePassthrough exists because of KominoVR's [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts). If you want to support someone, support them first: star and follow it, report what you find there, and send improvements upstream.

If you would like more Steam Frame ports, you can also [buy me a coffee](https://ko-fi.com/sana748653). It pays for the Claude API credits these ports are built with, spent as carefully as I can.

## Credits

- Built by [simplysanatori](https://github.com/simplysanatori).
- [Frame Passthrough Shortcuts](https://github.com/KominoVR/frame-passthrough-shortcuts) by [KominoVR](https://github.com/KominoVR), the original app. The passthrough style shortcuts, the camera switching, the SteamVR registration and the default bindings are KominoVR's work. This fork is built on it.
- The [OpenVR SDK](https://github.com/ValveSoftware/openvr) by Valve, whose loader is compiled in, and JsonCpp by Baptiste Lepilleur, which the loader uses.
- [JSON for Modern C++](https://github.com/nlohmann/json) by Niels Lohmann.
- [Zig](https://ziglang.org/) and LLVM's libc++, libc++abi and libunwind, used by upstream's cross-build.
- [Frametanium](https://github.com/Cogential/frametanium), which installs and updates it.
- SteamVR, the Steam Frame and the Arcturus camera are Valve's.

[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) has the full list with licences.

This is an unofficial project. It is not affiliated with or endorsed by Valve or the upstream author.

## Licence

MIT, as upstream: see [LICENSE](LICENSE). Upstream's copyright line stays as it is: Copyright (c) 2026 Frame Passthrough Shortcuts contributors. The fork's changes are Copyright (c) 2026 simplysanatori, under the same licence. Third-party parts keep their own licences: see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and the `third_party/` folder.

Upstream's own README is kept as [docs/UPSTREAM-README.md](docs/UPSTREAM-README.md).

## How this was made

I wanted my passthrough shortcuts back with colour switching, and I wanted them to stay out of my games. So I started from KominoVR's Frame Passthrough Shortcuts 0.1.1, restored 0.1.0's camera switch, made the gestures wait for passthrough, added the game option and packaged it for Frametanium.

This is vibe coded, with manual review throughout. I wrote it with Claude Code running on the Frame itself.
