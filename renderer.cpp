// Bronze-style materials on brown palette indices; starfield matches Tetrahedron reference tone.
//
// Geometry: camera/orbit matrices, mesh transforms, slab twist, projections, wireframe paths.
// Lighting: fixed-function lights, sticker materials (palette + emission), unlit star shell.

#include "renderer.h"

#include "math3.h"

#include <GL/glu.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

// Lighting: HSV → RGB for sticker diffuse materials (degrees).
void hsvToRgb(float hDeg, float s, float v, float rgb[3]) {
    hDeg = std::fmod(hDeg, 360.f);
    if (hDeg < 0.f) {
        hDeg += 360.f;
    }
    const float c = v * s;
    const float x = c * (1.f - std::fabs(std::fmod(hDeg / 60.f, 2.f) - 1.f));
    const float m = v - c;
    float rp = 0.f;
    float gp = 0.f;
    float bp = 0.f;
    const int sector = static_cast<int>(hDeg / 60.f);
    switch (sector) {
    case 0:
        rp = c;
        gp = x;
        break;
    case 1:
        rp = x;
        gp = c;
        break;
    case 2:
        gp = c;
        bp = x;
        break;
    case 3:
        gp = x;
        bp = c;
        break;
    case 4:
        rp = x;
        bp = c;
        break;
    default:
        rp = c;
        bp = x;
        break;
    }
    rgb[0] = std::clamp(rp + m, 0.f, 1.f);
    rgb[1] = std::clamp(gp + m, 0.f, 1.f);
    rgb[2] = std::clamp(bp + m, 0.f, 1.f);
}

// Lighting: diffuse RGBA for palette index 0..5 — bronze → chestnut → cocoa → copper → wheat (+ pine wood slot).
void diffuseBrown(int face, float out[4]) {
    const int f = face % 6;
    static const struct {
        float hDeg;
        float s;
        float v;
    } kBrownHues[6] = {
        {36.f, 0.40f, 0.50f}, // face 1 — pine wood
        {26.f, 0.52f, 0.50f}, // face 2 — bronze
        {16.f, 0.58f, 0.50f}, // face 3 — copper
        {42.f, 0.38f, 0.50f}, // face 4 — wheat
        {22.f, 0.52f, 0.50f}, // face 5 — chestnut
        {28.f, 0.48f, 0.50f}, // face 6 — cocoa
    };
    const auto& b = kBrownHues[f];
    hsvToRgb(b.hDeg, b.s, b.v, out);
    out[3] = 1.0f;
}

// Geometry: degrees ↔ radians for twist/orbit math.
float degToRad(float d) {
    return d * static_cast<float>(M_PI / 180.0);
}

// Lighting: framebuffer clear, depth test, shade model, global ambient (fixed-function lighting pipeline).
void setupFramebufferAndShadeModel() {
    glDisable(GL_CULL_FACE);
    glDisable(GL_COLOR_MATERIAL);
    // drawCube uses glScalef on lit geometry; without this, normals stay non-unit and diffuse can vanish
    // at many view angles (fixed-function lighting uses transformed normals as-is).
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glShadeModel(GL_SMOOTH);

    const float globalAmb[4] = {0.065f, 0.062f, 0.058f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    // Local viewer improves specular on faces toward the camera (lateral stickers were matte despite fills).
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
}

// Lighting: directional key/fill + eye headlight (POSITION each frame); LIGHT3-6 under identity MV (eye dirs).
void setupKeyLight() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    const float keyAmb[4] = {0.07f, 0.06f, 0.055f, 1.0f};
    const float keyDiff[4] = {1.35f, 1.25f, 1.15f, 1.0f};
    const float keySpec[4] = {1.55f, 1.45f, 1.35f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, keyAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, keyDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, keySpec);

    const float fillAmb[4] = {0.f, 0.f, 0.f, 1.0f};
    const float fillDiff[4] = {0.48f, 0.44f, 0.38f, 1.0f};
    const float fillSpec[4] = {0.12f, 0.11f, 0.10f, 1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT, fillAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fillDiff);
    glLightfv(GL_LIGHT1, GL_SPECULAR, fillSpec);

    const float z[4] = {0.f, 0.f, 0.f, 1.0f};
    for (GLenum li = GL_LIGHT2; li <= GL_LIGHT6; ++li) {
        glEnable(li);
        glLightfv(li, GL_AMBIENT, z);
        glLightfv(li, GL_DIFFUSE, z);
        glLightfv(li, GL_SPECULAR, z);
    }
}

