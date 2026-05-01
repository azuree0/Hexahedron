#include "cube_mesh.h"

namespace {

void addQuad(std::array<MeshTriangle, kCubeTriCount>& tris, int& triWrite, const Vec3& v00, const Vec3& v10,
             const Vec3& v11, const Vec3& v01, const Vec3& n) {
    MeshTriangle t0{};
    t0.normal = n;
    t0.pos[0] = v00;
    t0.pos[1] = v10;
    t0.pos[2] = v11;
    tris[static_cast<std::size_t>(triWrite++)] = t0;
    MeshTriangle t1{};
    t1.normal = n;
    t1.pos[0] = v00;
    t1.pos[1] = v11;
    t1.pos[2] = v01;
    tris[static_cast<std::size_t>(triWrite++)] = t1;
}

void insetPair(float& a0, float& a1, float inset) {
    const float c = 0.5f * (a0 + a1);
    const float h = 0.5f * (a1 - a0) * inset;
    a0 = c - h;
    a1 = c + h;
}

Vec3 stickerQuadCenter(float H, CubeFace f, int r, int c) {
    constexpr float kInset = 0.91f;
    Vec3 v00{};
    Vec3 v10{};
    Vec3 v11{};
    Vec3 v01{};
    if (f == CubeFace::Face1) {
        float x0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
        float x1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
        float z0 = -H + 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
        float z1 = -H + 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
        insetPair(x0, x1, kInset);
        insetPair(z0, z1, kInset);
        v00 = {x0, H, z0};
        v10 = {x0, H, z1};
        v11 = {x1, H, z1};
        v01 = {x1, H, z0};
    } else if (f == CubeFace::Face2) {
        float x0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
        float x1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
        float z0 = -H + 2.f * H * static_cast<float>(kCubeOrder - 1 - r) / static_cast<float>(kCubeOrder);
        float z1 = -H + 2.f * H * static_cast<float>(kCubeOrder - r) / static_cast<float>(kCubeOrder);
        insetPair(x0, x1, kInset);
        insetPair(z0, z1, kInset);
        v00 = {x0, -H, z0};
        v10 = {x1, -H, z0};
        v11 = {x1, -H, z1};
        v01 = {x0, -H, z1};
    } else if (f == CubeFace::Face3) {
        float x0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
        float x1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
        float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
        float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
        insetPair(x0, x1, kInset);
        insetPair(y0, y1, kInset);
        v00 = {x0, y0, H};
        v10 = {x1, y0, H};
        v11 = {x1, y1, H};
        v01 = {x0, y1, H};
    } else if (f == CubeFace::Face4) {
        float x0 = H - 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
        float x1 = H - 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
        float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
        float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
        insetPair(x0, x1, kInset);
        insetPair(y0, y1, kInset);
        v00 = {x0, y0, -H};
        v10 = {x0, y1, -H};
        v11 = {x1, y1, -H};
        v01 = {x1, y0, -H};
    } else if (f == CubeFace::Face5) {
        float z0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
        float z1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
        float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
        float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
        insetPair(z0, z1, kInset);
        insetPair(y0, y1, kInset);
        v00 = {H, y0, z0};
        v10 = {H, y1, z0};
        v11 = {H, y1, z1};
        v01 = {H, y0, z1};
    } else {
        float z0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
        float z1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
        float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
        float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
        insetPair(z0, z1, kInset);
        insetPair(y0, y1, kInset);
        v00 = {-H, y0, z0};
        v10 = {-H, y1, z0};
        v11 = {-H, y1, z1};
        v01 = {-H, y0, z1};
    }
    return (v00 + v10 + v11 + v01) * 0.25f;
}

} // namespace

Vec3 stickerSlotCenterInMesh(float halfExtent, int slot) {
    if (slot < 0 || slot >= kStickerCount) {
        return {};
    }
    const int perFace = kCubeOrder * kCubeOrder;
    const int fi = slot / perFace;
    const int rem = slot % perFace;
    const int r = rem / kCubeOrder;
    const int c = rem % kCubeOrder;
    return stickerQuadCenter(halfExtent, static_cast<CubeFace>(fi), r, c);
}

CubeMesh buildCubeMesh(float halfExtent) {
    CubeMesh mesh{};
    int triWrite = 0;
    const float H = halfExtent;
    constexpr float kInset = 0.91f;

    for (int fi = 0; fi < kCubeFaces; ++fi) {
        const auto f = static_cast<CubeFace>(fi);
        for (int r = 0; r < kCubeOrder; ++r) {
            for (int c = 0; c < kCubeOrder; ++c) {
                if (f == CubeFace::Face1) {
                    float x0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float x1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float z0 = -H + 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    float z1 = -H + 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    insetPair(x0, x1, kInset);
                    insetPair(z0, z1, kInset);
                    const Vec3 n{0.f, 1.f, 0.f};
                    addQuad(mesh.triangles, triWrite, {x0, H, z0}, {x0, H, z1}, {x1, H, z1}, {x1, H, z0}, n);
                } else if (f == CubeFace::Face2) {
                    float x0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float x1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float z0 = -H + 2.f * H * static_cast<float>(kCubeOrder - 1 - r) / static_cast<float>(kCubeOrder);
                    float z1 = -H + 2.f * H * static_cast<float>(kCubeOrder - r) / static_cast<float>(kCubeOrder);
                    insetPair(x0, x1, kInset);
                    insetPair(z0, z1, kInset);
                    const Vec3 n{0.f, -1.f, 0.f};
                    addQuad(mesh.triangles, triWrite, {x0, -H, z0}, {x1, -H, z0}, {x1, -H, z1}, {x0, -H, z1}, n);
                } else if (f == CubeFace::Face3) {
                    float x0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float x1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    insetPair(x0, x1, kInset);
                    insetPair(y0, y1, kInset);
                    const Vec3 n{0.f, 0.f, 1.f};
                    addQuad(mesh.triangles, triWrite, {x0, y0, H}, {x1, y0, H}, {x1, y1, H}, {x0, y1, H}, n);
                } else if (f == CubeFace::Face4) {
                    float x0 = H - 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float x1 = H - 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    insetPair(x0, x1, kInset);
                    insetPair(y0, y1, kInset);
                    const Vec3 n{0.f, 0.f, -1.f};
                    addQuad(mesh.triangles, triWrite, {x0, y0, -H}, {x0, y1, -H}, {x1, y1, -H}, {x1, y0, -H}, n);
                } else if (f == CubeFace::Face5) {
                    float z0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float z1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    insetPair(z0, z1, kInset);
                    insetPair(y0, y1, kInset);
                    const Vec3 n{1.f, 0.f, 0.f};
                    addQuad(mesh.triangles, triWrite, {H, y0, z0}, {H, y1, z0}, {H, y1, z1}, {H, y0, z1}, n);
                } else if (f == CubeFace::Face6) {
                    float z0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float z1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    insetPair(z0, z1, kInset);
                    insetPair(y0, y1, kInset);
                    const Vec3 n{-1.f, 0.f, 0.f};
                    addQuad(mesh.triangles, triWrite, {-H, y0, z0}, {-H, y0, z1}, {-H, y1, z1}, {-H, y1, z0}, n);
                }
            }
        }
    }
    return mesh;
}
