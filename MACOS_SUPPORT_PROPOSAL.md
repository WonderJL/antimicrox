# AntiMicroX macOS Support Technical Proposal

## Overview
- Deliver first-class macOS builds of AntiMicroX leveraging the Qt UI stack and SDL2 input layer already in use on Linux and Windows.
- Close current platform gaps (event injection, active window discovery, packaging) while preserving feature parity with existing releases.
- Establish a repeatable toolchain, QA plan, and release process so macOS artifacts can ship alongside Windows and Linux deliverables.

## Goals & Non-Goals
### Functional Goals
- Provide keyboard, mouse, script, and macro remapping backed by macOS-native event injection APIs.
- Support automatic profile switching for foreground applications, including Finder, Steam, and fullscreen games.
- Ship notarized `.dmg` and `.zip` artifacts plus a `.app` bundle conforming to Apple’s Gatekeeper requirements.
- Maintain feature parity with Linux/Windows where technically feasible (profiles, UI, CLI flags, translations).

### Non-Functional Goals
- Keep the codebase single-source; macOS-specific behavior uses compile-time branches without duplicating business logic.
- Require no runtime dependencies beyond Qt, SDL2, and system frameworks preinstalled on macOS.
- Integrate macOS builds into existing CI within one GitHub workflow file.

### Out of Scope
- Redesigning the Qt UI or introducing Swift/SwiftUI front-ends.
- Adding new controller features unrelated to macOS enablement.
- Providing Mac App Store distribution (future follow-up).

## Current State Assessment
| Area | Current Behavior | macOS Impact |
| ---- | ---------------- | ------------ |
| Event injection | `EventHandlerFactory` only creates `XTest`/`uinput` (Linux) or `WinSendInput` handlers (`src/eventhandlerfactory.cpp`). | macOS build fails: no handler, X11/uinput headers absent. |
| Input simulation | `uinputeventhandler.cpp` depends on `<linux/uinput.h>`. | Linux-only; must be excluded from macOS builds. |
| Active window tracking | `AutoProfileWatcher` relies on `X11Extras` or Win32 (`src/autoprofilewatcher.cpp`). | Needs macOS-specific implementation using Accessibility/Quartz APIs. |
| Captured window dialog | `CapturedWindowInfoDialog` expects `X11Extras` on non-Windows platforms (`src/gui/capturedwindowinfodialog.cpp`). | UI will not compile/run without new macOS branch. |
| Config paths | `PadderCommon::configPath()` uses XDG defaults for non-Windows (`src/common.h`). | macOS conventions require `~/Library/Application Support/AntiMicroX`. |
| Build options | CMake assumes `UNIX` ⇒ enable X11/uinput; no `APPLE` branches. | Must gate Linux options, add macOS frameworks, adjust install targets. |
| Packaging | Only Linux (AppImage/Flatpak/pkg) and Windows installers documented (`BUILDING.md`). | Need `.app` bundle, `macdeployqt`, codesign/notarize pipeline. |

## Feasibility Summary
- **Qt**: AntiMicroX builds against Qt 5/6; both support macOS (official binaries via `qt` homebrew cask or online installer).
- **SDL2**: Native macOS support for HID/gamepads; no additional porting required.
- **Event synthesis**: macOS offers `CGEventCreate*` APIs in CoreGraphics and `IOHID` for low-level access. Accessibility permission (`AXIsProcessTrustedWithOptions`) unlocks keyboard/mouse injection.
- **Foreground app detection**: `NSWorkspace.shared.frontmostApplication`, `CGWindowListCopyWindowInfo`, or AX APIs provide window title/bundle ID.
- **Tooling**: Existing CMake project can emit Xcode or Ninja builds; `macdeployqt` bundles Qt frameworks.
- **Conclusion**: Port is feasible with moderate effort (~4–6 weeks). Core risk areas are permissions UX and testing across Intel/Apple Silicon.

## Detailed Plan
### 1. Build & Toolchain Enablement
- Introduce `APPLE` branches in `CMakeLists.txt` to disable Linux-only options (`WITH_X11`, `WITH_UINPUT`, `INSTALL_UINPUT_UDEV_RULES`).
- Add `find_library`/`target_link_libraries` entries for `AppKit`, `ApplicationServices` (CoreGraphics), `IOKit`, and `Carbon` as needed.
- Ensure `USE_QT6_BY_DEFAULT` path resolves Qt frameworks on macOS; document `brew install qt sdl2` prerequisites in `BUILDING.md`.
- Generate an Info.plist template (set bundle identifier, usage descriptions) and wire into `MACOSX_BUNDLE` target.

### 2. macOS Event Simulation Layer
- Create `src/eventhandlers/maceventhandler.{h,cpp}` implementing `BaseEventHandler` using:
  - `CGEventCreateKeyboardEvent`, `CGEventPost` for key press/release.
  - `CGEventCreateMouseEvent` for button and relative/absolute mouse movement.
  - `AXIsProcessTrustedWithOptions` to request Accessibility permission at startup with user prompt.
  - `TISCopyCurrentKeyboardLayoutInputSource` + `UCKeyTranslate` for layout-aware text events.
