// 6×6 stickers / face; Renderer consumes normals.

#pragma once

#include "cube_state.h"
#include "math3.h"

#include <array>

inline constexpr int kCubeTriCount = kStickerCount * 2;

struct MeshTriangle {
    std::array<Vec3, 3> pos{};
    Vec3 normal{};
};

struct CubeMesh {
    std::array<MeshTriangle, kCubeTriCount> triangles{};
};

CubeMesh buildCubeMesh(float halfExtent);

Vec3 stickerSlotCenterInMesh(float halfExtent, int slot);
