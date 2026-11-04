#include "core/Options.h"

namespace s650 {

const SizeOption kSizes[6] = {
    {"7M",  3072, 2304, 3100000, 1700000}, {"3:2", 3072, 2048, 2800000, 1500000},
    {"5M",  2592, 1944, 2200000, 1200000}, {"3M",  2048, 1536, 1400000, 800000},
    {"1M",  1280, 960,  600000,  350000},  {"VGA", 640,  480,  200000,  120000}};
const char* const kModeKeys[4] = {"auto", "program", "scene", "movie"};
const char* const kSceneKeys[4] = {"portrait", "landscape", "twilight", "highspeed"};
const char* const kTimerKeys[3] = {"off", "2s", "10s"};
const char* const kQualityKeys[2] = {"fine", "standard"};
const char* const kAfKeys[2] = {"multi", "center"};
const char* const kFlashKeys[4] = {"auto", "on", "slow", "off"};
const char* const kWbKeys[6] = {"auto", "daylight", "cloudy", "fluorescent", "incandescent", "flash"};
const int kIsoValues[6] = {0, 100, 200, 400, 800, 1000};
const char* const kBeepKeys[3] = {"shutter", "on", "off"};
const char* const kUsbKeys[3] = {"msc", "pictbridge", "ptp"};
const int kAutoOffMinutes[4] = {0, 1, 2, 5};
const char* const kDensityKeys[4] = {"minimal", "standard", "detailed", "custom"};
const char* const kExposureInfoKeys[3] = {"off", "changed", "always"};
const char* const kStyleKeys[3] = {"modern", "light", "classic"};
const char* const kGridKeys[4] = {"off", "thirds", "square", "diagonal"};
const int kSlideIntervals[3] = {2, 4, 8};

}  // namespace s650
