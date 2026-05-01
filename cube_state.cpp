// Lattice addressing, slab masks, quarter-turn permutations (topology shared with Geometry: cube_mesh).

#include "cube_state.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

// Linear sticker index from face and local row, column (row-major within face).
inline int idx(CubeFace f, int r, int c) {
    return static_cast<int>(f) * kCubeOrder * kCubeOrder + r * kCubeOrder + c;
}

// Face8–Face12: same rule as Face8–9 — shell ring at gx==beltGx (column belts), slabs grow toward +X.
// Face13: Face5 col0 ksticker 145,151,… — held still (same lattice edge as Face3 col5).
inline bool face13Face5Column0Excluded(int slot) {
    if (slot < 0) {
        return false;
    }
    const int faceIdx = slot / (kCubeOrder * kCubeOrder);
    if (faceIdx != static_cast<int>(CubeFace::Face5)) {
        return false;
    }
    const int rem = slot % (kCubeOrder * kCubeOrder);
    return (rem % kCubeOrder) == 0;
}
// Face5 col5 — ksticker 150,156,…,180 (1-based); +X / low-z edge; in Face13 slab for animation/apply.
inline bool face13Face5Column5Included(int slot) {
    if (slot < 0) {
        return false;
    }
    const int faceIdx = slot / (kCubeOrder * kCubeOrder);
    if (faceIdx != static_cast<int>(CubeFace::Face5)) {
        return false;
    }
    const int rem = slot % (kCubeOrder * kCubeOrder);
    return (rem % kCubeOrder) == kCubeOrder - 1;
}

// Face14: ordered quarter-turn ring (0-based slots); ksticker 30–25, 185/191/…/215, 43–48, 179/173/…/149.
inline constexpr std::array<int, kQuarterRingStickerCount> kFace14RingSlots = {29,  28,  27,  26,  25,  24,   // Face1 row
                                                         184, 190, 196, 202, 208, 214, // Face6 col 4
                                                         42,  43,  44,  45,  46,  47,   // Face2 row
                                                         178, 172, 166, 160, 154, 148}; // Face5 col 4

inline bool face14RingContainsSlot(int slot) {
    if (slot < 0) {
        return false;
    }
    for (int s : kFace14RingSlots) {
        if (s == slot) {
            return true;
        }
    }
    return false;
}

inline bool face14RingContainsCoord(int gx, int gy, int gz) {
    for (int s : kFace14RingSlots) {
        int sx = 0;
        int sy = 0;
        int sz = 0;
        CubeState::slotToGrid(s, sx, sy, sz);
        if (sx == gx && sy == gy && sz == gz) {
            return true;
        }
    }
    return false;
}

inline void applyFace14RingPermute(std::array<int, kStickerCount>& colors, int dir) {
    std::array<int, kStickerCount> next = colors;
    for (int i = 0; i < kQuarterRingStickerCount; ++i) {
        const int dest = kFace14RingSlots[static_cast<std::size_t>(i)];
        const int srcIdx = (i - kCubeOrder * dir + 100 * kQuarterRingStickerCount) % kQuarterRingStickerCount;
        const int src = kFace14RingSlots[static_cast<std::size_t>(srcIdx)];
        next[static_cast<std::size_t>(dest)] = colors[static_cast<std::size_t>(src)];
    }
    colors = next;
}

// Face15: ksticker 24–19, 184/190/…/214, 49–54, 178/172/…/148 — same +Z quarter sense as Face14.
inline constexpr std::array<int, kQuarterRingStickerCount> kFace15RingSlots = {23,  22,  21,  20,  19,  18,   // Face1 row
                                                         183, 189, 195, 201, 207, 213, // Face6 col 4
                                                         48,  49,  50,  51,  52,  53,   // Face2 row
                                                         177, 171, 165, 159, 153, 147}; // Face5 col 4

inline bool face15RingContainsSlot(int slot) {
    if (slot < 0) {
        return false;
    }
    for (int s : kFace15RingSlots) {
        if (s == slot) {
            return true;
        }
    }
    return false;
}

