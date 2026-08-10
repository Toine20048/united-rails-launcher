# Modifications from upstream PrismLauncher

This file exists to satisfy GPL-3.0 §5(a): a modified work must carry prominent
notices stating that it was changed, and the date of those changes.

**Base:** [PrismLauncher](https://github.com/PrismLauncher/PrismLauncher) v12.0.0
(`develop`, commit `d7870ff659cbb66707fe0141e21e28bf84210857`)

**Modified by:** Unlimited Rails
**Date of modifications:** 10–11 August 2026

This is an unofficial fork. It is **not** produced, endorsed, or supported by
the Prism Launcher project. Please do not report problems with this launcher to
them.

## What was changed

### Purpose

Upstream Prism manages any number of Minecraft instances from any source. This
fork launches exactly one modpack — Neo Rails, from `unlimited-rails.de` — and
the interface was reduced to match.

### New code (`launcher/unlimitedrails/`)

| File | Purpose |
| --- | --- |
| `PackSource.{h,cpp}` | Polls `unlimited-rails.de/api/version.php` for the current pack version, download URL, SHA-256, changelog and screenshots |
| `HomeWindow.{h,cpp}` | Replaces the instance-list UI with a single-pack home screen: one Install/Update/Play button, account picker, changelog, screenshot backdrop, settings menu |
| `Style.{h,cpp}` | Application-wide dark/gold theme and the multi-size launcher icon |

### Changed upstream files

| File | Change |
| --- | --- |
| `CMakeLists.txt` | `-mguard=cf` guarded behind `check_cxx_compiler_flag` (GCC 16 rejects it); binary renamed to `unlimitedrails` |
| `launcher/CMakeLists.txt` | Added `Launcher_WARNINGS_AS_ERRORS` option (default ON, matching previous behaviour); added the new sources; made the Windows resource depend on the `.ico` and `.manifest` |
| `launcher/Application.cpp/.h` | Shows `HomeWindow` instead of `MainWindow`; skips the setup wizard via `applyLauncherDefaults()`; registers the `URPack*` settings; applies the fork's style |
| `launcher/main.cpp` | `Q_INIT_RESOURCE` renamed to match the renamed `.qrc` |
| `launcher/ui/InstanceWindow.cpp` | Uses the launcher icon rather than the per-instance icon |
| `libraries/launcher/CMakeLists.txt`, `libraries/javacheck/CMakeLists.txt` | Java `-source/-target` raised from 7 to 8 (JDK 21 refuses 7) |
| `program_info/*` | Rebranded: name, application ID, domain, icons, installer strings. Prism's artwork replaced. `File /nonfatal` for the updater binary, which this fork does not build |

### Build configuration

The updater is disabled (`Launcher_ENABLE_UPDATER=OFF`). Upstream's updater
points at Prism's own GitHub releases, which would offer to "update" this
launcher into Prism Launcher.

### Not changed

The Minecraft launching machinery, account handling, Java handling, download
and instance-management code are upstream's work, unmodified.

## Licence

GPL-3.0, the same as upstream. See `LICENSE` for the full text and `COPYING.md`
for the attribution chain back through PolyMC to MultiMC. Upstream copyright
notices have been preserved; ours were added alongside rather than replacing
them.
