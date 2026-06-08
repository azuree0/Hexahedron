<img width="869" height="1024" alt="H" src="https://github.com/user-attachments/assets/3f656e97-cb66-41db-bf0a-be252f75d8e6" />

# Prior

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
│  Goal: each face one uniform brown hues.                                   │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

# History

```text
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  400 BCE — ANCIENT GREECE                                                │
  │  Plato links 5 regular solids to elements; cube (“earth”) = hexahedron   │
  │  Mathematical study of polyhedra; later Euclid, Elements (~300 BCE)      │
  └───────────────────────────────────┬──────────────────────────────────────┘
                                      │
                                      v
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  200 BCE — 1400 CE — CLASSICAL / MEDIEVAL                                │
  │  Greek math via commentators & Persia scholars; solid geometry preserved │
  └───────────────────────────────────┬──────────────────────────────────────┘
                                      │
                                      v
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  1400s — 1600s — RENAISSANCE                                             │
  │  Polyhedra in art & architecture; perspective; coords (Descartes, 1600s) │
  └───────────────────────────────────┬──────────────────────────────────────┘
                                      │
                                      v
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  1800s — 1900s — MODERN MATH & SCIENCE                                   │
  │  Crystallography (cubic systems); group theory — symmetries of the cube  │
  └───────────────────────────────────┬──────────────────────────────────────┘
                                      │
                                      v
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  1900s — 2000s — COMPUTATION & PLAY                                      │
  │  3D graphics (meshes, linear algebra)                                    │
  └───────────────────────────────────┬──────────────────────────────────────┘
                                      │
                                      v
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  TODAY                                                                   │
  │  Real-time rendering, engines, STEM tools                                │
  └──────────────────────────────────────────────────────────────────────────┘
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