// Lighting: unlit star shell (glColor). Geometry: points on a large sphere (deterministic RNG from caller).
void emitStarPointsOnSphere(int count, float pointSize, float cr, float cg, float cb) {
    glPointSize(pointSize);
    glColor3f(cr, cg, cb);
    for (int i = 0; i < count; ++i) {
        const float theta = static_cast<float>(std::rand() % 628) / 100.0f;
        const float phi = static_cast<float>(std::rand() % 314) / 100.0f;
        constexpr float radius = 50.0f;
        const float x = radius * std::sin(phi) * std::cos(theta);
        const float y = radius * std::sin(phi) * std::sin(theta);
        const float z = radius * std::cos(phi);
        glVertex3f(x, y, z);
    }
}

// Geometry: +Y twists match Face2 sense; Face7–Face12 +X; Face13–18 +Z (Face14–18 custom rings).
void animAxisAngle(CubeFace f, int dir, float currentDeg, Vec3& axisOut, float& angRadOut) {
    const float rad = degToRad(currentDeg);
    angRadOut = static_cast<float>(dir) * rad;
    if (f == CubeFace::Face7 || f == CubeFace::Face8 || f == CubeFace::Face9 || f == CubeFace::Face10 ||
        f == CubeFace::Face11 || f == CubeFace::Face12) {
        axisOut = {1.f, 0.f, 0.f};
    } else if (f == CubeFace::Face13 || f == CubeFace::Face14 || f == CubeFace::Face15 ||
               f == CubeFace::Face16 || f == CubeFace::Face17 || f == CubeFace::Face18) {
        axisOut = {0.f, 0.f, 1.f};
    } else {
        axisOut = {0.f, 1.f, 0.f};
    }
}

// Geometry: Rodrigues twist for animated verts/normals (small angles no-op).
Vec3 rotateAnim(Vec3 v, const Vec3& axis, float angRad) {
    if (std::fabs(angRad) < 1e-6f) {
        return v;
    }
    return rotateAroundAxis(v, normalize(axis), angRad);
}

// Geometry: mesh [-1,1] coords ↔ lattice indices for slab membership (surface voxel centers).
int meshCoordToLattice(float u) {
    int g = static_cast<int>(std::floor((u + 1.f) * 0.5f * static_cast<float>(kCubeOrder)));
    return std::clamp(g, 0, kCubeOrder - 1);
}

// Geometry: apply active turn to mesh-space points (same slab as drawCube).
Vec3 transformMeshPointForTurn(Vec3 p, const TurnAnimCube* anim, int stickerSlot = -1) {
    if (anim == nullptr || !anim->active) {
        return p;
    }
    const int gx = meshCoordToLattice(p.x);
    const int gy = meshCoordToLattice(p.y);
    const int gz = meshCoordToLattice(p.z);
    if (!CubeState::inSlab(anim->face, anim->depth, gx, gy, gz, stickerSlot)) {
        return p;
    }
    Vec3 axis{0.f, 1.f, 0.f};
    float angRad = 0.f;
    animAxisAngle(anim->face, anim->dir, anim->currentDeg, axis, angRad);
    return rotateAnim(p, axis, angRad);
}

// Geometry: sticker inset intervals (matches buildCubeMesh in cube_mesh.cpp).
void insetStickerRange(float& a0, float& a1, float inset) {
    const float c = 0.5f * (a0 + a1);
    const float h = 0.5f * (a1 - a0) * inset;
    a0 = c - h;
    a1 = c + h;
}