inline bool face15RingContainsCoord(int gx, int gy, int gz) {
    for (int s : kFace15RingSlots) {
        int sx = 0;
        int sy = 0;
        int sz = 0;
        CubeState::slotToGrid(s, sx, sy, sz);
        if (sx == gx && sy == gy && sz == gz) {
            return true;
        }
    }
    return false;
}

inline void applyFace15RingPermute(std::array<int, kStickerCount>& colors, int dir) {
    std::array<int, kStickerCount> next = colors;
    for (int i = 0; i < kQuarterRingStickerCount; ++i) {
        const int dest = kFace15RingSlots[static_cast<std::size_t>(i)];
        const int srcIdx = (i - kCubeOrder * dir + 100 * kQuarterRingStickerCount) % kQuarterRingStickerCount;
        const int src = kFace15RingSlots[static_cast<std::size_t>(srcIdx)];
        next[static_cast<std::size_t>(dest)] = colors[static_cast<std::size_t>(src)];
    }
    colors = next;
}

// Face16 (F key): same ring topology as Face15, shifted one step (Face14→Face15→Face16): Face1 row up,
// Face6/Face5 one column toward +Z belt, Face2 row down — same +Z quarter sense / permute rule as Face15.
inline constexpr std::array<int, kQuarterRingStickerCount> kFace16RingSlots = {17,  16,  15,  14,  13,  12,   // Face1 row
                                                         182, 188, 194, 200, 206, 212, // Face6 (one column toward +Z vs Face15 belt)
                                                         54,  55,  56,  57,  58,  59,   // Face2 row
                                                         176, 170, 164, 158, 152, 146}; // Face5 (matching step vs Face15 belt)

inline bool face16RingContainsSlot(int slot) {
    if (slot < 0) {
        return false;
    }
    for (int s : kFace16RingSlots) {
        if (s == slot) {
            return true;
        }
    }
    return false;
}

inline bool face16RingContainsCoord(int gx, int gy, int gz) {
    for (int s : kFace16RingSlots) {
        int sx = 0;
        int sy = 0;
        int sz = 0;
        CubeState::slotToGrid(s, sx, sy, sz);
        if (sx == gx && sy == gy && sz == gz) {
            return true;
        }
    }
    return false;
}

inline void applyFace16RingPermute(std::array<int, kStickerCount>& colors, int dir) {
    std::array<int, kStickerCount> next = colors;
    for (int i = 0; i < kQuarterRingStickerCount; ++i) {
        const int dest = kFace16RingSlots[static_cast<std::size_t>(i)];
        const int srcIdx = (i - kCubeOrder * dir + 100 * kQuarterRingStickerCount) % kQuarterRingStickerCount;
        const int src = kFace16RingSlots[static_cast<std::size_t>(srcIdx)];
        next[static_cast<std::size_t>(dest)] = colors[static_cast<std::size_t>(src)];
    }
    colors = next;
}

// Face17 (G key): same ring shift as Face15→Face16 applied to Face16 — Face1 r=1, Face6/Face5 c=1, Face2 r=4.
inline constexpr std::array<int, kQuarterRingStickerCount> kFace17RingSlots = {11,  10,  9,  8,  7,  6,   // Face1 row
                                                         181, 187, 193, 199, 205, 211, // Face6
                                                         60,  61,  62,  63,  64,  65,   // Face2 row
                                                         175, 169, 163, 157, 151, 145}; // Face5

inline bool face17RingContainsSlot(int slot) {
    if (slot < 0) {
        return false;
    }
    for (int s : kFace17RingSlots) {
        if (s == slot) {
            return true;
        }
    }
    return false;
}

inline bool face17RingContainsCoord(int gx, int gy, int gz) {
    for (int s : kFace17RingSlots) {
        int sx = 0;
        int sy = 0;
        int sz = 0;
        CubeState::slotToGrid(s, sx, sy, sz);
        if (sx == gx && sy == gy && sz == gz) {
            return true;
        }
    }
    return false;
}

inline void applyFace17RingPermute(std::array<int, kStickerCount>& colors, int dir) {
    std::array<int, kStickerCount> next = colors;
    for (int i = 0; i < kQuarterRingStickerCount; ++i) {
        const int dest = kFace17RingSlots[static_cast<std::size_t>(i)];
        const int srcIdx = (i - kCubeOrder * dir + 100 * kQuarterRingStickerCount) % kQuarterRingStickerCount;
        const int src = kFace17RingSlots[static_cast<std::size_t>(srcIdx)];
        next[static_cast<std::size_t>(dest)] = colors[static_cast<std::size_t>(src)];
    }
    colors = next;
}

