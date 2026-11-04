# LCD integration: the first real hardware target (round 8)

**Status: nothing about the S650's display path is known beyond panel-level facts and family-level chip features. No framebuffer address, register, pixel format or refresh behaviour is stated here as fact, and none may be guessed.**

## Known
| Fact | Source | Grade |
|---|---|---|
| Panel: 2.0-inch, "115,200 dots (480×240)" | S650 manual | confirmed (document) |
| The LCD connects to the main board by a long bottom connector (silkscreen J3/40); the service manual's block names include `LCD[0..7]` plus sync/clock lines | iFixit photos; service manual | P / C (text) |
| The COACH 9 family has an on-chip **programmable LCD controller** ("multiple LCD vendors") and an **OSD layer with transparency** with hardware blending / picture-in-picture | COACH 9 brief | F |
| 8 data lines suggest a serial-RGB-style or narrow parallel interface; the pixel format (RGB565? 8-bit palette? YUV?) and timing are not documented | inference from signal names | I / U |

## Unknown (all required before drawing a single pixel from our code)
Controller register map and initialisation, panel timing, pixel format and colour depth, framebuffer/OSD location and size, how many layers exist, whether the OSD is separate from the video layer, refresh/ vsync behaviour, backlight control, and **whether the original firmware owns the controller exclusively** (very likely, since it draws the live view and its menus).

## Integration approaches (technical comparison)
| Approach | Needs | Risk | Evidence status |
|---|---|---|---|
| **A. Draw via the original firmware's own drawing/OSD routines** (hook or call them) | The firmware image, the routine addresses, calling conventions | Lowest hardware risk; depends on firmware knowledge | Blocked: no image |
| **B. Take over the OSD layer from our own code while the original firmware keeps the video layer** | Controller documentation or reverse-engineered register behaviour; a way to run code concurrently | Medium: can corrupt the original's display state | Unknown whether a separate OSD layer is accessible |
| **C. Own the whole display** (our own controller init) | Full panel and controller knowledge; our own camera pipeline | High; means re-implementing the live view | Not recommended (see platform research) |
| **D. Observation-only first** (read the framebuffer/OSD memory of the running original firmware) | A safe way to read memory | Needs hardware access | Not available |

**Preferred:** A, then B. C only if the camera can be fully initialised independently (no evidence).

## The first display milestone
Show a **static test pattern** (for example 8 colour bars plus a 1-pixel border and the text `S650`) on the real LCD. Defined in a hardware-neutral way so the same pattern is used on the desktop (it can already be rendered through `DisplayBackend` today).

**Prerequisites** (all unmet): (1) a way to run our code (`EXECUTION-ENVIRONMENT.md`), (2) a validated recovery path on a donor (`RECOVERY-REQUIREMENTS.md`), (3) the display facts above from documents or image analysis, (4) explicit approval for each experiment.

**Placeholder in the code base:** the interface already isolates the display behind `DisplayBackend::present(const Framebuffer&)`. A camera implementation does **not** exist and must not be faked; until the facts above are known it should be an *unavailable* backend that reports "no display path", like `UnavailableCamera` does for the camera. The framebuffer is 32-bit today; a 16-bit or palette conversion layer would be added only once the hardware's format is known.

## Non-invasive steps that are possible now
- Compute and document what the desktop interface needs at 480×240 vs 320×240 (a panel of 480×240 dots with a 4:3 picture implies non-square pixels, **a hypothesis**; either the UI is drawn at 480×240 or the controller scales 320×240).
- Pre-build the test pattern and the colour-format conversion routines (RGB565, 8-bit palette, YUV) so that a later experiment changes one function.