// Geometry: sticker-outline segments (vertex order matches addQuad in cube_mesh.cpp).
void emitStickerQuadOutline(const Vec3& v00, const Vec3& v10, const Vec3& v11, const Vec3& v01,
                            const TurnAnimCube* anim, int stickerSlot = -1) {
    const Vec3 t00 = transformMeshPointForTurn(v00, anim, stickerSlot);
    const Vec3 t10 = transformMeshPointForTurn(v10, anim, stickerSlot);
    const Vec3 t11 = transformMeshPointForTurn(v11, anim, stickerSlot);
    const Vec3 t01 = transformMeshPointForTurn(v01, anim, stickerSlot);
    glVertex3f(t00.x, t00.y, t00.z);
    glVertex3f(t10.x, t10.y, t10.z);
    glVertex3f(t10.x, t10.y, t10.z);
    glVertex3f(t11.x, t11.y, t11.z);
    glVertex3f(t11.x, t11.y, t11.z);
    glVertex3f(t01.x, t01.y, t01.z);
    glVertex3f(t01.x, t01.y, t01.z);
    glVertex3f(t00.x, t00.y, t00.z);
}

// Geometry: face outward normals in mesh space.
Vec3 outwardNormalMesh(CubeFace f) {
    switch (f) {
    case CubeFace::Face1:
        return {0.f, 1.f, 0.f};
    case CubeFace::Face2:
        return {0.f, -1.f, 0.f};
    case CubeFace::Face3:
        return {0.f, 0.f, 1.f};
    case CubeFace::Face4:
        return {0.f, 0.f, -1.f};
    case CubeFace::Face5:
        return {1.f, 0.f, 0.f};
    case CubeFace::Face6:
        return {-1.f, 0.f, 0.f};
    case CubeFace::Face7:
        return {-1.f, 0.f, 0.f};
    case CubeFace::Face8:
    case CubeFace::Face9:
    case CubeFace::Face10:
    case CubeFace::Face11:
    case CubeFace::Face12:
        return {1.f, 0.f, 0.f};
    case CubeFace::Face13:
    case CubeFace::Face14:
    case CubeFace::Face15:
    case CubeFace::Face16:
    case CubeFace::Face17:
    case CubeFace::Face18:
        return {0.f, 0.f, 1.f};
    }
    return {0.f, 1.f, 0.f};
}

// Geometry: orbit maps mesh normals into eye space (same composition as beginScene MODELVIEW rotation).
// beginScene does glRotate pitch(X), yaw(Y), roll(Z) after translate — column-vector MV builds R_x R_y R_z on
// vectors as v' = R_x * R_y * R_z * v, i.e. apply roll(Z), then yaw(Y), then pitch(X).
Vec3 orbitRotateDirection(Vec3 v, float yawDeg, float pitchDeg, float rollDeg) {
    Vec3 v1 = rotateAroundAxis(v, {0.f, 0.f, 1.f}, degToRad(rollDeg));
    Vec3 v2 = rotateAroundAxis(v1, {0.f, 1.f, 0.f}, degToRad(yawDeg));
    Vec3 v3 = rotateAroundAxis(v2, {1.f, 0.f, 0.f}, degToRad(pitchDeg));
    return v3;
}

// Geometry: pick face whose normal is most toward the camera (+Z eye).
CubeFace dominantFacingFaceFromOrbit(float yawDeg, float pitchDeg, float rollDeg) {
    CubeFace best = CubeFace::Face1;
    float bestZ = -1e10f;
    for (int i = 0; i < kCubeFaces; ++i) {
        const auto f = static_cast<CubeFace>(i);
        const Vec3 nEye = orbitRotateDirection(outwardNormalMesh(f), yawDeg, pitchDeg, rollDeg);
        if (nEye.z > bestZ) {
            bestZ = nEye.z;
            best = f;
        }
    }
    return best;
}

} // namespace

// Geometry: default camera distance (orbit zoom clamp range in handleMouseWheel).
Renderer::Renderer() : cameraDistance_(5.0f) {}

// Lighting: GL lights + framebuffer state for shaded cube draws.
void Renderer::initialize() {
    setupFramebufferAndShadeModel();
    setupKeyLight();
}