// Face18 (H key): Face16→Face17 shift ring + full Face4 sheet (kstickers 109–144, 1-based); all use same +Z sense.
inline constexpr std::array<int, kQuarterRingStickerCount> kFace18RingSlots = {5,   4,   3,   2,   1,   0,   // Face1 row
                                                        180, 186, 192, 198, 204, 210, // Face6
                                                        66,  67,  68,  69,  70,  71,   // Face2 row
                                                        174, 168, 162, 156, 150, 144}; // Face5

inline bool face18RingContainsSlot(int slot) {
    if (slot < 0) {
        return false;
    }
    for (int s : kFace18RingSlots) {
        if (s == slot) {
            return true;
        }
    }
    return false;
}

inline bool face18RingContainsCoord(int gx, int gy, int gz) {
    for (int s : kFace18RingSlots) {
        int sx = 0;
        int sy = 0;
        int sz = 0;
        CubeState::slotToGrid(s, sx, sy, sz);
        if (sx == gx && sy == gy && sz == gz) {
            return true;
        }
    }
    return false;
}

inline void applyFace18RingPermute(std::array<int, kStickerCount>& colors, int dir) {
    std::array<int, kStickerCount> next = colors;
    for (int i = 0; i < kQuarterRingStickerCount; ++i) {
        const int dest = kFace18RingSlots[static_cast<std::size_t>(i)];
        const int srcIdx = (i - kCubeOrder * dir + 100 * kQuarterRingStickerCount) % kQuarterRingStickerCount;
        const int src = kFace18RingSlots[static_cast<std::size_t>(srcIdx)];
        next[static_cast<std::size_t>(dest)] = colors[static_cast<std::size_t>(src)];
    }
    colors = next;
}

// Face4 stickers (linear 108–143): turn with Face18 under rotateSlabCoords (+Z), disjoint from kFace18RingSlots.
inline bool face18Face4SheetSlot(int slot) {
    const int lo = meshStickerSlot(CubeFace::Face4, 0, 0);
    const int hi = meshStickerSlot(CubeFace::Face4, kCubeOrder - 1, kCubeOrder - 1);
    return slot >= lo && slot <= hi;
}

inline bool xShellBeltInSlab(int depth, int gx, int gy, int gz, int beltGx) {
    if (depth >= kCubeOrder) {
        return true;
    }
    const int xHi = std::min(kCubeOrder - 1, beltGx + depth - 1);
    if (gx < beltGx || gx > xHi) {
        return false;
    }
    const int yMax = kCubeOrder - 1;
    return (gy == 0 || gy == yMax || gz == 0 || gz == yMax);
}

// Build (gx,gy,gz) for slot on surface; must match cube_mesh mapping.
void faceRcToGrid(CubeFace f, int r, int c, int& gx, int& gy, int& gz) {
    switch (f) {
    case CubeFace::Face1:
        gx = c;
        gy = kCubeOrder - 1;
        gz = r;
        break;
    case CubeFace::Face2:
        gx = c;
        gy = 0;
        gz = kCubeOrder - 1 - r;
        break;
    case CubeFace::Face3:
        gx = c;
        gy = kCubeOrder - 1 - r;
        gz = kCubeOrder - 1;
        break;
    case CubeFace::Face4:
        gx = kCubeOrder - 1 - c;
        gy = kCubeOrder - 1 - r;
        gz = 0;
        break;
    case CubeFace::Face5:
        gx = kCubeOrder - 1;
        gz = kCubeOrder - 1 - c;
        gy = kCubeOrder - 1 - r;
        break;
    case CubeFace::Face6:
        gx = 0;
        gz = c;
        gy = kCubeOrder - 1 - r;
        break;
    case CubeFace::Face7:
        gx = 0;
        gz = c;
        gy = kCubeOrder - 1 - r;
        break;
    case CubeFace::Face8:
        gx = 1;
        gz = c;
        gy = kCubeOrder - 1 - r;
        break;
    case CubeFace::Face9:
        gx = 2;
        gz = c;
        gy = kCubeOrder - 1 - r;
        break;
    case CubeFace::Face10:
        gx = 3;
        gz = c;
        gy = kCubeOrder - 1 - r;
        break;
    case CubeFace::Face11:
        gx = 4;
        gz = c;
        gy = kCubeOrder - 1 - r;
        break;
    case CubeFace::Face12:
        gx = kCubeOrder - 1;
        gz = c;
        gy = kCubeOrder - 1 - r;
        break;
    case CubeFace::Face13:
        // Same lattice as Face3 (+Z surface); ksticker 73–108 (1-based).
        gx = c;
        gy = kCubeOrder - 1 - r;
        gz = kCubeOrder - 1;
        break;
    }
}

} // namespace

