# LCD rendering research and proposed backend specification (round 9)

Goal: our own graphics on the S650's LCD. **No display register, framebuffer address or pixel format of the S650 is known, and none is assumed.** This document separates what the documents say, what the code can already do, and what a hardware backend would have to provide.

## 1. What the documents say about the display path
| Statement | Source | Class |
|---|---|---|
| Panel: 2.0-inch, "115,200 dots (480x240)" | S650 manual | V |
| LCD wired by `LCD[0..7]` + sync/clock lines to the main board connector J3 (40 contacts) | service manual labels; iFixit photos of another unit | V (labels) / T |
| "Digital video interface to LCD. Includes on-chip programmable controller for multiple LCD vendors" | COACH 8, 9, 10 briefs | F |
| "Rich color on-screen display (OSD) with transparency levels"; "Picture-in-Picture with blending support in hardware" | COACH 8, 9 (family) and 10 briefs | F |
| "Advanced GUI support including rich OSD, bitmaps, picture in picture and more"; "Resizable, multicolor fonts" (listed under the *firmware* highlights) | COACH 8/9/10 briefs | F |
| Older generation (COACH-V): "Font-based (8-bit colors) and graphics based OSD engines for displaying user-interface messages, menus and icons on the LCD/TV" | COACH-V brief (saved locally) | O |
| On-chip NTSC/PAL encoder and video DAC | COACH 9 brief | F |

### What "OSD with transparency" most plausibly means (hypotheses, H)
1. The chip composites **at least two planes in hardware**: the live/playback **video plane** and an **OSD plane** carrying menus, icons and text, with per-plane or per-pixel transparency levels. This is how a camera can show a live view while drawing UI without CPU blending.
2. The *firmware's* GUI framework draws into that OSD plane (the briefs list the GUI feature under firmware, not hardware): fonts, bitmaps, picture-in-picture.
3. The older generation had two OSD engines (a font/character one at 8-bit colour and a bitmap one); the 9S may retain something similar. **Colour depth, plane count, size, alpha precision, memory format and ownership by the original firmware are unknown.**

### Could it be useful for our interface?
Yes **if** the OSD plane can be reached: our HUD (flat translucent scrims, text, icons, AF brackets) maps naturally onto an overlay plane with transparency levels, and the original firmware would keep the video plane (live view) untouched, preserving photography. That is the display counterpart of "UI integrated with the original camera software". **Whether the S650 exposes the plane to anything other than Sony's own code is unknown.**

## 2. Can the existing renderer adapt to a fixed framebuffer with minimal change?
Tested on the desktop this round:
- **Pixel format and panel size:** `RawFramebufferDisplay` converts the unchanged 320x240 frame to RGB565 / RGB332 / Gray8 / 8-bit palette / RGB888 at any panel size (stretch or letterbox), with 16-bit word order selectable and a **dirty rectangle** per frame. The full application runs against a 480x240 RGB565 framebuffer **with zero application changes** (test `rawfb_full_application_runs_unchanged_on_a_fixed_framebuffer`). 93 tests pass.
- **Cost model:** 480x240 RGB565 = 230,400 bytes per frame (320x240 RGB565 = 153,600; 8-bit = 115,200 / 76,800).
- **Non-square pixels:** if the panel really is 480x240 showing a 4:3 picture, pixels are 1.5x wide. The test pattern's circle becomes an ellipse with width/height about 1.5 when stretched, making the question visible on the real display with a single glance. Two valid strategies: draw natively at 480x240 (UI re-layout) or let the controller/backend stretch 320x240. **Unknown which the hardware does.**
- **What still needs code changes (not done; estimates H):**
  1. *Overlay mode:* today the HUD is composited onto our own live-view picture in software. For an OSD plane the UI must render **only UI pixels with alpha** (ARGB or palette+alpha) and skip drawing the live frame. Needed: an alpha-aware `Framebuffer`/`Canvas::blendPixel` (premultiplied) and a "no live frame" mode in `ShootingScreen`/`PlaybackScreen` (viewer images are real content, not overlay). About 150-250 changed lines (estimate).
  2. *Fixed-capacity buffers:* the renderer allocates frames dynamically; see `EMBEDDED-PORTABILITY.md`.
  3. *Fonts/icons* are code-generated (no asset files), which suits a constrained target.

## 3. Proposed LCD rendering backend specification
```cpp
struct LcdCaps {                  // what the platform says it can do (all fields come from the backend, never assumed by the UI)
    int panelW, panelH;           // physical pixel grid
    PixelFormat formats[4]; int nFormats;   // supported surface formats
    bool hasVideoPlane;           // live view owned by the camera pipeline
    bool hasOverlayPlane;         // UI can draw on a separate plane
    bool overlayHasAlpha;         // per-pixel or per-plane transparency
    int overlayColorBits;         // 1..32 (8 = palette)
    bool doubleBuffered, vsync;
    int maxFps;
};
class LcdBackend : public DisplayBackend {
public:
    virtual LcdCaps caps() const = 0;
    virtual Result selectFormat(PixelFormat f, ScaleMode m) = 0;
    virtual Result lock(Surface& s) = 0;                 // gives a writable surface (pointer, stride, format)
    virtual Result commit(Rect dirty) = 0;               // present only the changed rectangle
    virtual Result setBacklight(int level1to5) = 0;      // real backlight instead of the simulated pixel scaling
    virtual Result setOverlayEnabled(bool on) = 0;       // hide our UI without touching the camera's video
};
```
**Rules**
1. **Unknown hardware means `unavailable`.** A backend that cannot establish a fact returns an error and the application keeps running; it never guesses a register or address. The desktop has `UnavailableCamera`-style behaviour to copy.
2. The UI renders into a `Framebuffer` it owns; the backend converts (as `RawFramebufferDisplay` does) or hands over its own surface for direct drawing.
3. Layering: if `hasOverlayPlane`, the UI draws overlay-only pixels; otherwise it draws the whole frame (live view source must then come from the camera backend).
4. Updates use dirty rectangles; the backend may coalesce.
5. Brightness: the UI sends a level; only the backend knows how to apply it.
6. Every call is bounded in time and failure is reported, never blocked.

## 4. The first display milestone: a precisely defined test pattern
`display/TestPattern.cpp` (`s650_native --pattern DIR`) produces: 8 colour bars (white, yellow, cyan, green, magenta, red, blue, dark grey), a 16-step grey ramp (0,17,...,255), a 1-pixel white border, 8x8 red/green/blue/yellow corner squares, a grey centre cross, a 36-pixel-radius circle and the text `S650 TEST`. Reference outputs in `docs/generated/`:
| File | Content |
|---|---|
| `lcd-test-pattern-320x240-logical.png` | the logical frame |
| `lcd-test-pattern-480x240-rgb565.bin` | 230,400 raw bytes (little-endian words), SHA-256 `84c6eb7f442701f2...` |
| `lcd-test-pattern-480x240-rgb565-decoded.png` | the same bytes decoded back |
**Success criterion for the hardware milestone (future):** the real LCD shows the pattern with all bars distinct, the ramp monotonic, the corners and border visible, and the circle's aspect ratio telling whether pixels are square. A photograph of the screen compared with the reference is sufficient evidence; no claim is made until that has actually happened.

## 5. Prerequisites for that milestone (none met)
1. A way to run our code (`EXECUTION-PATH-INVESTIGATION.md`).
2. Display facts (controller, format, plane ownership) from documents or image analysis.
3. A validated recovery path.
4. Explicit approval for each physical step.
