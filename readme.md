# Prior

## Install

- **CMake 3.20+**               — https://cmake.org/download/
- **C++17** (MSVC)              — https://visualstudio.microsoft.com/downloads/
- **SFML 3.x**                  — https://www.sfml-dev.org/download.php
- **OpenGL**                    — `opengl32` on Windows (linked by CMake)

## Build

```text
cmake -B build -DSFML_ROOT=C:/SFML
cmake --build build --config Release
```

## Run

```text
.\build\Release\run.exe
```

# Function

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                                                                            │
│  Rules: 6 faces, 6x6 stickers each, 216 surface stickers total.            │
|                                                                            |
│  Goal: each face one uniform brown hue (6 browns when solved).             │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

# History

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                                                                            │
│  1974 — Erno Rubik prototypes a 3x3x3 hinged cube in Hungary.              │
│                                     ▼                                      │
│  1977 — Hungarian patent; working name Magic Cube.                         │
│                                     ▼                                      │
│  1980 — Ideal Toy exports Rubik's Cube; global puzzle craze.               │
│                                     ▼                                      │
│  1980s onward — higher-order NxNxN puzzles extend face and slice turns.    │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

# Structure

```text
├── main.cpp                        # (frontend)          # entry loop, diamond clip, hud, orbit, keyboard routing.
├── renderer.cpp                    # (frontend)          # stars, lighting, cube draw, animated slab rotation.
├── renderer.h                      # (frontend)          # declare renderer API and TurnAnimCube.
├── window_shape.cpp                # (frontend)          # win32 region: rhombus from client midpoints.
├── window_shape.h                  # (frontend)          # declare applyDiamondWindowShape.

├── cube_state.cpp                  # (backend)           # slab permutations, scramble, undo, lattice tables.
├── cube_state.h                    # (backend)           # CubeState, CubeMove, CubeFace constants.
├── cube_mesh.cpp                   # (backend)           # build 6x6 quads per face; sticker order matches state.
├── cube_mesh.h                     # (backend)           # CubeMesh, MeshTriangle, buildCubeMesh.
├── math3.h                         # (backend)           # vec3, normalize, rotateAroundAxis helpers.

├── CMakeLists.txt                  #                     # SFML 3, OpenGL, run target, post-build DLL copy.
├── .gitignore                      #                     # ignore build dirs, logs, IDE files.
└── readme.md                       #                     #
```