- Update `EventHandlerFactory::buildEventGeneratorList()` to include `macos` identifier, with fallback to new handler when `Q_OS_MAC`.
- Introduce settings toggle for fallback behaviors (e.g., disable absolute mouse if permission missing) with user-facing warnings.

### 3. Active Window Detection & Auto Profiles
- Add `MacExtras` helper paralleling `X11Extras`, exposing:
  - `FrontmostApplicationInfo` struct containing `bundleIdentifier`, `executablePath`, `windowTitle`.
  - Implementation via `NSWorkspace` (bundle ID/path) and `CGWindowListCopyWindowInfo` (window titles of focused layer).
- Extend `AutoProfileWatcher` to use `MacExtras` when `Q_OS_MAC`, keeping same signals/slots.
- Update `CapturedWindowInfoDialog` to populate window metadata using `MacExtras`; show bundle ID in lieu of X11 window class.

### 4. Configuration Paths & Settings
- Switch `PadderCommon::configPath()` to call `QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)` on macOS.
- Migrate existing Linux profile assumptions (e.g., `~/.config`) by detecting old locations on first launch and copying to the new directory.
- Ensure default profile directory uses `~/Documents/AntiMicroX` or user-selected path via Qt dialogs.

### 5. UI & UX Adjustments
- Add macOS-specific messaging for permission requirements (Accessibility, Input Monitoring) in onboarding dialogs.
- Review icon/theme lookups: fallback to bundled icons when macOS themes lack freedesktop names.
- Validate menu shortcuts (⌘, ⌥) map correctly; update `JoyButton` binding labels where platform symbols differ.

### 6. Packaging & Distribution
- Configure CMake `MACOSX_BUNDLE` output (bundle structure in `build/bin/AntiMicroX.app`).
- Add script `other/macos/package.sh` to run `macdeployqt`, embed SDL2 dylib, codesign with `--deep` placeholder identity, and produce `.dmg` via `create-dmg` or `hdiutil`.
- Document manual codesign/notarization steps and required entitlements (e.g., `com.apple.security.device.usb`, `com.apple.security.cs.disable-library-validation` if injecting events).
- Prepare Homebrew tap formula template referencing release `.tar.gz` sources for community adoption.

### 7. Continuous Integration & QA Automation
- Extend GitHub Actions with macOS runner job:
  - Install deps (`brew install qt sdl2 ninja`).
  - Configure `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DMACOS=ON` (new option toggles mac-specific build bits).
  - Run `ctest` (once macOS tests exist) and archive `.app` + `.dmg` as artifacts.
- Optionally gate release uploads on signed build from trusted machine (manual promotion step).

## Work Breakdown & Milestones
| Phase | Duration | Deliverables |
| ----- | -------- | ------------ |
| Foundation | 1 week | CMake `APPLE` guards, project builds (linking, basic UI) but without event injection. |
| Input & Window APIs | 2 weeks | `MacEventHandler`, `MacExtras`, permission UX; manual validation of keyboard/mouse mapping and autoplay watcher. |
| Packaging | 1 week | `.app` bundle, packaging script, Info.plist, codesign instructions. |
| QA & Polish | 1–2 weeks | Automated smoke tests, documentation updates, regression fixes, final release checklist. |

## Testing Strategy
- **Unit Tests**: Add tests around new helpers (e.g., path resolution, serialization of bundle IDs) using QtTest.
- **Integration Tests**: Create headless harness verifying `MacEventHandler` generates expected CGEvents (sleep/delay to observe via accessibility test stubs).
- **Manual Regression**: Checklist covering controller detection, binding creation, text entry, spring mouse, auto profiles across Finder/Safari/Steam.
- **Permission Scenarios**: Test first-run without accessibility enabled, toggling permissions while running, running under different user accounts.
- **Hardware Coverage**: Validate on Intel (Monterey) and Apple Silicon (Ventura/Sonoma) with Xbox/PlayStation controllers.

## Risks & Mitigations
- **Accessibility permission denial**: Without access, core features break. Mitigate with startup checks, inline guidance, graceful degradation (disable injection features instead of crashing).
- **App Store security hardening**: macOS 13+ may block unsigned dylibs; ensure SDL2 is codesigned and embedded.
- **Controller driver variance**: Some HID devices require installation (e.g., DualSense). Provide FAQ and rely on SDL2 HIDAPI backend to minimize issues.
- **Timeline creep**: Keep scope tight by deferring optional niceties (Touch Bar support, tray icon rewrite) to later versions.

## Dependencies & Open Questions
- Confirm minimum macOS version (recommend 11 Big Sur to cover ARM64).
- Decide on Qt version baseline (Qt 6.6+ strongly recommended for Apple Silicon). Is Qt 5 support required on macOS?
- Determine signing identity availability (project maintainer vs community). Without notarization, Gatekeeper prompts will appear.
- Validate whether `CHECK_FOR_UPDATES` feature should be enabled by default on macOS (GitHub API usage). Requires network entitlement messaging.

## Next Steps
1. Green-light scope and identify maintainers/approvers for macOS contributions.
2. Create `macos-support` feature branch and begin Phase 1 tasks.
3. Schedule mid-project review to reassess risks and resource needs before packaging work.