// Geometry: dolly camera along view axis (clamped distance).
void Renderer::handleMouseWheel(int delta) {
    cameraDistance_ += static_cast<float>(delta) * 0.25f;
    cameraDistance_ = std::max(kCameraDistMin, std::min(kCameraDistMax, cameraDistance_));
}

// Geometry: viewport rectangle for projection matrix aspect.
void Renderer::resize(int width, int height) {
    if (height <= 0) {
        height = 1;
    }
    glViewport(0, 0, width, height);
}

// Lighting: depth-tested unlit star points (disables lighting for GL_POINTS).
void Renderer::drawStars() {
    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glBegin(GL_POINTS);
    std::srand(42);
    emitStarPointsOnSphere(150, 2.0f, 1.0f, 1.0f, 1.0f);
    emitStarPointsOnSphere(15, 3.0f, 1.0f, 1.0f, 0.9f);
    glEnd();

    glEnable(GL_LIGHTING);
}

// Geometry + Lighting: perspective + orbit modelview; directional light POSITIONs in scene space; star pass.
void Renderer::beginScene(float yawDeg, float pitchDeg, float rollDeg) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const double aspect =
        static_cast<double>(viewport[2]) / static_cast<double>(viewport[3] > 0 ? viewport[3] : 1);
    gluPerspective(static_cast<double>(kFovYDeg), aspect, 0.1, 250.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -cameraDistance_);
    glRotatef(pitchDeg, 1.0f, 0.0f, 0.0f);
    glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
    glRotatef(rollDeg, 0.0f, 0.0f, 1.0f);

    // World-space sun direction must be set with the same modelview as geometry, or lighting drifts to black
    // at some orbit angles (POSITION was previously fixed at init under identity only).
    const float lightDir[4] = {-0.75f, 0.85f, 0.9f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightDir);

    // Same modelview: fill is directional; without POSITION it can sit at a bad default and fight key light.
    const float fillDir[4] = {0.4f, -0.55f, -0.72f, 0.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, fillDir);

    // Mesh +/- X / +/- Y: directional fills in object space; GL transforms POSITION by the current MODELVIEW
    // (same rotation as mesh normals after uniform scale + normalize). Avoids hand-rolled orbit math drifting
    // from glRotate composition — lateral stickers stay lit when facing the camera.
    const float axisAmb[4] = {0.f, 0.f, 0.f, 1.0f};
    const float axisDiff[4] = {0.46f, 0.40f, 0.34f, 1.0f};
    const float axisSpec[4] = {0.13f, 0.12f, 0.10f, 1.0f};
    const GLenum axisLights[4] = {GL_LIGHT3, GL_LIGHT4, GL_LIGHT5, GL_LIGHT6};
    const float axisObj[4][4] = {
        {1.f, 0.f, 0.f, 0.f},
        {-1.f, 0.f, 0.f, 0.f},
        {0.f, 1.f, 0.f, 0.f},
        {0.f, -1.f, 0.f, 0.f},
    };
    for (int i = 0; i < 4; ++i) {
        glLightfv(axisLights[i], GL_AMBIENT, axisAmb);
        glLightfv(axisLights[i], GL_DIFFUSE, axisDiff);
        glLightfv(axisLights[i], GL_SPECULAR, axisSpec);
        glLightfv(axisLights[i], GL_POSITION, axisObj[i]);
    }

    // Eye-space headlight only (identity MV): frontal fill along +Ze — primary “shine” for stickers facing the view.
    glPushMatrix();
    glLoadIdentity();
    const float headAmb2[4] = {0.f, 0.f, 0.f, 1.0f};
    const float headDiff2[4] = {1.05f, 0.98f, 0.88f, 1.0f};
    const float headSpec2[4] = {0.35f, 0.30f, 0.26f, 1.0f};
    glLightfv(GL_LIGHT2, GL_AMBIENT, headAmb2);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, headDiff2);
    glLightfv(GL_LIGHT2, GL_SPECULAR, headSpec2);
    const float headPos2[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT2, GL_POSITION, headPos2);
    glPopMatrix();

    drawStars();
}

