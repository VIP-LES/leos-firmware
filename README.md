# LEOS Firmware

Firmware for LEOS Raspberry Pi Pico-based boards.

## Project Structure

Top-level layout:

- `boards/`: Board-specific firmware targets (each board/module lives here).
- `common/`: Shared platform code used by board targets.
- `cmake/`: CMake helper modules and SDK import logic.
- `external/`: External dependencies and LEOS SDK/cyphal types.
- `build/`: Generated build output (created after configure/build).

Board executables are defined in `boards/CMakeLists.txt`. Each board implementation and support files are kept under its board folder (for example, `boards/sensor/`).

## Building

You can build firmware either with plain CMake or with the helper script.

### Option 1: Helper Script

From repository root:

```bash
./compile [--target <name>] [--release] [--rebuild]
```

Notes:

- Default mode is Debug.
- If `--target` is omitted, all targets are built.
- Use `--release` for Release builds.
- Use `--rebuild` to remove and recreate the `build/` directory before building.
- `--target` names are mapped to CMake targets (for example, `foo` -> `leos-foo-module`).

### Option 2: CMake

From repository root:

```bash
cmake -S . -B build
cmake --build build -j
```

This generates board artifacts (including `.uf2`) in the build tree, for example under `build/boards/`.