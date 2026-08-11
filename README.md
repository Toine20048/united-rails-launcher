<p align="center">
  <img alt="Unlimited Rails" src="/program_info/icon-256.png" width="128">
</p>

<h1 align="center">Unlimited Rails Launcher</h1>

<p align="center">
  A Minecraft launcher for one modpack: <b>Neo Rails</b>.<br>
  Install it, keep it updated, press Play.
</p>

<p align="center">
  <b>This is an unofficial fork of <a href="https://github.com/PrismLauncher/PrismLauncher">Prism Launcher</a>
  and is not endorsed by or affiliated with it.</b><br>
  Please do not report issues with this launcher to the Prism Launcher project.
</p>

---

## What it does

- Installs the Neo Rails modpack on first run — no instance setup, no pack browsing
- Checks `unlimited-rails.de` on every start and offers the new version when one is published
- Updates in place, keeping worlds, settings, and any mods, resource packs or shaders the player added
- Signs in with a Microsoft account, exactly as Prism does
- Shows server screenshots and a changelog, both managed from the website
- Gives access to mods, resource packs, shaders, memory allocation and the game log through one menu

## Download

Grab the installer or the portable zip from
[Releases](../../releases).

- **Installer** — `UnlimitedRails-Setup.exe`
- **Portable** — unzip anywhere and run `unlimitedrails.exe`

Windows only for now. The code still builds for Linux and macOS as upstream
does, but those are not tested or released here.

### "Windows protected your PC"

Windows will probably show a blue **"Windows protected your PC"** box the first
time you run the launcher. To continue, click **More info**, then **Run anyway**.

This is expected, and it is not a virus warning. Windows shows it for any
program that has not been signed with a paid code-signing certificate, purely
because it has not seen it downloaded many times before. It says nothing about
what the program does.

Signing certificates cost money per year, and this is a free community
launcher, so it is unsigned for now. If you would rather verify the download
yourself, every release is built from the source in this repository — you can
read it, and build it yourself with the instructions below.

## Building from source

Requires a C++20 compiler, CMake, Ninja and Qt 6. On Windows via
[MSYS2](https://www.msys2.org/) (UCRT64 environment):

```
pacman -S --needed \
  mingw-w64-ucrt-x86_64-cc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-5compat \
  mingw-w64-ucrt-x86_64-qt6-svg mingw-w64-ucrt-x86_64-qt6-imageformats \
  mingw-w64-ucrt-x86_64-qt6-tools mingw-w64-ucrt-x86_64-qt6-networkauth \
  mingw-w64-ucrt-x86_64-quazip mingw-w64-ucrt-x86_64-cmark \
  mingw-w64-ucrt-x86_64-zlib mingw-w64-ucrt-x86_64-tomlplusplus \
  mingw-w64-ucrt-x86_64-qrencode mingw-w64-ucrt-x86_64-extra-cmake-modules
```

```
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=install \
  -DLauncher_BUILD_PLATFORM=msys2 \
  -DLauncher_APP_BINARY_NAME=unlimitedrails \
  -DLauncher_ENABLE_UPDATER=OFF \
  -DLauncher_WARNINGS_AS_ERRORS=OFF
cmake --build build
cmake --install build --prefix install
```

`Launcher_WARNINGS_AS_ERRORS=OFF` is needed with compilers newer than upstream's
CI uses. See [MODIFICATIONS.md](MODIFICATIONS.md) for the full list of changes
and why each was necessary.

## Licence

**GPL-3.0-only**, the same licence as Prism Launcher, which this is derived from.

That means you are free to use, study, modify and redistribute this software,
including commercially — provided that anything you distribute is also released
under GPL-3.0 with its complete corresponding source code, that you keep the
copyright and licence notices intact, and that you state what you changed.

- [`LICENSE`](LICENSE) — full GPL-3.0 text
- [`COPYING.md`](COPYING.md) — attribution chain: Prism Launcher, PolyMC, MultiMC
- [`MODIFICATIONS.md`](MODIFICATIONS.md) — what this fork changed, and when

Upstream copyright notices have been kept and ours added alongside, not in place
of them. The Minecraft launching, account, Java and download machinery is the
work of the Prism Launcher, PolyMC and MultiMC contributors.

The Unlimited Rails name and logo are not part of the GPL grant — the licence
covers the code, not the branding. If you fork this, please rebrand it, exactly
as this project rebranded away from Prism.

Minecraft is a trademark of Mojang Studios. This project is not affiliated with
Mojang Studios or Microsoft.
