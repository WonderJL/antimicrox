# AntiMicroX Virtual Controller Support (macOS)

## Overview
- Deliver an optional backend that exposes AntiMicroX mappings as a macOS-visible virtual game controller, so games receive authenticated joystick/HID events rather than keyboard/mouse emulation.
- Implement the virtual controller in user space using `IOHIDUserDevice` (available since macOS 10.9) backed by a well-formed HID gamepad report descriptor.
- Maintain compatibility with existing event handlers; the new backend is opt-in via settings/CLI flags and coexists with mouse/keyboard simulation.

## Goals & Non-Goals
### Goals
- Create a virtual HID gamepad that appears in `IORegistry` and to `GCController`/SDL clients as a standard controller with buttons, hats, and analog axes.
- Allow AntiMicroX profiles to map keyboard/mouse inputs to virtual controller elements (e.g., map WASD to left stick axes, mouse delta to right stick).
- Provide a tray/UI indicator when the virtual controller is active and expose runtime calibration (dead zones, axis scaling).
- Package the feature with minimal user setup: no kernel extensions or DriverKit system extensions required.

### Non-Goals
- Implement non-standard controller protocols (e.g., DualShock/Series X specific HID descriptors) beyond the generic HID gamepad report.
- Provide rumble/force-feedback passthrough in the first iteration.
- Support multi-controller virtualization (initial scope is one virtual device per AntiMicroX instance).

## Technical Background
- macOS applications access controllers via HID Manager (`IOHIDManager`) or the Game Controller framework (`GCController`). Both rely on HID devices registered under the `IOHIDDevice` plane.
- Apple allows user-space processes to create virtual HID devices with `IOHIDUserDevice`. The app must supply:
  - A HID report descriptor describing collections (buttons, axes, hats).
  - Properties such as `kIOHIDReportIntervalKey`, `kIOHIDProductIDKey`, and vendor/product strings.
  - Each input report via `IOHIDUserDeviceHandleReport`.
- The process requires the `com.apple.security.device.usb` entitlement for sandboxed apps, but non-sandboxed desktop apps can create HID devices without additional entitlements.
- Existing AntiMicroX event pipeline already emits normalized values from `JoyButtonSlot`. We can layer the virtual controller backend alongside `SendInput`/`uinput` by introducing a `MacVirtualControllerEventHandler` that implements `BaseEventHandler`.

## Proposed Architecture
1. **HID Descriptor & Device Management** (`src/mac/machid/`):
   - Define a vendor/product pair (e.g., Vendor 0x3438, Product 0x00A0 reserved for AntiMicroX) and a HID descriptor covering:
     - 16 digital buttons (Usage Page 0x09).
     - D-pad/hat switch (Usage 0x39) with 8 directions.
     - 4 analog axes (X, Y, Rx, Ry) with logical min/max -32767..32767.
     - 2 triggers (Z, Rz) as analog axes.
   - Implement a singleton `MacVirtualPad` that owns the `IOHIDUserDeviceRef`, handles lifecycle (create/destroy), and exposes `setButtonState`, `setAxisState`, `commitReport` APIs.

2. **Event Handler Integration**:
   - Add a new backend `MacHIDEventHandler` derived from `BaseEventHandler`.
   - Map `JoyButtonSlot` modes to HID fields:
     - `JoyKeyboard`/`JoyMouseButton` slots assigned to virtual buttons or hat directions.
     - `JoyMouseMovement` or analog stick bindings feed axis values with optional smoothing.
     - Provide translation tables for macro/script slots -> no-op or future extension.
   - Update `EventHandlerFactory` to expose identifier `machid` for `Q_OS_MAC` builds. Fallback order on macOS becomes `machid` → `macos` (Quartz) to preserve old functionality.

3. **Configuration/UI**:
   - Add a toggle in macOS settings pane (`MainSettingsDialog`) labeled "Virtual Controller Output" with ability to edit vendor/product strings.
   - Provide per-profile mapping templates (e.g., "Keyboard WASD" mapping to left stick) stored in new JSON config under `virtualcontroller/`.

4. **Runtime Behavior**:
   - When enabled, the backend opens the virtual device at app startup. If creation fails (e.g., missing permissions), show a macOS notification guiding the user.
   - Each frame/event dispatch the handler updates an in-memory report buffer and calls `IOHIDUserDeviceHandleReport` at ~250 Hz, or when state changes, whichever comes first.

## Implementation Plan
| Phase | Duration | Deliverables |
|-------|----------|--------------|
| HID Foundations | ~1 week | HID descriptor, `MacVirtualPad` helper, test harness verifying device appears in IORegistry. |
| Event Handler Integration | ~1-2 weeks | `MacHIDEventHandler`, mapping logic, settings flag, fallback updates in `EventHandlerFactory`. |
| UI & Config | ~1 week | Settings checkbox, profile binding UI updates, persistence in `antimicrox_settings.ini`. |
| QA & Polish | ~1 week | Automated/smoke tests, docs, troubleshooting guidance. |

## Testing Strategy
- **Unit Tests**: Qt-based tests for HID report packing, verifying axis normalization and button bitfields.
- **Integration Tests**: Small app verifying `GCController.controllers()` sees the AntiMicroX device and reads state changes.
- **Manual QA**: Validate with popular macOS games (Steam titles, emulators) using the virtual pad. Confirm default orientation, dead zones, and hat behavior.
- **Regression**: Ensure existing keyboard/mouse emulation still works when virtual controller is disabled.

## Permissions & Distribution
- Running unsandboxed avoids additional entitlements. If a sandboxed build is desired (e.g., for App Store), request `com.apple.security.device.usb` or `com.apple.security.device.hid.devd`. This spec assumes unsandboxed distribution.
- No kernel extensions required; `IOHIDUserDevice` runs entirely in user space.

## Risks & Mitigations
- **macOS Updates**: Future macOS versions may restrict virtual HID creation. Track Apple developer notes, consider fallbacks (DriverKit HID extension) if required.
- **Game Compatibility**: Some titles expect specific vendor/product IDs or use Steam Input. Provide configurable IDs and a knowledge base section for known compatibility notes.
- **Performance**: Handling high-frequency updates can tax the main thread. Use a dedicated dispatch queue/timer to push HID reports without blocking UI.

## Next Steps
1. Approve scope and vendor/product IDs.
2. Implement `MacVirtualPad` prototype and run IORegistry tests.
3. Wire the backend into AntiMicroX after confirming a stable HID device.
4. Document usage, troubleshooting, and limitations in the README/wiki.