std::array<std::array<std::array<int, kCubeOrder>, kCubeOrder>, kCubeOrder> CubeState::gridSlot_{};
bool CubeState::topologyReady_ = false;

// Initialize gridSlot_ so each surface cell maps to exactly 1 of 216 stickers.
void CubeState::ensureTopology() {
    if (topologyReady_) {
        return;
    }
    for (int x = 0; x < kCubeOrder; ++x) {
        for (int y = 0; y < kCubeOrder; ++y) {
            for (int z = 0; z < kCubeOrder; ++z) {
                gridSlot_[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)][static_cast<std::size_t>(z)] =
                    -1;
            }
        }
    }
    for (int fi = 0; fi < kCubeFaces; ++fi) {
        const auto f = static_cast<CubeFace>(fi);
        for (int r = 0; r < kCubeOrder; ++r) {
            for (int c = 0; c < kCubeOrder; ++c) {
                int gx = 0;
                int gy = 0;
                int gz = 0;
                faceRcToGrid(f, r, c, gx, gy, gz);
                gridSlot_[static_cast<std::size_t>(gx)][static_cast<std::size_t>(gy)][static_cast<std::size_t>(gz)] =
                    idx(f, r, c);
            }
        }
    }
    topologyReady_ = true;
}

// Decode a flat sticker index into lattice coordinates on the 6x6x6 grid.
void CubeState::slotToGrid(int slot, int& gx, int& gy, int& gz) {
    ensureTopology();
    const int f = slot / (kCubeOrder * kCubeOrder);
    const int rem = slot % (kCubeOrder * kCubeOrder);
    const int r = rem / kCubeOrder;
    const int c = rem % kCubeOrder;
    faceRcToGrid(static_cast<CubeFace>(f), r, c, gx, gy, gz);
}

// Look up sticker index for a lattice cell (interior cells remain -1 in the table).
int CubeState::gridToSlot(int gx, int gy, int gz) {
    ensureTopology();
    return gridSlot_[static_cast<std::size_t>(gx)][static_cast<std::size_t>(gy)][static_cast<std::size_t>(gz)];
}