// Geometry + Lighting: lit sticker triangles (materials per palette index); optional slab twist verts/normals.
void Renderer::drawCube(const CubeMesh& mesh, const std::array<int, kStickerCount>& colors, float scale,
                         const TurnAnimCube* anim, float yawDeg, float pitchDeg, float rollDeg) {
    thread_local std::vector<char> mask;
    mask.assign(static_cast<std::size_t>(kStickerCount), 0);
    const bool useAnim = anim != nullptr && anim->active;
    Vec3 axis{0.f, 1.f, 0.f};
    float angRad = 0.f;
    if (useAnim) {
        animAxisAngle(anim->face, anim->dir, anim->currentDeg, axis, angRad);
        std::vector<int> moving;
        CubeState::stickersInSlab(anim->face, anim->depth, moving);
        for (int id : moving) {
            mask[static_cast<std::size_t>(id)] = 1;
        }
    }

    const float specMat[4] = {0.55f, 0.50f, 0.45f, 1.0f};

    const auto emitSticker = [&](int s, bool rotateSlab) {
        const int pal = colors[static_cast<std::size_t>(s)];
        float diff[4];
        diffuseBrown(pal, diff);
        const int stickersPerFace = kCubeOrder * kCubeOrder;
        const int faceIdx = s / stickersPerFace;
        // Face3/4: Z baseline. Face2 / Face5 (slots 37–72, 145–180): lateral uplift + frontal shine toward +Z eye.
        // Face1 / Face6 (1–36, 181–216): Z baseline — normal brightness vs boosted neighbors.
        const bool lateralBoost =
            faceIdx == static_cast<int>(CubeFace::Face2) || faceIdx == static_cast<int>(CubeFace::Face5);

        const CubeFace mf = static_cast<CubeFace>(faceIdx);
        const Vec3 nEye = orbitRotateDirection(outwardNormalMesh(mf), yawDeg, pitchDeg, rollDeg);
        constexpr float kGrazingNZSlack = 0.02f;
        const bool lateralFacingCamera = lateralBoost && (nEye.z >= -kGrazingNZSlack);
        const float nz = nEye.z;
        const bool lateralFrontalShine = lateralBoost && (nz >= 0.12f);

        constexpr float kAmbFloor = 0.03f;
        constexpr float kAmbFromDiffuseZ = 0.26f;
        constexpr float kAmbFromDiffuseLateral = 0.34f;
        float ambK = lateralBoost ? kAmbFromDiffuseLateral : kAmbFromDiffuseZ;
        constexpr float kEmitZ = 0.28f;
        constexpr float kEmitLateral = 0.32f;
        float emitK = lateralBoost ? kEmitLateral : kEmitZ;
        if (lateralFacingCamera && !lateralFrontalShine) {
            ambK += 0.07f;
            emitK += 0.09f;
        }

        float diffR = diff[0];
        float diffG = diff[1];
        float diffB = diff[2];
        if (lateralFrontalShine) {
            const float t = std::clamp((nz - 0.12f) / 0.78f, 0.f, 1.f);
            const float ambBlend = 0.55f + 0.38f * t; // 0.55..0.93 of diffuse color as ambient
            ambK = std::max(ambK, ambBlend);
            const float diffBoost = 1.0f + 0.55f * t;
            diffR = std::min(1.f, diffR * diffBoost);
            diffG = std::min(1.f, diffG * diffBoost);
            diffB = std::min(1.f, diffB * diffBoost);
            emitK = std::max(emitK, 0.14f + 0.18f * t);
        }

        const float ambMat[4] = {std::min(1.f, kAmbFloor + diffR * ambK),
                                 std::min(1.f, kAmbFloor + diffG * ambK),
                                 std::min(1.f, kAmbFloor + diffB * ambK), 1.0f};
        const float emitMat[4] = {std::max(0.06f, diffR * emitK), std::max(0.05f, diffG * emitK),
                                   std::max(0.045f, diffB * emitK), 1.0f};
        const float diffMat[4] = {diffR, diffG, diffB, diff[3]};

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambMat);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffMat);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specMat);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emitMat);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, lateralFrontalShine ? 38.0f : 48.0f);

        for (int t = 0; t < 2; ++t) {
            const MeshTriangle& tri = mesh.triangles[static_cast<std::size_t>(s * 2 + t)];
            Vec3 n = tri.normal;
            if (rotateSlab) {
                n = rotateAnim(n, axis, angRad);
            }
            glNormal3f(n.x, n.y, n.z);
            for (int k = 0; k < 3; ++k) {
                Vec3 p = tri.pos[static_cast<std::size_t>(k)];
                if (rotateSlab) {
                    p = rotateAnim(p, axis, angRad);
                }
                glVertex3f(p.x, p.y, p.z);
            }
        }
    };

    glPushMatrix();
    glScalef(scale, scale, scale);

    if (useAnim) {
        // Interleaved sticker order made static quads win the depth test at slab seams during a turn
        // (same numeric depth as the moving layer). Draw rest first, then the slab with a slight offset.
        glBegin(GL_TRIANGLES);
        for (int s = 0; s < kStickerCount; ++s) {
            if (mask[static_cast<std::size_t>(s)] != 0) {
                continue;
            }
            emitSticker(s, false);
        }
        glEnd();

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.2f, -2.5f);
        glBegin(GL_TRIANGLES);
        for (int s = 0; s < kStickerCount; ++s) {
            if (mask[static_cast<std::size_t>(s)] == 0) {
                continue;
            }
            emitSticker(s, true);
        }
        glEnd();
        glDisable(GL_POLYGON_OFFSET_FILL);
    } else {
        glBegin(GL_TRIANGLES);
        for (int s = 0; s < kStickerCount; ++s) {
            emitSticker(s, false);
        }
        glEnd();
    }
    glPopMatrix();
}

