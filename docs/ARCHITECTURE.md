# Architecture

## Principle
**The interface knows nothing about hardware.** It draws into a 320×240 software framebuffer, reads logical button events, and asks four backend interfaces for everything else. Replacing the backends is how a future camera port would work; the interface code would not change. No backend here pretends to be a real camera: simulated ones say so, and `UnavailableCamera` fails every call.

```
                +-------------------------- Application -----------------------------+
 InputBackend ->|  SystemController (power, boot, auto-off)   UIController            |-> DisplayBackend
  (buttons)     |        |                                    |  screens + dialog     |    (frames)
                |        +----------- Settings (plain data) --+                       |
                +--------------------------|-----------------------|-----------------+
                                      CameraBackend          StorageBackend
                                  (live view, exposure,     (images, protect, delete,
                                   capture, movie)           settings persistence)
```

## Modules (`include/<module>`, `src/<module>`)
| Module | Contents |
|---|---|
| `core` | `Types` (Color, Rect, `Action`, `InputEvent`), `Clock` (SystemClock, ManualClock, date helpers), `Options` (option tables), `Settings` (data, effective values, serialise/deserialise), `Strings` (+ generated `StringsEn.cpp`), `SystemController`, `Application`, `DesktopRig` (headless simulated rig for tests/tour) |
| `display` | `Framebuffer` + `Canvas` (fill, alpha, AA line/circle, polygon, scaled blit, clip, text), `Font` (5×7 proportional bitmap), `Icons` (vector line icons), `Png`, `DisplayBackend` + `HeadlessDisplay`, `Scenes` (procedural pictures for the simulations), `CocoaWindow` (macOS) |
| `input` | `InputBackend`, `ScriptedInput`, key mapping |
| `camera` | `CameraBackend`, `SimulatedCamera`, `UnavailableCamera`, `Exposure` (toy model; simulation only) |
| `storage` | `StorageBackend`, `SimulatedStorage` (memory + optional settings file), `ImageRecord` |
| `ui` | `UIController`, screens (`ShootingScreen`, `MenuScreen`, `DisplayOptionsScreen`, `PlaybackScreen`), `MenuModel` (data-driven menu), `Hud` (shooting overlay renderer), `Widgets`, `Theme`, `Boot` |

## Backend interfaces (summary)
```cpp
class DisplayBackend { initialize(); clear(); present(const Framebuffer&); shutdown(); name(); };
class InputBackend   { initialize(); bool poll(InputEvent&); name(); };
class CameraBackend  { initialize(); CameraStatus status(); bool liveFrame(Framebuffer&, const Effective&, ms);
                       ExposureInfo exposure(const Effective&); bool flashWouldFire(...);
                       CaptureResult capture(const Effective&, wall); Result setMovieRecording(bool); };
class StorageBackend { initialize(); images(); addImage(); setProtected(); removeImage(); removeAllUnprotected();
                       format(); capacity/used/free; loadSettings(); saveSettings(); };
class Clock          { nowMs(); wallSeconds(); };
```
`Action` is the only input vocabulary: Up, Down, Left, Right, Ok, Back, Menu, Play, Wide, Tele, Shutter (Press = half-press, Release = shoot), ModePrev, ModeNext, Power, Delete, Protect, Disp. Pointer/touch events do not exist.

## Frame loop
`Application::tick()`: compute `dt` from the `Clock` (capped at 100 ms) → drain `InputBackend` → `SystemController::update()` (boot, shutdown, auto power-off) → `UIController::update(dt)` → render into one `Framebuffer` → apply the brightness setting → `DisplayBackend::present()`. Tests drive the same loop with a `ManualClock` and `ScriptedInput`.

## Power and lifecycle
`SystemController`: Off → Booting (1.9 s animation, any key skips, or direct if disabled in Setup) → On → ShuttingDown (1.35 s) → Off. Auto power-off counts idle time from the last key press. This is the *simulated* lifecycle only.

## UI structure
- `UIController` owns the screens and a modal `Dialog` (default focus is the safe choice; Initialize defaults to OK as in Version 2.0). It routes input to the dialog first.
- `MenuScreen` is a state machine with levels Rail → Rows → Picker | Slider | Edit | View, remembered category/row per category, and three appearances from one data model (`buildMenu`). Rows carry a "disabled because…" function so locked items explain themselves.
- `Hud` renders from a `HudModel` value (settings, exposure info, remaining shots, focus state, …), so the display-options preview reuses the exact shooting overlay.
- `ShootingScreen` runs AF seek/lock (240 ms), self-timer, red-eye pre-flashes, movie recording state, toasts and the lens-settle animation.
- All geometry follows `S650-INTERFACE-PORTING-SPEC.md` (22 px header, 24 px rows, 34 px description bar, 16 px hint bar, modern body = exactly seven rows, classic = five).

## Failure behaviour
- Camera backend fails to initialise: startup continues, the log says so, the shooting screen shows "NO CAMERA", shutter attempts show "Camera unavailable…", nothing is invented.
- Storage full: capture is refused with "Memory full". Protected images cannot be deleted. Settings that cannot be saved are announced, not hidden.
- Corrupt or unreadable settings file: ignored, defaults used. Unknown/out-of-range values are clamped.

## Testing approach
Unit and behaviour tests drive `Application` end-to-end through `DesktopRig`: scripted buttons, manual clock, headless display, in-memory storage. Rendering tests check deterministic hashes, specific theme pixels and a smoke pass over every screen × theme. See `DEVELOPMENT-ROADMAP.md` for results.

## Extension points for a real camera (not implemented)
1. A `DisplayBackend` that writes the frame to whatever drawing surface the camera exposes (format and API unknown).
2. An `InputBackend` that maps physical switches to `Action`s (mechanism unknown).
3. A `CameraBackend` that forwards to the original camera functions (unknown) — and should report `available=false` for anything it cannot do.
4. A `StorageBackend` over the camera's file system and settings storage (unknown).
5. A `Clock` over the camera's timer/RTC (unknown).
