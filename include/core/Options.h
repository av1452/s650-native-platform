#pragma once
// Option tables. All values are CONCEPT assumptions carried over from the Version 2.0 browser prototype,
// not S650 documentation (the retailer-quoted ISO 1000 ceiling and the image-size list are unverified).

namespace s650 {

struct SizeOption { const char* id; int w, h; int fineBytes, stdBytes; };

extern const SizeOption kSizes[6];
extern const char* const kModeKeys[4];       // auto program scene movie
extern const char* const kSceneKeys[4];
extern const char* const kTimerKeys[3];      // off 2s 10s
extern const char* const kQualityKeys[2];
extern const char* const kAfKeys[2];
extern const char* const kFlashKeys[4];
extern const char* const kWbKeys[6];
extern const int kIsoValues[6];              // 0 = auto
extern const char* const kBeepKeys[3];
extern const char* const kUsbKeys[3];
extern const int kAutoOffMinutes[4];
extern const char* const kDensityKeys[4];    // minimal standard detailed custom
extern const char* const kExposureInfoKeys[3];
extern const char* const kStyleKeys[3];      // modern light classic
extern const char* const kGridKeys[4];
extern const int kSlideIntervals[3];

constexpr int kEvSteps = 13;                 // -2.0 .. +2.0 in 1/3 EV steps
inline float evValue(int idx) { return float(idx - 6) / 3.0f; }
constexpr int kZoomSteps = 26;               // 0..25, each 0.04 of the range

}  // namespace s650