// Geometry: face normal most toward +Z eye (orbit consistency with beginScene).
CubeFace Renderer::dominantFacingFace(float yawDeg, float pitchDeg, float rollDeg) const {
    return dominantFacingFaceFromOrbit(yawDeg, pitchDeg, rollDeg);
}

// Geometry: face visibility in eye space (grazing slack for HUD projection).
bool Renderer::faceFacesCamera(CubeFace face, float yawDeg, float pitchDeg, float rollDeg) const {
    const Vec3 nEye = orbitRotateDirection(outwardNormalMesh(face), yawDeg, pitchDeg, rollDeg);
    // Eye +Z is toward the viewer (see dominantFacingFaceFromOrbit). Logs showed Face6 at nz5≈-0.015
    // with fc5=0 while users still expected 181–216; tiny negative slack includes that grazing band.
    constexpr float kGrazingNZSlack = 0.02f;
    return nEye.z >= -kGrazingNZSlack;
}

// Geometry: unlit sticker-outline lines (same inset coords as mesh); Lighting disabled for duration.
void Renderer::drawCubeWireframe(float scale, const TurnAnimCube* anim, CubeFace onlyFace) const {
    constexpr float H = 1.f;
    constexpr float kInset = 0.91f;

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.5f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glPushMatrix();
    glScalef(scale, scale, scale);
    glBegin(GL_LINES);

    for (int fi = 0; fi < kCubeFaces; ++fi) {
        const auto f = static_cast<CubeFace>(fi);
        if (f != onlyFace) {
            continue;
        }
        for (int r = 0; r < kCubeOrder; ++r) {
        for (int c = 0; c < kCubeOrder; ++c) {
            const int stickerSlot = fi * kCubeOrder * kCubeOrder + r * kCubeOrder + c;
            if (f == CubeFace::Face1) {
                    float x0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float x1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float z0 = -H + 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    float z1 = -H + 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    insetStickerRange(x0, x1, kInset);
                    insetStickerRange(z0, z1, kInset);
                    emitStickerQuadOutline({x0, H, z0}, {x1, H, z0}, {x1, H, z1}, {x0, H, z1}, anim, stickerSlot);
                } else if (f == CubeFace::Face2) {
                    float x0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float x1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float z0 = -H + 2.f * H * static_cast<float>(kCubeOrder - 1 - r) / static_cast<float>(kCubeOrder);
                    float z1 = -H + 2.f * H * static_cast<float>(kCubeOrder - r) / static_cast<float>(kCubeOrder);
                    insetStickerRange(x0, x1, kInset);
                    insetStickerRange(z0, z1, kInset);
                    emitStickerQuadOutline({x0, -H, z0}, {x1, -H, z0}, {x1, -H, z1}, {x0, -H, z1}, anim, stickerSlot);
                } else if (f == CubeFace::Face3) {
                    float x0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float x1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    insetStickerRange(x0, x1, kInset);
                    insetStickerRange(y0, y1, kInset);
                    emitStickerQuadOutline({x0, y0, H}, {x1, y0, H}, {x1, y1, H}, {x0, y1, H}, anim, stickerSlot);
                } else if (f == CubeFace::Face4) {
                    float x0 = H - 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float x1 = H - 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    insetStickerRange(x0, x1, kInset);
                    insetStickerRange(y0, y1, kInset);
                    emitStickerQuadOutline({x0, y0, -H}, {x0, y1, -H}, {x1, y1, -H}, {x1, y0, -H}, anim, stickerSlot);
                } else if (f == CubeFace::Face5) {
                    float z0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float z1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    insetStickerRange(z0, z1, kInset);
                    insetStickerRange(y0, y1, kInset);
                    emitStickerQuadOutline({H, y0, z0}, {H, y1, z0}, {H, y1, z1}, {H, y0, z1}, anim, stickerSlot);
                } else if (f == CubeFace::Face6) {
                    float z0 = -H + 2.f * H * static_cast<float>(c) / static_cast<float>(kCubeOrder);
                    float z1 = -H + 2.f * H * static_cast<float>(c + 1) / static_cast<float>(kCubeOrder);
                    float y0 = H - 2.f * H * static_cast<float>(r + 1) / static_cast<float>(kCubeOrder);
                    float y1 = H - 2.f * H * static_cast<float>(r) / static_cast<float>(kCubeOrder);
                    insetStickerRange(z0, z1, kInset);
                    insetStickerRange(y0, y1, kInset);
                    emitStickerQuadOutline({-H, y0, z0}, {-H, y1, z0}, {-H, y1, z1}, {-H, y0, z1}, anim, stickerSlot);
                }
            }
        }
    }

    glEnd();
    glPopMatrix();

    glEnable(GL_LIGHTING);
}

