# Unlimited Rails Launcher — fork notes

A fork of [PrismLauncher](https://github.com/PrismLauncher/PrismLauncher) v12.0.0,
rebranded and cut down to launch one modpack: Neo Rails, served from
`https://unlimited-rails.de/`.

Licensed GPL-3.0, same as upstream. Upstream copyright notices are kept and
ours added alongside — that is a licence requirement, not a courtesy.

## Building

Toolchain is MSYS2 at `D:\msys64` (UCRT64 environment). It lives on D: because
C: only had ~30 GB free. Provisioned by `/setup-toolchain.sh` inside MSYS2.

Current versions: GCC 16.2, CMake 4.4.2, Ninja 1.13.2, Qt 6.11.1.

```
D:\msys64\usr\bin\bash.exe -lc "bash /configure.sh && bash /build.sh"
```

Helper scripts at the MSYS2 root:

| Script | Does |
| --- | --- |
| `/setup-toolchain.sh` | Installs MSYS2 packages. One-off. |
| `/configure.sh` | CMake configure with the right options. |
| `/build.sh` | Builds. Preserves ninja's exit code. |
| `/genicons.sh` | Regenerates `.ico`/`.png` from the SVG. |
| `/package.sh` | Produces `install/` with Qt DLLs bundled. |

**Never pipe the build to `tail`.** The pipeline returns `tail`'s exit code, so
a failed build reports success and the real error scrolls away. `/build.sh`
writes to `/d/dev/build.log` and greps it instead.

## Patches against upstream

Prism v12 does not compile on a 2026 toolchain. Three changes were required:

1. **`-mguard=cf`** — rejected by GCC 16. Now behind `check_cxx_compiler_flag`
   in `CMakeLists.txt`, so it is still used where supported.
2. **Java `-source/-target 7`** — JDK 21 refuses anything below 8. Bumped to 8
   in `libraries/launcher` and `libraries/javacheck`. Safe: the pack needs
   Java 21 regardless.
3. **`-Werror` was hardcoded** — GCC 16 warns about things upstream CI's older
   compilers do not. Added a `Launcher_WARNINGS_AS_ERRORS` option that defaults
   to ON (upstream behaviour); our build passes OFF.

All three are conditional rather than deletions, so pulling upstream changes
does not conflict.

## Branding

Driven entirely by three CMake variables, so renaming the files in
`program_info/` plus editing these is the whole job:

| Variable | Value |
| --- | --- |
| `Launcher_CommonName` | `UnlimitedRails` |
| `Launcher_DisplayName` | `Unlimited Rails` |
| `Launcher_AppID` | `de.unlimitedrails.Launcher` |
| `Launcher_APP_BINARY_NAME` | `unlimitedrails` |
| `Launcher_Domain` | `unlimited-rails.de` |

Two traps:

- `launcher/main.cpp` hardcodes `Q_INIT_RESOURCE(prismlauncher)`. It must match
  the renamed `.qrc` or the link fails with an undefined reference.
- **`Launcher_ENABLE_UPDATER` must stay OFF.** Prism's updater checks Prism's
  own GitHub releases; left on, this launcher would offer to update itself into
  real Prism Launcher.

`metainfo.xml` and the man page still say "Prism Launcher". Both are Linux-only
(AppStream / man) and do not affect Windows builds.

## Auth

Uses Prism's bundled MSA client ID (`CMakeLists.txt:272`). Prism allows this —
a custom client ID is optional — so Microsoft login works in a self-built fork
with no Azure app and no Mojang approval. If it ever gets rate-limited, the fix
is registering an Azure app and setting `-DLauncher_MSA_CLIENT_ID=...`.

## Icons

`program_info/de.unlimitedrails.Launcher.svg` is the source of truth and is
currently a **placeholder**. Replace it, then run `/genicons.sh` to regenerate
`unlimitedrails.ico` (7 sizes, 16–256) and the 256px PNG, then rebuild.

## Distribution

`/package.sh` writes a self-contained `install/` tree with all 66 Qt/mingw DLLs
bundled; verified to run with MSYS2 off the PATH. Zip that directory and hand
it out.

The NSIS installer is **not** working — MSYS2's `makensis` crashes on Prism's
`win_install.nsi` with `basic_string: construction from null`. A portable zip
needs no installer, no admin rights, so this is not currently worth chasing.

## The fork's own code

Everything specific to this launcher is in `launcher/unlimitedrails/`:

- **`PackSource`** — polls `api/version.php` for the current version, download
  URL, sha256, changelog and screenshot list.
- **`HomeWindow`** — the whole UI. One button that reads Install, Update to X,
  or Play depending on state; an account picker; the changelog; and the
  screenshots painted as a full-window backdrop that cross-fades every 8s.
  Only the top and bottom bands are dimmed, by gradient, so the middle of the
  screenshot keeps its real brightness.
- **`Style`** — the app-wide dark/gold look, applied to every dialog, plus
  `launcherIcon()`.

The **⚙ menu** reaches Prism's own instance pages by id rather than
reimplementing them: `mods`, `resourcepacks`, `shaderpacks`, `settings`
(memory/Java), `console` (log), plus sign-out and the game folder.

`default-backdrop.jpg` is bundled and painted from the first frame, so startup
never shows black while the server screenshots load. JPEG, not PNG: 195 KB
against 1.2 MB for something that is dimmed and scaled anyway.

**Icons must be rendered per size, not scaled.** The icon SVG has the logo
embedded as a raster, so letting Qt scale it to 16px produces mush. `genicons.sh`
renders `icon-{16,24,32,48,64,128,256}.png` and `launcherIcon()` builds a QIcon
from all of them. The header uses `icon-128.png` downscaled once.

Screenshots and changelog come from the server, never from the build, so they
can be changed from the admin panel without shipping a new launcher.

Install and update both use `InstanceImportTask`, which takes a URL directly.
Passing `extraInfo["original_instance_id"]` is what makes an update overwrite
the existing instance and keep saves, `options.txt` and resourcepacks, rather
than creating a second copy.

### Startup wiring

`performMainStartupAction()` creates `HomeWindow` instead of calling
`showMainWindow()`. `MainWindow` still compiles and is left untouched, but
`m_mainWindow` stays null — anything that dereferences it needs a guard.
Exiting works because `HomeWindow::isClosing` is connected to
`on_windowClose`, which decrements the open-window count.

Prism's setup wizard is skipped via `applyLauncherDefaults()`, which sets the
theme, language and automatic Java download. Without it, a first run lands on a
theme picker instead of the Play button.

## Idea: incremental updates (not built)

Every update downloads the whole `.mrpack`. Measured against Neo Rails 3.1:

| Part | Size | Behaviour on update |
| --- | --- | --- |
| 110 files from Modrinth's CDN | 488 MB | Already incremental — Prism diffs the index and skips unchanged files |
| 228 files bundled in the `.mrpack` | 74 MB compressed | Re-downloaded and re-extracted every time |

The bundled part is 60.5 MB of 27 mods, 10.6 MB shaderpacks, 2.7 MB resource
packs, 0.4 MB configs. Those 27 are the mods not on Modrinth — including the
ones written in-house, which are exactly what changes between versions. So a
one-line fix in RailAtlas currently costs every player 78 MB.

The fix would be a file-level sync: publish each override file content-addressed
by SHA-256 with a `manifest.json`, and have the launcher download only what
differs. A typical update drops to a few MB. The cost is that publishing becomes
"run a script, then upload" rather than just uploading the `.mrpack`, which is
why it was deferred.

## Not verified yet

The launcher builds, starts, reaches the API and reports the right state. These
paths have not been exercised end to end:

- Microsoft login (needs real credentials).
- Installing the pack (78 MB download plus NeoForge setup).
- Launching the game.
- An actual update, which needs a second pack version published.
