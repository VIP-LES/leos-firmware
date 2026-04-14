# LEOS Firmware

Firmware for LEOS Raspberry Pi Pico-based boards.

## Project Structure

Top-level layout:

- `boards/`: Board-specific firmware targets (each board/module lives here).
- `tools/`: Independent testing firmware or software used to support platform development.
- `common/`: Shared platform code used by board targets.
- `cmake/`: CMake helper modules and SDK import logic.
- `external/`: External dependencies and LEOS SDK/cyphal types.
- `build/`: Generated build output (created after configure/build). Output firmware will all be in `build/firmware`.

Board executables are defined in `boards/CMakeLists.txt`. Each board implementation and support files are kept under its board folder (for example, `boards/sensor/`). Any supporting tools or testing firmware is grouped under the `tools/` directory

## Building

You can build firmware either with plain CMake or with the helper script.

### Option 1: Helper Script

From repository root:

```bash
./compile [--target <name>] [--release] [--rebuild] [--install]
```

Notes:

- Default mode is Debug.
- If `--target` is omitted, all production firmware targets are built (no tools).
- Use `--release` for Release builds.
- Use `--rebuild` to remove and recreate the `build/` directory before building.
- `--target` names are mapped to CMake targets (for example, `radio` -> `leos-radio`).
- `--install` when used with `--target` will attempt to use picotool to load the firmware onto a board.

### Option 2: CMake

If building manually, be sure to initialize all the submodules with
```bash
git submodule update --init --recursive
```
From repository root:

```bash
cmake -S . -B build
cmake --build build -j
```

This generates board artifacts (including `.uf2`) in the build output directory `build/firmware`.

### Option 3: GitHub Action Artifacts

If you do not want to build locally and just need output files to load onto boards, use GitHub Actions. Each `master` commit runs a build environment to generate all the executables for download.

From the repository page, navigate to "Actions" and find the latest commit with a green checkmark. If your desired commit or version did not build successfully, then you must build manually or submit a fix for it :\)

On the Summary page for an action run, the artifacts section at the bottom has a download link for `firmware.zip` that has ".uf2" drag-n-drop files ready to be uploaded to Raspberry Pi Picos. Note: These firmwares are built for specific Pico boards that may change from semester to semester or per produced board.