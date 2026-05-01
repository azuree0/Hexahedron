// Puzzle state: sticker permutation / lattice topology (indices align with Geometry: cube_mesh); no lighting.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

// Grid width along one face edge (6x6 stickers per side).
inline constexpr int kCubeOrder = 6;
inline constexpr int kCubeFaces = 6;
inline constexpr int kStickerCount = kCubeOrder * kCubeOrder * kCubeFaces;
// Face14–17: quarter rings (4 × kCubeOrder stickers). Face18: same ring count + full Face4 sheet co-twist.
inline constexpr int kQuarterRingStickerCount = 4 * kCubeOrder;

// Face1–6: 1–6. Face7: Q. Face8–12: W–Y. Face13: A. Face14–17: S–G (rings). Face18: H — ring + Face4 sheet (109–144 1-based).
enum class CubeFace : std::uint8_t {
    Face1 = 0,
    Face2 = 1,
    Face3 = 2,
    Face4 = 3,
    Face5 = 4,
    Face6 = 5,
    Face7 = 6,
    Face8 = 7,
    Face9 = 8,
    Face10 = 9,
    Face11 = 10,
    Face12 = 11,
    Face13 = 12,
    Face14 = 13,
    Face15 = 14,
    Face16 = 15,
    Face17 = 16,
    Face18 = 17
};

// Row-major sticker index on the mesh for CubeFace::Face1..Face6 (must match CubeState::idx / cube_mesh).
inline int meshStickerSlot(CubeFace face, int row, int col) {
    return static_cast<int>(face) * kCubeOrder * kCubeOrder + row * kCubeOrder + col;
}

// One quarter-turn of a face slab: which face, how many layers from that side, and CW vs CCW.
struct CubeMove {
    CubeFace face = CubeFace::Face1;
    // 1 = outer layer ... 6 = whole cube (relative to that face).
    std::uint8_t depth = 1;
    // +1 = clockwise quarter turn when looking at that face from outside; -1 = CCW quarter.
    std::int8_t dir = 1;
};

// Permutation state for sticker colors with undo history and scramble helpers.
class CubeState {
public:
    CubeState();

    // Solved: each face shows one uniform palette index (0..5).
    void reset();
    // Apply one move and append it to undo history.
    void apply(const CubeMove& m);
    // Pop the last move and restore colors (inverse of apply).
    void undo();
    // Jump to solved; clears history.
    void autoComplete();
    // Random quarter turns then clear history so undo does not walk the scramble.
    void scramble(int moves, unsigned seed);

    // Palette index per sticker slot (face color in solved state).
    const std::array<int, kStickerCount>& colors() const { return color_; }

    // Collect sticker indices in the turning slab (for GL animation).
    static void stickersInSlab(CubeFace face, int depth, std::vector<int>& outIndices);

    // Map linear sticker index to 0..5 lattice coords (must match cube_mesh).
    static void slotToGrid(int slot, int& gx, int& gy, int& gz);
    // Inverse map from lattice coords to sticker index (-1 if interior).
    static int gridToSlot(int gx, int gy, int gz);

    // True when (gx,gy,gz) lies in the depth-th slab from the given face (used for GL animation tests).
    // slot: optional 0..215; Face13 uses it to hold Face5 col0 still while Face3 col5 turns (same lattice edge).
    static bool inSlab(CubeFace face, int depth, int gx, int gy, int gz, int slot = -1);

private:
    // Fill gridSlot_ from face (row,col) layout once per process.
    static void ensureTopology();
    // Rotate lattice coords by one quarter turn for that face and direction.
    static void rotateSlabCoords(CubeFace face, int dir, int& gx, int& gy, int& gz);

    std::array<int, kStickerCount> color_{};
    std::vector<CubeMove> history_;

    static std::array<std::array<std::array<int, kCubeOrder>, kCubeOrder>, kCubeOrder> gridSlot_;
    static bool topologyReady_;
};
