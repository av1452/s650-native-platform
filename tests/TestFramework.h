#pragma once
// Minimal test harness (no dependencies). Each TEST registers itself; CHECK records failures with file and line.
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
namespace testfw {
struct Case { const char* name; std::function<void()> fn; };
inline std::vector<Case>& cases() { static std::vector<Case> c; return c; }
inline int& failures() { static int f = 0; return f; }
inline int& checks() { static int c = 0; return c; }
struct Reg { Reg(const char* n, std::function<void()> f) { cases().push_back({n, std::move(f)}); } };
}  // namespace testfw
#define TEST(name) static void name(); static testfw::Reg reg_##name(#name, name); static void name()
#define CHECK(cond) do { ++testfw::checks(); if (!(cond)) { ++testfw::failures(); std::printf("    FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_EQ(a, b) do { ++testfw::checks(); auto va = (a); auto vb = (b); if (!(va == vb)) { ++testfw::failures(); std::printf("    FAIL %s:%d  %s == %s\n", __FILE__, __LINE__, #a, #b); } } while (0)