// Geometry: mesh local → window pixels (matches beginScene camera + optional slab twist); no lighting.
bool Renderer::projectMeshAnchorToWindow(float meshScale, float yawDeg, float pitchDeg, float rollDeg, Vec3 meshLocal,
                                         const TurnAnimCube* anim, int viewportW, int viewportH, double& outPx,
                                         double& outPy, bool& outVisible, int stickerSlot) const {
    if (viewportW <= 0 || viewportH <= 0) {
        outVisible = false;
        return false;
    }
    const Vec3 p = transformMeshPointForTurn(meshLocal, anim, stickerSlot);

    GLdouble proj[16]{};
    GLdouble mv[16]{};
    GLint vp[4] = {0, 0, viewportW, viewportH};
    const double aspect =
        static_cast<double>(viewportW) / static_cast<double>(viewportH > 0 ? viewportH : 1);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(static_cast<double>(kFovYDeg), aspect, 0.1, 250.0);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -cameraDistance_);
    glRotatef(pitchDeg, 1.0f, 0.0f, 0.0f);
    glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
    glRotatef(rollDeg, 0.0f, 0.0f, 1.0f);
    glScalef(meshScale, meshScale, meshScale);
    glGetDoublev(GL_MODELVIEW_MATRIX, mv);

    GLdouble winX = 0.0;
    GLdouble winY = 0.0;
    GLdouble winZ = 0.0;
    const GLboolean ok = gluProject(static_cast<GLdouble>(p.x), static_cast<GLdouble>(p.y), static_cast<GLdouble>(p.z),
                                     mv, proj, vp, &winX, &winY, &winZ);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    outVisible = (ok == GL_TRUE) && (winZ >= 0.0 && winZ <= 1.0);
    outPx = winX;
    outPy = static_cast<double>(viewportH) - winY;
    return ok == GL_TRUE;
}