// Depth selects how many layers from the turning face participate (outer layer is depth 1).
bool CubeState::inSlab(CubeFace face, int depth, int gx, int gy, int gz, int slot) {
    if (depth < 1) {
        depth = 1;
    }
    if (depth > kCubeOrder) {
        depth = kCubeOrder;
    }
    const int lo = kCubeOrder - depth;
    switch (face) {
    case CubeFace::Face1:
        return gy >= lo;
    case CubeFace::Face2:
        // Bottom face (gy==0); mirror Face1: outer layer first, then include higher y (same rotY axis).
        if (depth >= kCubeOrder) {
            return true;
        }
        return gy < depth;
    case CubeFace::Face3:
        if (depth >= kCubeOrder) {
            return true;
        }
        {
            const int hi = kCubeOrder - 3;
            const int bandLo = std::max(0, hi + 1 - depth);
            return gy >= bandLo && gy <= hi;
        }
    case CubeFace::Face4:
        if (depth >= kCubeOrder) {
            return true;
        }
        {
            const int hi = kCubeOrder - 4;
            const int bandLo = std::max(0, hi + 1 - depth);
            return gy >= bandLo && gy <= hi;
        }
    case CubeFace::Face5:
        if (depth >= kCubeOrder) {
            return true;
        }
        {
            const int hi = kCubeOrder - 5;
            const int bandLo = std::max(0, hi + 1 - depth);
            return gy >= bandLo && gy <= hi;
        }
    case CubeFace::Face6:
        if (depth >= kCubeOrder) {
            return true;
        }
        return gy < depth;
    case CubeFace::Face7:
        // Q key; ksticker 181–216 (Face6 sheet, 1-based); −X cap, slabs grow toward +X.
        if (depth >= kCubeOrder) {
            return true;
        }
        return gx < depth;
    case CubeFace::Face8:
        // gx==1 (W); Face10/11/12 shift one lattice column toward +X (R,T,Y keys).
        return xShellBeltInSlab(depth, gx, gy, gz, 1);
    case CubeFace::Face9:
        return xShellBeltInSlab(depth, gx, gy, gz, 2);
    case CubeFace::Face10:
        return xShellBeltInSlab(depth, gx, gy, gz, 3);
    case CubeFace::Face11:
        return xShellBeltInSlab(depth, gx, gy, gz, 4);
    case CubeFace::Face12:
        return xShellBeltInSlab(depth, gx, gy, gz, 5);
    case CubeFace::Face13:
        // A key; +Z cap. Exclude Face5 col0 (145,151,…). Face5 col5 (150,156,…,180) joins Face13 (low-z +X edge).
        // Face3 col5 (78,84,…) via gz depth band.
        if (face13Face5Column0Excluded(slot)) {
            return false;
        }
        if (face13Face5Column5Included(slot)) {
            return true;
        }
        if (depth >= kCubeOrder) {
            return true;
        }
        return gz >= kCubeOrder - depth;
    case CubeFace::Face14:
        // S key; fixed kQuarterRingStickerCount kstickers — quarter shift along kFace14RingSlots (depth unused).
        if (slot >= 0) {
            return face14RingContainsSlot(slot);
        }
        return face14RingContainsCoord(gx, gy, gz);
    case CubeFace::Face15:
        // D key; fixed kQuarterRingStickerCount kstickers — same +Z quarter convention as Face14.
        if (slot >= 0) {
            return face15RingContainsSlot(slot);
        }
        return face15RingContainsCoord(gx, gy, gz);
    case CubeFace::Face16:
        // F key; fixed kQuarterRingStickerCount kstickers — same +Z convention and quarter cycle as Face15.
        if (slot >= 0) {
            return face16RingContainsSlot(slot);
        }
        return face16RingContainsCoord(gx, gy, gz);
    case CubeFace::Face17:
        // G key; fixed kQuarterRingStickerCount kstickers — same +Z convention as Face16 ring.
        if (slot >= 0) {
            return face17RingContainsSlot(slot);
        }
        return face17RingContainsCoord(gx, gy, gz);
    case CubeFace::Face18:
        // H key: quarter ring + full Face4 (109–144 1-based); same +Z twist as Face13–17.
        if (slot >= 0) {
            if (face18RingContainsSlot(slot)) {
                return true;
            }
            return face18Face4SheetSlot(slot);
        }
        if (face18RingContainsCoord(gx, gy, gz)) {
            return true;
        }
        {
            const int gSlot = CubeState::gridToSlot(gx, gy, gz);
            return gSlot >= 0 && face18Face4SheetSlot(gSlot);
        }
    }
    return false;
}

