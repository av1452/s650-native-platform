# Hardware requirements of the interface

What the native interface needs from a platform, method by method, and what is known about the real S650. **Status columns are about the real camera: "Unknown" means no source located describes it.** Nothing in this table was verified against a Sony DSC-S650. Grades: **E** = confirmed for the exact ZR36473, **F** = confirmed for the COACH 9 family only, **I** = inferred from related hardware, **U** = unknown.

## 1. Display
| Need | Interface | Desktop implementation | Real S650 | Grade |
|---|---|---|---|---|
| A drawing surface of fixed size; interface draws 320×240 | `DisplayBackend::present(Framebuffer)` | PNG capture / macOS window | Panel documented as 2.0-inch, "115,200 dots (480×240)" (manual). Pixel grid, pixel aspect, colour depth and controller setup are unknown. | U |
| Programmable LCD controller | - | - | COACH 9 family brief: "on-chip programmable controller for multiple LCD vendors" | F |
| Overlay layer with transparency | (UI blends in software today) | software alpha | Family brief: "on-screen display with transparency and picture-in-picture with hardware blending". Whether software can drive it, and how, is unknown. | F / U |
| Frame update speed | ~15-60 fps assumed by animations | unthrottled | Unknown; animations degrade to cuts if needed | U |
| Backlight level | `Settings::setup.brightness` (1-5) | scales pixels (stand-in) | Unknown | U |

## 2. Input
| Need | Interface | Real S650 | Grade |
|---|---|---|---|
| Up/Down/Left/Right/OK (control button), MENU, PLAY/DISP, BACK | `InputBackend::poll()` → `Action` | The manual and service manual describe a "control button" with a centre press. How switches are read (GPIO, matrix, ADC, separate MCU) is unknown. The button set used here (BACK, DISP as separate keys) is a concept assumption. | U |
| Zoom lever, shutter half/full press, mode dial, power switch | `Wide`, `Tele`, `Shutter` (Press/Release), `ModePrev/Next`, `Power` | Physical existence of these controls is documented; their electrical interface is not. | U |
| Key repeat | handled by the window backend | Unknown | U |

## 3. Camera functions (`CameraBackend`)
| Method | What it must do | Desktop implementation | Real S650 | Grade |
|---|---|---|---|---|
| `liveFrame` | Provide the preview image behind the overlay | procedural scene with drift/zoom | The camera shows live view with original firmware; interface to it is unknown. The best case is that the original firmware keeps drawing the video layer and we draw only the overlay. | U |
| `exposure` | Report shutter, aperture, ISO currently chosen | toy model | Camera surely computes these; API unknown. | U |
| `flashWouldFire` | Predict flash for the red-eye pre-flash timing | rule in simulation | Unknown | U |
| `capture` | Take a picture, save a JPEG | fake image + metadata | Sensor, lens, shutter, AF, JPEG engine are driven by original firmware/hardware (JPEG codec in the SoC: family brief). Access unknown. | F / U |
| `setMovieRecording` | Start/stop Motion JPEG recording | flag | AVI Motion JPEG documented in manual; control unknown | U |
| `status` | availability, battery | 82 % constant | Battery level source unknown | U |
| Zoom motor, autofocus drive, flash charge/fire, CCD timing | not exposed by the interface (the camera does them) | - | **A replacement application would have to drive these itself**, which is why the preferred design leaves them to the original firmware (see `PLATFORM-RESEARCH.md`). | U |

## 4. Storage (`StorageBackend`)
| Need | Desktop | Real S650 | Grade |
|---|---|---|---|
| List images with name, folder, size, date, protect flag, EXIF-like data | in-memory records | FAT file system on Memory Stick Duo and ~24 MB internal volume observed (25,263,616 bytes). File layer API unknown. | I / U |
| Thumbnails and full decode for zoom/pan | stored 320×240 preview | SoC has a JPEG codec (family). Thumbnail source (EXIF/APP segment vs decode) and memory budget unknown. | F / U |
| Protect and delete | flag / erase | In-camera feature exists (manual); implementation unknown | U |
| Persist settings | text file | Unknown; **the internal volume must not be treated as evidence of where firmware or settings live** | U |
| Capacity and free space | 1 GB model | Card size queried at run time; mechanism unknown | U |

## 5. System
| Need | Real S650 | Grade |
|---|---|---|
| Timer for animation and auto power-off | Unknown | U |
| Wall-clock/RTC for date and time | Camera has a clock (menu documented); access unknown | U |
| Power-off request | Unknown | U |
| Execution environment (CPU, OS or bare metal, executable format, memory map, stack/heap, C++ runtime) | CPU: "32-bit MIPS" for the family (F); everything else unknown. | F / U |

## 6. Interface resource needs (measured from this code, desktop build; estimates for a port)
| Item | Value |
|---|---|
| Source size | about 3,800 lines of library code, 1,000 lines of tests |
| Static library / executable (macOS arm64, Release) | 0.68 MB / 0.53 MB (includes desktop and string tables; not a camera estimate) |
| One 320×240 frame | 307,200 B as 32-bit pixels; 153,600 B as 16-bit RGB565 (the current code uses 32-bit) |
| Frames alive at once (desktop build) | about 6-8 (output, live view, scratch for previews, cached scenes) - a port should use far fewer |
| Per-image preview in simulated storage | 307,200 B each (16 samples ≈ 4.9 MB) - prototype only; the camera would read JPEGs |
| Font / icons | bitmap font data is a few KB in code; icons are vector code |
| Dynamic allocation | heavy use of `std::string`, `std::vector`, `std::function`, `shared_ptr` - acceptable on desktop, to be reviewed for a constrained target (see `PORTABILITY.md`) |
| Floating point | used in drawing (AA lines, blits) and in the toy exposure model - a CPU without an FPU would need fixed-point rewrites of the drawing code |

## 7. What is needed to fill the unknowns
Authoritative platform documentation (boot, memory map, executable format, display/OSD programming, input, driver APIs) or the original firmware to analyse, plus a recovery method and donor hardware. See `S650-HARDWARE-IMPLEMENTATION-ROADMAP.md` in the research project for the gated stages.
