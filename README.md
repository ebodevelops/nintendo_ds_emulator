# nds_emu — Nintendo DS Emulator for Windows

Work-in-progress Nintendo DS emulator written in C++20, using SDL2 for the
Windows frontend.

## Status

Foundation + ARM7TDMI interpreter. The project builds, opens a window, parses
Nintendo DS ROM headers, copies ARM9/ARM7 program code into main RAM, and
runs the **ARM7TDMI** (ARMv4T) interpreter — full ARM + Thumb decode/execute,
banked registers, mode switching, SWI/undefined entry. The ARM9 core, video,
audio, and I/O are still stubs.

The ARM7 interpreter is exercised by a unit-test suite at
`tests/test_arm7.cpp`; run via `ctest` after building.

## Building on Windows

### Prerequisites

- Visual Studio 2022 (with the "Desktop development with C++" workload), **or**
  MinGW-w64 + Ninja
- CMake 3.20 or newer
- Git

SDL2 is fetched automatically by CMake (`FetchContent`) when it is not
installed system-wide.

### Build (Visual Studio)

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc
```

The resulting executable lives at
`build\windows-msvc\src\Release\nds_emu.exe`.

### Build (MinGW + Ninja)

```powershell
cmake --preset windows-mingw
cmake --build --preset windows-mingw
```

### Build (Linux, for development)

```bash
sudo apt install libsdl2-dev
cmake --preset linux
cmake --build --preset linux
```

## Running

```
nds_emu.exe path\to\game.nds
```

A 256×384 window opens (top + bottom DS screens stacked). ROM-header
information is printed to stdout. Close the window to exit.

You must supply your own `.nds` ROM file. No firmware/BIOS is included; once
BIOS-dependent features land, place `bios9.bin`, `bios7.bin`, and
`firmware.bin` dumped from your own DS next to the executable.

## Roadmap

See [the plan](https://claude.ai/code) for the milestone schedule. Summary:

1. ARM7TDMI interpreter (ARMv4T) — **done** (this commit)
2. ARM946E-S interpreter (ARMv5TE) + CP15
3. DMA, timers, IPC, IRQ controller
4. 2D engines + VRAM mapping
5. Input (keyboard + mouse-as-touch) + RTC
6. Audio (16 channels)
7. Cartridge backup saves
8. 3D GPU (geometry + rasterizer)
9. BIOS HLE / real-BIOS support
10. Compatibility pass, optimization, debugger, save states

## License

TBD.
