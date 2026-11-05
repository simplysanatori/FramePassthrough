# FramePassthrough technical notes

## When the gestures act

The app reads its SteamVR actions only while its gate is open. The gate is open when passthrough is showing and, with `disable_while_game_running` on (the default), no game is running. While the gate is closed the app releases its action set and ignores the sticks. A gesture that began before the gate opened is ignored: presses are rejected for 250 ms (double-press) or 1050 ms (hold) after it opens.

A game is any SteamVR scene application other than SteamVR Home, the stock environments and SteamVR's own components. OpenXR titles count. The Frame's home view runs no scene application.

How passthrough is detected (`passthrough_signal` `auto`), as seen on SteamVR 2.19.1:

- **No game running.** Opening or closing passthrough flips SteamVR's `camera.enableConstructRoomView` setting and sends `VREvent_CameraSettingsHaveChanged`. The app reads that setting on the event and every 500 ms. The dashboard does not change it.
- **A game running.** SteamVR sends `VREvent_RoomViewShown` and `VREvent_RoomViewHidden`. Passthrough opened from the dashboard sends no event, but XRService logs `currentRoomViewRenderingMode` (0 = off) to `vrserver.txt`. The newer of the two wins. When a game starts, passthrough counts as off until one of them says otherwise.
- If the app has nothing to go on yet, it lets the gestures act rather than go dead.

SteamVR still delivers the stick clicks to the game as well, as it does for every overlay app, unless its experimental `steamvr.globalActionSetPriority` setting is on.

## Camera source

The saved choice is in `~/.config/frame-passthrough-shortcuts/camera-preference.json`. It is checked only while passthrough is showing: every poll for five seconds after passthrough appears, then every two seconds. It is written only when the source SteamVR reports differs from it, because each write makes the compositor restart the camera feed. The RGB camera is found by its sensor in `/sys/class/video4linux`.

## config.json

`~/.local/share/FramePassthrough/app/config.json`. Reloaded within a second of an edit; a broken edit is logged and the previous settings stay. `config.default.json` next to it holds the defaults.

| Key | Default | What it does |
| --- | --- | --- |
| `disable_while_game_running` | `true` | Ignore the gestures while a game runs, even with passthrough showing |
| `passthrough_signal` | `"auto"` | `auto`, `setting`, `events`, `steamvr_log` or `off`. `off` lets the gestures act at any time, as in the original |
| `poll_interval_ms` | `20` | Controller read interval, 10 to 1000 ms |
| `debounce_ms` | `250` | Shortest time between two firings of one action, 0 to 5000 ms |

A config file without a key gets its default. An `rgb` block from an older config file is accepted and ignored.

## Command line

`~/.local/share/FramePassthrough/app/frame-passthrough-shortcuts`:

| Command | What it does |
| --- | --- |
| `--status` | Registration, autostart and running state as JSON |
| `--install` | Register with SteamVR, turn on autostart and start. The default with no command. Works with SteamVR closed: the app then starts with SteamVR next time |
| `--stop` | Stop until SteamVR starts again. Autostart stays on |
| `--uninstall` | Remove the SteamVR registration and autostart. Keeps preferences |
| `--set disable-while-game-running on\|off` | Edit the option in the config the running app uses |
| `--bindings` | Open the SteamVR binding page. Run `--stop` first |
| `--doctor` | Report the passthrough style and camera source. Run `--stop` first, with passthrough open |
| `--action NAME` | Run `toggle-style`, `translucent`, `opaque`, `toggle-rgb`, `rgb` or `monochrome` once. Run `--stop` first |
| `--self-test` | Cycle the passthrough style and put it back. Run `--stop` first |
| `--validate` | Check `config.json` and the action files without SteamVR |
| `--config PATH` | Use another config file |

After `--stop` and a one-shot command, run `--install` to start the app again.

## Files

| What | Where |
| --- | --- |
| Program, `config.json`, bindings | `~/.local/share/FramePassthrough/app` |
| Camera choice, `application.log`, `runtime.json` (live gate state), `app.vrmanifest`, `launch.json`, `instance.lock` | `~/.config/frame-passthrough-shortcuts` |
| SteamVR autostart | `~/.config/openvr/config/vrappconfig/local.frame.passthrough.shortcuts.vrappconfig` |
| SteamVR app list, only when installed with SteamVR closed | `~/.config/openvr/config/appconfig.json` |

The app key is `local.frame.passthrough.shortcuts`, the same as the original's. It is registered as an internal SteamVR app, so SteamVR adds no library entry. The binding choice lives in SteamVR's own settings, which the app does not edit.

## Install and uninstall

`install.sh` copies the program to `~/.local/share/FramePassthrough/app`. It keeps that folder's `config.json`, or on a first install takes over the original's from `~/devkit-game/Frame_Passthrough_Shortcuts`. It stops any running copy, old or new, then runs `--install`. It never touches `camera-preference.json` or the binding choice. `FRAMEPASSTHROUGH_APP=DIR` puts a trial install elsewhere.

The original's folder stays but no longer runs. Do not run the original's `--uninstall`: it shares the app key, so it removes this app's registration and deletes `~/.config/frame-passthrough-shortcuts`. If the original gets registered again, run `install.sh` again.

`uninstall.sh` runs `--uninstall`, stops anything left running from the program folder and deletes it. `~/.config/frame-passthrough-shortcuts` is kept.

## Troubleshooting

- `application.log` has a `Shortcuts enabled` or `Shortcuts disabled` line with the reason each time the gate changes. `runtime.json` shows the current state.
- `No active bindings` in the log: choose the Frame Passthrough Shortcuts binding in SteamVR's controller bindings.
- `Arcturus RGB camera not detected`: the hold has nothing to switch to.
- `install.sh` says another copy is still running: something holds `instance.lock`. Restart SteamVR and run it again.
- `--install` says SteamVR has never run for this user: start SteamVR once.

## Building

`scripts/make-release.sh [build-dir]` builds natively on a Linux Arm64 machine with CMake, Ninja, a C++17 compiler and `strip`. It waits while `~/.cache/headset-in-use` exists, runs the unit tests, refuses a binary that needs a glibc newer than `MAX_GLIBC` (default 2.38), and writes `dist/framepassthrough-<version>-linux-arm64.tar.gz` and its `.sha256`. The OpenVR loader and libstdc++ are linked statically; the binary needs only glibc.

A plain build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

This also builds `fp-probe` (`tools/probe`), a read-only SteamVR state logger for development. `-DFPS_BUILD_PROBE=OFF` skips it. Upstream's Node packaging and Windows cross-build scripts are still in `scripts/`; see [UPSTREAM-README.md](UPSTREAM-README.md).
