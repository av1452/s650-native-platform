# Generated: embedded portability measurement

Produced by `tools/portability/measure.py`. Compile probes: clang, `--target=mipsel-none-elf -march=mips32 -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -nostdinc++`, compile-only.

| source | lines | std headers (transitive) | blocked by (no std headers) | with minimal shims (`target/shim`) |
|---|---|---|---|---|
| `src/core/Application.cpp` | 79 | algorithm, cstdint, deque, functional, memory, stdint.h, string, utility, vector | 'memory' file not found | 'memory' file not found |
| `src/core/Clock.cpp` | 21 | chrono, ctime, stdint.h | 'chrono' file not found | 'chrono' file not found |
| `src/core/DateTime.cpp` | 30 | cstdio, ctime, stdint.h, string | 'string' file not found | 'string' file not found |
| `src/core/Options.cpp` | 26 | - | **compiles** | **compiles** |
| `src/core/OptionsText.cpp` | 13 | cstdio, string | 'string' file not found | 'string' file not found |
| `src/core/Settings.cpp` | 38 | algorithm, stdint.h | 'algorithm' file not found | **compiles** |
| `src/core/SettingsIO.cpp` | 54 | algorithm, functional, sstream, stdint.h, string, vector | 'string' file not found | 'string' file not found |
| `src/core/Strings.cpp` | 19 | string, unordered_map, utility | 'string' file not found | 'string' file not found |
| `src/core/SystemController.cpp` | 44 | cstdint, stdint.h | 'cstdint' file not found | **compiles** |
| `src/display/Font.cpp` | 145 | algorithm, cstdint, stdint.h, string, unordered_map, utility, vector | 'algorithm' file not found | 'unordered_map' file not found |
| `src/display/Framebuffer.cpp` | 139 | algorithm, cmath, cstdint, stdint.h, string, utility, vector | 'cstdint' file not found | 'string' file not found |
| `src/display/Icons.cpp` | 95 | cmath, cstdint, stdint.h, string, utility, vector | 'cstdint' file not found | 'string' file not found |
| `src/display/Png.cpp` | 56 | cstdint, cstdio, stdint.h, string, utility, vector | 'string' file not found | 'string' file not found |
| `src/display/RawFramebufferDisplay.cpp` | 101 | algorithm, cstdint, stdint.h, string, utility, vector | 'vector' file not found | 'vector' file not found |
| `src/display/Scenes.cpp` | 85 | cmath, cstdint, stdint.h, string, utility, vector | 'cstdint' file not found | 'string' file not found |
| `src/display/TestPattern.cpp` | 23 | cstdint, stdint.h, string, utility, vector | 'cstdint' file not found | 'string' file not found |
| `src/input/InputBackend.cpp` | 19 | cctype, deque, stdint.h, string | 'deque' file not found | 'deque' file not found |
| `src/camera/SimulatedCamera.cpp` | 119 | algorithm, cmath, cstdint, cstdio, memory, stdint.h, string, utility, vector | 'memory' file not found | 'memory' file not found |
| `src/storage/SimulatedStorage.cpp` | 87 | cstdint, cstdio, fstream, memory, sstream, stdint.h, string, utility, vector | 'cstdint' file not found | 'memory' file not found |
| `src/ui/Boot.cpp` | 39 | algorithm, cstdint, functional, stdint.h, string, utility, vector | 'cstdint' file not found | 'string' file not found |
| `src/ui/DisplayOptionsScreen.cpp` | 102 | algorithm, cmath, cstdint, functional, memory, stdint.h, string, utility, vector | 'algorithm' file not found | 'string' file not found |
| `src/ui/Hud.cpp` | 158 | algorithm, cstdint, cstdio, functional, memory, stdint.h, string, utility, vector | 'string' file not found | 'string' file not found |
| `src/ui/MenuModel.cpp` | 124 | algorithm, cstdint, functional, memory, stdint.h, string, utility, vector | 'functional' file not found | 'functional' file not found |
| `src/ui/MenuScreen.cpp` | 301 | algorithm, cstdint, cstdio, functional, memory, stdint.h, string, utility, vector | 'algorithm' file not found | 'cstdio' file not found |
| `src/ui/PlaybackScreen.cpp` | 250 | algorithm, cstdint, cstdio, functional, memory, stdint.h, string, utility, vector | 'algorithm' file not found | 'cstdio' file not found |
| `src/ui/ShootingScreen.cpp` | 156 | algorithm, cstdint, functional, memory, stdint.h, string, utility, vector | 'algorithm' file not found | 'string' file not found |
| `src/ui/Theme.cpp` | 10 | stdint.h | **compiles** | **compiles** |
| `src/ui/UIController.cpp` | 68 | algorithm, cstdint, functional, memory, stdint.h, string, utility, vector | 'functional' file not found | 'functional' file not found |
| `src/ui/Widgets.cpp` | 106 | algorithm, cstdint, functional, stdint.h, string, utility, vector | 'functional' file not found | 'functional' file not found |

**Compile result:** 2/29 without any standard headers; 4/29 with the minimal shims.

## Facility usage (files affected, including via project headers)

| facility | files affected | direct uses in .cpp files |
|---|---|---|
| std::string | 24 | 102 |
| std::vector | 20 | 34 |
| std::pair/tie | 20 | 0 |
| std::min/max/clamp/sort | 16 | 92 |
| snprintf/stol (C/C++ library) | 12 | 59 |
| std::shared_ptr/unique_ptr | 10 | 6 |
| std::function | 8 | 10 |
| math (std::sin, pow, ...) | 5 | 31 |
| std::deque | 2 | 0 |
| iostream/sstream/fstream | 2 | 5 |
| std::unordered_map | 2 | 5 |
| std::chrono | 1 | 1 |
| exceptions | 1 | 2 |
| std::initializer_list | 1 | 1 |
| heap (new/delete) | 1 | 1 |

## Greedy unblock plan (provide facilities one at a time; counts are files whose standard-library needs are then fully met)

| step | provide | files unblocked (cumulative) |
|---|---|---|
| 1 | std::chrono | 4/29 |
| 2 | snprintf/stol (C/C++ library) | 5/29 |
| 3 | std::string | 7/29 |
| 4 | std::deque | 8/29 |
| 5 | iostream/sstream/fstream | 8/29 |
| 6 | std::unordered_map | 8/29 |
| 7 | std::pair/tie | 9/29 |
| 8 | std::vector | 18/29 |
| 9 | std::shared_ptr/unique_ptr | 21/29 |
| 10 | std::function | 29/29 |

(math and algorithm helpers, heap use and initializer_list are excluded from the plan: they map to compiler builtins / need no library at compile time, but linking needs compiler-rt and a libm.)
