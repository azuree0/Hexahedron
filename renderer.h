// GL 2 fixed-function: orbit, mesh, lighting.

#include <SFML/OpenGL.hpp>

#include "cube_mesh.h"
#include "cube_state.h"
#include "math3.h"

#include <array>

// Active quarter-turn for drawCube
struct TurnAnimCube {
    bool active = false;
    CubeFace face = CubeFace::Face1;
    int depth = 1;
    int dir = 1;
    float currentDeg = 0.f;
};

class Renderer {
public:
    Renderer();

    void initialize();
    void resize(int width, int height);
    void handleMouseWheel(int delta);
    void beginScene(float yawDeg, float pitchDeg, float rollDeg);
    void drawCube(const CubeMesh& mesh, const std::array<int, kStickerCount>& colors, float scale,
                  const TurnAnimCube* anim, float yawDeg, float pitchDeg, float rollDeg);

    void drawCubeWireframe(float scale, const TurnAnimCube* anim, CubeFace onlyFace) const;

    CubeFace dominantFacingFace(float yawDeg, float pitchDeg, float rollDeg) const;
    bool faceFacesCamera(CubeFace face, float yawDeg, float pitchDeg, float rollDeg) const;

    bool projectMeshAnchorToWindow(float meshScale, float yawDeg, float pitchDeg, float rollDeg, Vec3 meshLocal,
                                   const TurnAnimCube* anim, int viewportW, int viewportH, double& outPx, double& outPy,
                                   bool& outVisible, int stickerSlot = -1) const;

private:
    void drawStars();

    float cameraDistance_;
    static constexpr float kFovYDeg = 45.f;
    static constexpr float kCameraDistMin = 3.0f;
    static constexpr float kCameraDistMax = 16.f;
};