// Apply a 90-degree rotation in the plane orthogonal to the face axis (dir = +/-1 quarter).
void CubeState::rotateSlabCoords(CubeFace face, int dir, int& gx, int& gy, int& gz) {
    constexpr float kCenter = (kCubeOrder - 1) * 0.5f;
    float vx = static_cast<float>(gx) - kCenter;
    float vy = static_cast<float>(gy) - kCenter;
    float vz = static_cast<float>(gz) - kCenter;

    const float s = static_cast<float>(dir) * 1.57079632679f; // +90 deg * dir, CW from outside

    auto rotY = [&](float ang) {
        const float c = std::cos(ang);
        const float si = std::sin(ang);
        const float nx = vx * c + vz * si;
        const float nz = -vx * si + vz * c;
        vx = nx;
        vz = nz;
    };
    auto rotX = [&](float ang) {
        const float c = std::cos(ang);
        const float si = std::sin(ang);
        const float ny = vy * c - vz * si;
        const float nz = vy * si + vz * c;
        vy = ny;
        vz = nz;
    };
    auto rotZ = [&](float ang) {
        const float c = std::cos(ang);
        const float si = std::sin(ang);
        const float nx = vx * c - vy * si;
        const float ny = vx * si + vy * c;
        vx = nx;
        vy = ny;
    };

    if (face == CubeFace::Face7) {
        rotX(s);
    } else if (face == CubeFace::Face8 || face == CubeFace::Face9 || face == CubeFace::Face10 ||
               face == CubeFace::Face11 || face == CubeFace::Face12) {
        // Same +dir sense as Face2-style moves; axis +X (quarter turn in yz-plane).
        rotX(s);
    } else if (face == CubeFace::Face13 || face == CubeFace::Face14 || face == CubeFace::Face15 ||
               face == CubeFace::Face16 || face == CubeFace::Face17 || face == CubeFace::Face18) {
        // +Z axis; quarter-turn in xy-plane (Face14–18: custom rings, same dir sense as Face13).
        rotZ(s);
    } else {
        rotY(s);
    }

    gx = static_cast<int>(std::lround(vx + kCenter));
    gy = static_cast<int>(std::lround(vy + kCenter));
    gz = static_cast<int>(std::lround(vz + kCenter));
    gx = std::max(0, std::min(kCubeOrder - 1, gx));
    gy = std::max(0, std::min(kCubeOrder - 1, gy));
    gz = std::max(0, std::min(kCubeOrder - 1, gz));
}

// List all sticker indices whose lattice points fall inside the slab for animation masking.
void CubeState::stickersInSlab(CubeFace face, int depth, std::vector<int>& outIndices) {
    ensureTopology();
    outIndices.clear();
    for (int i = 0; i < kStickerCount; ++i) {
        int gx = 0;
        int gy = 0;
        int gz = 0;
        slotToGrid(i, gx, gy, gz);
        if (inSlab(face, depth, gx, gy, gz, i)) {
            outIndices.push_back(i);
        }
    }
}

// Build topology tables then start solved.
CubeState::CubeState() {
    ensureTopology();
    reset();
}

// Assign each face a uniform color index and drop undo history.
void CubeState::reset() {
    for (int f = 0; f < kCubeFaces; ++f) {
        for (int i = 0; i < kCubeOrder * kCubeOrder; ++i) {
            color_[static_cast<std::size_t>(f * kCubeOrder * kCubeOrder + i)] = f;
        }
    }
    history_.clear();
}

// Move stickers in the slab by rotating their lattice coords; records the move for undo.
void CubeState::apply(const CubeMove& m) {
    if (m.face == CubeFace::Face14) {
        applyFace14RingPermute(color_, static_cast<int>(m.dir));
        history_.push_back(m);
        return;
    }
    if (m.face == CubeFace::Face15) {
        applyFace15RingPermute(color_, static_cast<int>(m.dir));
        history_.push_back(m);
        return;
    }
    if (m.face == CubeFace::Face16) {
        applyFace16RingPermute(color_, static_cast<int>(m.dir));
        history_.push_back(m);
        return;
    }
    if (m.face == CubeFace::Face17) {
        applyFace17RingPermute(color_, static_cast<int>(m.dir));
        history_.push_back(m);
        return;
    }
    if (m.face == CubeFace::Face18) {
        applyFace18RingPermute(color_, static_cast<int>(m.dir));
        const int f4Lo = meshStickerSlot(CubeFace::Face4, 0, 0);
        const int f4Hi = meshStickerSlot(CubeFace::Face4, kCubeOrder - 1, kCubeOrder - 1);
        std::array<int, kStickerCount> sheet = color_;
        for (int s = f4Lo; s <= f4Hi; ++s) {
            int gx = 0;
            int gy = 0;
            int gz = 0;
            slotToGrid(s, gx, gy, gz);
            int tx = gx;
            int ty = gy;
            int tz = gz;
            rotateSlabCoords(CubeFace::Face18, static_cast<int>(m.dir), tx, ty, tz);
            const int dest = gridToSlot(tx, ty, tz);
            sheet[static_cast<std::size_t>(dest)] = color_[static_cast<std::size_t>(s)];
        }
        color_ = sheet;
        history_.push_back(m);
        return;
    }
    std::array<int, kStickerCount> next = color_;
    for (int s = 0; s < kStickerCount; ++s) {
        int gx = 0;
        int gy = 0;
        int gz = 0;
        slotToGrid(s, gx, gy, gz);
        if (!inSlab(m.face, m.depth, gx, gy, gz, s)) {
            continue;
        }
        int tx = gx;
        int ty = gy;
        int tz = gz;
        rotateSlabCoords(m.face, static_cast<int>(m.dir), tx, ty, tz);
        const int dest = gridToSlot(tx, ty, tz);
        next[static_cast<std::size_t>(dest)] = color_[static_cast<std::size_t>(s)];
    }
    color_ = next;
    history_.push_back(m);
}

