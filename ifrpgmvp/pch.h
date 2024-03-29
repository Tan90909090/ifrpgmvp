// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

#include <string_view>
#include <array>
#include <memory>
#include <limits>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <cassert>

// x86-windows-static版のvcpkgを使うよう.vcxprojの<VcpkgTriplet>で指定している
#include <png.h>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#endif //PCH_H
