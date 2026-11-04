**Colour/black-and-white passthrough switching has been removed. Use the [official Arcturus camera control application](https://store.steampowered.com/app/4103250/Arcturus_Vision_Camera/) instead.**

# Frame Passthrough Shortcuts

Change Steam Frame passthrough modes from your controllers. Switch between
translucent passthrough, which shows your surroundings alongside immersive VR content,
and opaque passthrough, which only shows your surroundings.

[Watch the showcase on X](https://x.com/_komino/status/2103490940476309863?s=20).

<br>
<br>

<a href="https://framedropvr.com/install?manifest=https%3A%2F%2Fgithub.com%2FKominoVR%2Fframe-passthrough-shortcuts%2Freleases%2Flatest%2Fdownload%2Fframe-passthrough-shortcuts.framedrop.json" style="display:inline-flex;align-items:center;gap:10px;padding:8px 14px 8px 10px;color:#e6b54a;background:#080809;border:1px solid #e6b54a;text-decoration:none;font:600 13px/1.2 'JetBrains Mono', ui-monospace, monospace;letter-spacing:0.06em;text-transform:uppercase;">
  <svg xmlns="http://www.w3.org/2000/svg" viewBox="24 11 40 40" width="28" height="28" fill="none" aria-hidden="true">
    <path d="M58 23V48H26V42" stroke="#e6b54a" stroke-width="2.5" stroke-linejoin="miter"/>
    <path d="M56 16H41" stroke="#c41e3a" stroke-width="2.5"/>
    <path d="M41 16H26V20" stroke="#cb3c3d" stroke-width="2.5" stroke-linejoin="miter"/>
    <path d="M26 20V26" stroke="#d25d41" stroke-width="2.5"/>
    <path d="M26 26V32" stroke="#d97c44" stroke-width="2.5"/>
    <path d="M26 32V37" stroke="#df9747" stroke-width="2.5"/>
    <path d="M26 37V42" stroke="#e3a949" stroke-width="2.5"/>
    <rect x="55" y="13" width="6" height="6" fill="#c41e3a"/>
  </svg>
  Install with FrameDrop
</a>

After installing with FrameDrop, launch **Frame Passthrough Shortcuts** once from
**Library → Non-Steam** on the headset to enable SteamVR autostart.

Download the [Linux ARM64 package](https://github.com/KominoVR/frame-passthrough-shortcuts/releases/download/v0.1.1/frame-passthrough-shortcuts-0.1.1-linux-arm64.zip) if you prefer to install manually.

[View the GitHub repository](https://github.com/KominoVR/frame-passthrough-shortcuts).

<br>

# Controls

| Gesture on either thumbstick | Action |
| --- | --- |
| Double-press | Toggle translucent / opaque passthrough |

**These only work with the SteamVR dashboard closed!**

**Open passthrough first with its usual system button (double-press
the headset side button or the Steam button on either controller)**

All actions can be changed in **SteamVR controller bindings → Frame Passthrough
Shortcuts**, including separate translucent and opaque actions.

<br>

# Compatibility

- Runs locally on Steam Frame.
- Shortcuts require the SteamVR dashboard to be closed.

<br>

# Manual installation

Copy the Linux ARM64 ZIP to the Frame, connect over SSH, and run:

```sh
unzip -oq ~/frame-passthrough-shortcuts-0.1.1-linux-arm64.zip -d ~/devkit-game/Frame_Passthrough_Shortcuts
~/devkit-game/Frame_Passthrough_Shortcuts/frame-passthrough-shortcuts --install
```

`--install` registers the OpenVR manifest and enables SteamVR autostart. To
remove the registration, package files, bindings, and app settings:

```sh
~/devkit-game/Frame_Passthrough_Shortcuts/frame-passthrough-shortcuts --uninstall
```

<br>

# Build

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