// Reverse the last recorded move by pulling colors back along the forward rotation map.
void CubeState::undo() {
    if (history_.empty()) {
        return;
    }
    const CubeMove m = history_.back();
    history_.pop_back();
    if (m.face == CubeFace::Face14) {
        applyFace14RingPermute(color_, -static_cast<int>(m.dir));
        return;
    }
    if (m.face == CubeFace::Face15) {
        applyFace15RingPermute(color_, -static_cast<int>(m.dir));
        return;
    }
    if (m.face == CubeFace::Face16) {
        applyFace16RingPermute(color_, -static_cast<int>(m.dir));
        return;
    }
    if (m.face == CubeFace::Face17) {
        applyFace17RingPermute(color_, -static_cast<int>(m.dir));
        return;
    }
    if (m.face == CubeFace::Face18) {
        const int f4Lo = meshStickerSlot(CubeFace::Face4, 0, 0);
        const int f4Hi = meshStickerSlot(CubeFace::Face4, kCubeOrder - 1, kCubeOrder - 1);
        std::array<int, kStickerCount> sheet = color_;
        for (int s = f4Lo; s <= f4Hi; ++s) {
            int gx = 0;
            int gy = 0;
            int gz = 0;
            slotToGrid(s, gx, gy, gz);
            int tx = gx;
            int ty = gy;
            int tz = gz;
            rotateSlabCoords(CubeFace::Face18, -static_cast<int>(m.dir), tx, ty, tz);
            const int src = gridToSlot(tx, ty, tz);
            sheet[static_cast<std::size_t>(s)] = color_[static_cast<std::size_t>(src)];
        }
        color_ = sheet;
        applyFace18RingPermute(color_, -static_cast<int>(m.dir));
        return;
    }
    std::array<int, kStickerCount> next = color_;
    for (int s = 0; s < kStickerCount; ++s) {
        int gx = 0;
        int gy = 0;
        int gz = 0;
        slotToGrid(s, gx, gy, gz);
        if (!inSlab(m.face, m.depth, gx, gy, gz, s)) {
            continue;
        }
        int tx = gx;
        int ty = gy;
        int tz = gz;
        rotateSlabCoords(m.face, static_cast<int>(m.dir), tx, ty, tz);
        const int src = gridToSlot(tx, ty, tz);
        next[static_cast<std::size_t>(s)] = color_[static_cast<std::size_t>(src)];
    }
    color_ = next;
}

// Alias for reset(): jump to the solved coloring.
void CubeState::autoComplete() {
    reset();
}

// Apply random moves for mixing; clears history afterward so undo does not unwind the scramble.
void CubeState::scramble(int moves, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> faceDist(0, static_cast<int>(CubeFace::Face18));
    std::uniform_int_distribution<int> depthDist(1, kCubeOrder);
    std::uniform_int_distribution<int> dirDist(0, 1);
    for (int i = 0; i < moves; ++i) {
        CubeMove m{};
        m.face = static_cast<CubeFace>(faceDist(rng));
        m.depth = static_cast<std::uint8_t>(depthDist(rng));
        m.dir = dirDist(rng) == 0 ? static_cast<std::int8_t>(1) : static_cast<std::int8_t>(-1);
        apply(m);
    }
    history_.clear();
}
