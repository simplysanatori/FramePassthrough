# Frame Passthrough Shortcuts

Change Steam Frame passthrough modes from your controllers. Switch between
translucent passthrough, which shows your surroundings alongside immersive VR content,
and opaque passthrough, which only shows your surroundings.

It works with the Arcturus RGB camera! 
You can switch between RGB and monochrome cameras (Day and Night vision 😎).

[![Install with FrameDrop](images/framedrop-button.svg)](https://framedropvr.com/install?manifest=https%3A%2F%2Fgithub.com%2FKominoVR%2Fframe-passthrough-shortcuts%2Freleases%2Flatest%2Fdownload%2Fframe-passthrough-shortcuts.framedrop.json)

Download the [Linux ARM64 package](https://github.com/KominoVR/frame-passthrough-shortcuts/releases/download/v0.1.0/frame-passthrough-shortcuts-0.1.0-linux-arm64.zip) if you prefer to install manually.

## Controls

| Gesture on either thumbstick | Action |
| --- | --- |
| Double-press | Toggle translucent / opaque passthrough |
| Hold for one second | Toggle Arcturus RGB camera / built-in monochrome cameras |

**These only work with the SteamVR dashboard closed!**

**Open passthrough first with its usual system button (double-press
the headset side button or the Steam button on either controller)**

All actions can be changed in **SteamVR controller bindings → Frame Passthrough
Shortcuts**, including separate translucent, opaque, RGB, and monochrome
actions.

## Compatibility

- Runs locally on Steam Frame.
- Arcturus RGB camera is optional.
- Shortcuts require the SteamVR dashboard to be closed.
- Selecting monochrome only changes the camera source; it does not power
  off the Arcturus RGB camera.
- Camera switching uses SteamVR’s internal camera controls. Updates may affect
  compatibility. (Please report any issues)

## Manual installation

Copy the Linux ARM64 ZIP to the Frame, connect over SSH, and run:

```sh
unzip -oq ~/frame-passthrough-shortcuts-0.1.0-linux-arm64.zip -d ~/devkit-game/Frame_Passthrough_Shortcuts
~/devkit-game/Frame_Passthrough_Shortcuts/frame-passthrough-shortcuts --install
```

`--install` registers the OpenVR manifest and enables SteamVR autostart. To
remove the registration, package files, bindings, and app settings:

```sh
~/devkit-game/Frame_Passthrough_Shortcuts/frame-passthrough-shortcuts --uninstall
```

## Build

On Linux ARM64, with CMake and a C++17 compiler installed:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
node scripts/package.mjs --build-dir build --platform linux-arm64
```

Packaging requires Node.js and writes ZIP archives and SHA-256 checksums to
`dist/`.

[MIT licensed](LICENSE).

AI disclosure: Developed with assistance from coding agents.
