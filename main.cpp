// SFML 3 + OpenGL application.
// Geometry + Lighting: delegated to Renderer / CubeMesh; this file is UI, input, and puzzle state routing.

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <algorithm>
#include <ctime>
#include <optional>
#include <string>

#include "cube_mesh.h"
#include "cube_state.h"
#include "math3.h"
#include "renderer.h"
#include "window_shape.h"

namespace {

// UI: client pixel size for the GL viewport and SFML surface.
constexpr unsigned kWindowW = 1100;
constexpr unsigned kWindowH = 1300;
// UX: angular speed for a quarter-turn animation (degrees per second).
constexpr float kAnimDegPerSec = 320.f;
// UI: scale the cube mesh so it fits inside the diamond clip after orbit and zoom.
constexpr float kMeshViewScale = 0.70f;

// UI: bottom Y of the help block in client pixels; updated when drawing so drag-strip hit tests match.
// Diamond window clips the corners - HUD must sit below the tip where the rhombus is wide enough.
float g_hudChromeBottomY = 280.f;

// UX: left-drag either orbits the view or moves the borderless window.
enum class LeftDragKind : std::uint8_t { None = 0, Orbit, MoveWindow };

// UX: start a layer turn animation when idle; copies the move into pending for apply() at 90 degrees.
void startFaceTurn(const CubeMove& m, TurnAnimCube& anim, CubeMove& pending) {
    if (anim.active) {
        return;
    }
    pending = m;
    anim.active = true;
    anim.currentDeg = 0.f;
    anim.face = m.face;
    anim.depth = m.depth;
    anim.dir = m.dir;
}

// UX: advance the in-flight turn; commits sticker permutation when the sweep reaches 90 degrees.
void advanceTurnAnimation(float dt, TurnAnimCube& anim, CubeState& state, CubeMove& pending) {
    if (!anim.active) {
        return;
    }
    anim.currentDeg += kAnimDegPerSec * dt;
    if (anim.currentDeg >= 90.f) {
        anim.currentDeg = 90.f;
        state.apply(pending);
        anim.active = false;
        anim.currentDeg = 0.f;
    }
}

// UX: apply mouse deltas to orbit angles; clamps pitch so the camera does not flip past the poles.
void applyOrbitDrag(float& yawDeg, float& pitchDeg, int dx, int dy) {
    constexpr float kOrbitSensitivity = 0.35f;
    yawDeg += static_cast<float>(dx) * kOrbitSensitivity;
    pitchDeg += static_cast<float>(dy) * kOrbitSensitivity;
    if (pitchDeg > 89.f) {
        pitchDeg = 89.f;
    }
    if (pitchDeg < -89.f) {
        pitchDeg = -89.f;
    }
}

// UI: request depth buffer, stencil, and OpenGL 2.1 for fixed-function rendering.
sf::ContextSettings makeGlContextSettings() {
    sf::ContextSettings s;
    s.depthBits = 24;
    s.stencilBits = 8;
    s.majorVersion = 2;
    s.minorVersion = 1;
    return s;
}

// UI: load a Windows system font and build optional HUD text (Aesthetic: light gray on starfield).
// Returns false if no font file opened (HUD disabled).
bool tryLoadHudFont(sf::Font& fontOut, std::optional<sf::Text>& hudOut, std::optional<sf::Text>& devHudOut) {
    const bool ok = fontOut.openFromFile(R"(C:\Windows\Fonts\arial.ttf)") ||
                    fontOut.openFromFile(R"(C:\Windows\Fonts\calibri.ttf)");
    if (!ok) {
        return false;
    }
// UI size
    constexpr unsigned kHudFontPx = 18u;
    hudOut.emplace(fontOut, "", kHudFontPx);
    hudOut->setFillColor(sf::Color(235, 235, 240));
    hudOut->setLineSpacing(1.08f);
    constexpr unsigned kDevHudFontPx = 16u;
    devHudOut.emplace(fontOut, "", kDevHudFontPx);
    devHudOut->setFillColor(sf::Color::White);
    devHudOut->setLineSpacing(1.06f);
    return true;
}

// UI .exe
void drawHudOverlay(sf::RenderWindow& window, sf::Text& hud) {
    hud.setString(std::string("1-6: columns turn\n"
                              "q-y: columns turn\n"
                              "a-h: rows turn\n\n"
                              "Shift +: reverse\n"
                              "Space: scramble\n"
                              "Tab: solve\n\n"
                              "0: ksticker\n"));
    const sf::FloatRect lb = hud.getLocalBounds();
    const float cw = static_cast<float>(window.getSize().x);
    const float ch = static_cast<float>(window.getSize().y);
    constexpr float kHudPadX = 30.f;
    const float needHalfW = lb.size.x * 0.5f + kHudPadX;
    const float yInsideRhombus = std::clamp(needHalfW * ch / cw, 10.f, ch * 0.48f);
    // UI height
    constexpr float kHudNudgeUp = 18.f;
    const float yHud = std::max(8.f, yInsideRhombus - kHudNudgeUp);
    hud.setOrigin({lb.position.x + lb.size.x * 0.5f, lb.position.y});
    hud.setPosition({cw * 0.5f, yHud});
    g_hudChromeBottomY = yHud + lb.size.y + 10.f;
    window.pushGLStates();
    window.draw(hud);
    window.popGLStates();
}

// UI: sticker indices projected onto visible faces (Geometry: Renderer::projectMeshAnchorToWindow; fill matches Lighting bronze slot).
void drawDevNamesOnCube(sf::RenderWindow& window, Renderer& renderer, float yawDeg, float pitchDeg, float rollDeg,
                        float meshScale, const TurnAnimCube& anim, sf::Text& devHud) {
    constexpr unsigned kStickerIdxFontPx = 28u;
    constexpr unsigned kDevHudDefaultPx = 16u;

    // Lighting: HUD text color matched to diffuseBrown palette slot 0 (bronze) so labels read on starfield.
    devHud.setFillColor(sf::Color(133, 94, 64));
    const int vw = static_cast<int>(window.getSize().x);
    const int vh = static_cast<int>(window.getSize().y);
    const TurnAnimCube* animPtr = anim.active ? &anim : nullptr;

    window.pushGLStates();
    devHud.setCharacterSize(kStickerIdxFontPx);
    constexpr float kMeshHalfExtent = 1.f;
    for (int fi = 0; fi < kCubeFaces; ++fi) {
        const CubeFace face = static_cast<CubeFace>(fi);
        if (!renderer.faceFacesCamera(face, yawDeg, pitchDeg, rollDeg)) {
            continue;
        }
        for (int r = 0; r < kCubeOrder; ++r) {
            for (int c = 0; c < kCubeOrder; ++c) {
                const int slot = meshStickerSlot(face, r, c);
                const Vec3 p = stickerSlotCenterInMesh(kMeshHalfExtent, slot);
                devHud.setString(std::to_string(slot + 1));
                double px = 0.0;
                double py = 0.0;
                bool vis = false;
                static_cast<void>(renderer.projectMeshAnchorToWindow(meshScale, yawDeg, pitchDeg, rollDeg, p, animPtr,
                                                                     vw, vh, px, py, vis, slot));
                if (!vis) {
                    continue;
                }
                const sf::FloatRect lb = devHud.getLocalBounds();
                devHud.setOrigin({lb.position.x + lb.size.x * 0.5f, lb.position.y + lb.size.y * 0.5f});
                devHud.setPosition({static_cast<float>(px), static_cast<float>(py)});
                window.draw(devHud);
            }
        }
    }
    devHud.setCharacterSize(kDevHudDefaultPx);
    window.popGLStates();
}

// UI: hit-test a generous top band so borderless window moves without Alt (diamond tip is narrow).
// Height scales with client size; capped so most of the view stays orbit.
bool isPointerInHudDragStrip(sf::Vector2i p, const sf::RenderWindow& window) {
    const int h = static_cast<int>(window.getSize().y);
    const int stripH = std::min(160, std::max(56, h / 4));
    return p.y >= 0 && p.y < stripH && p.x >= 0 && p.x < static_cast<int>(window.getSize().x);
}

// UX: Space scramble, Backspace undo, Tab solved, Enter reset (cancels in-flight animation on Enter).
void handleGlobalPuzzleCommands(sf::Keyboard::Key key, CubeState& state, TurnAnimCube& anim, sf::Clock& scrambleClock) {
    using K = sf::Keyboard::Key;
    if (key == K::Space) {
        if (!anim.active) {
            state.scramble(48, static_cast<unsigned>(scrambleClock.getElapsedTime().asMilliseconds()));
        }
    } else if (key == K::Backspace) {
        if (!anim.active) {
            state.undo();
        }
    } else if (key == K::Tab) {
        if (!anim.active) {
            state.autoComplete();
        }
    } else if (key == K::Enter) {
        anim.active = false;
        anim.currentDeg = 0.f;
        state.reset();
    }
}

// UX: Ctrl + (numpad or main-row) 1-6 sets active layer depth. Numpad or main row 1-6 alone turns that face.
bool trySetDepth(sf::Keyboard::Key key, bool ctrlHeld, int& depthOut) {
    if (!ctrlHeld) {
        return false;
    }
    using K = sf::Keyboard::Key;
    if (key == K::Numpad1 || key == K::Num1) {
        depthOut = 1;
    } else if (key == K::Numpad2 || key == K::Num2) {
        depthOut = 2;
    } else if (key == K::Numpad3 || key == K::Num3) {
        depthOut = 3;
    } else if (key == K::Numpad4 || key == K::Num4) {
        depthOut = 4;
    } else if (key == K::Numpad5 || key == K::Num5) {
        depthOut = 5;
    } else if (key == K::Numpad6 || key == K::Num6) {
        depthOut = 6;
    } else {
        return false;
    }
    return true;
}

// UX: Ctrl+digit sets depth; numpad or main-row 1-6 alone turns Face1..Face6 (Shift inverts direction).
// Routes other keys to global puzzle commands.
void handlePuzzleKeys(sf::Keyboard::Key key, int& depth, int dir, CubeState& state, TurnAnimCube& anim,
                      sf::Clock& scrambleClock, CubeMove& pending) {
    using K = sf::Keyboard::Key;
    const bool ctrlHeld =
        sf::Keyboard::isKeyPressed(K::LControl) || sf::Keyboard::isKeyPressed(K::RControl);
    if (trySetDepth(key, ctrlHeld, depth)) {
        return;
    }
    CubeMove m{};
    m.depth = static_cast<std::uint8_t>(depth);
    m.dir = static_cast<std::int8_t>(dir);
    if (key == K::Num1 || key == K::Numpad1) {
        m.face = CubeFace::Face1;
    } else if (key == K::Num2 || key == K::Numpad2) {
        m.face = CubeFace::Face2;
    } else if (key == K::Num3 || key == K::Numpad3) {
        m.face = CubeFace::Face3;
    } else if (key == K::Num4 || key == K::Numpad4) {
        m.face = CubeFace::Face4;
    } else if (key == K::Num5 || key == K::Numpad5) {
        m.face = CubeFace::Face5;
    } else if (key == K::Num6 || key == K::Numpad6) {
        m.face = CubeFace::Face6;
    } else if (key == K::Q) {
        m.face = CubeFace::Face7;
    } else if (key == K::W) {
        m.face = CubeFace::Face8;
    } else if (key == K::E) {
        m.face = CubeFace::Face9;
    } else if (key == K::R) {
        m.face = CubeFace::Face10;
    } else if (key == K::T) {
        m.face = CubeFace::Face11;
    } else if (key == K::Y) {
        m.face = CubeFace::Face12;
    } else if (key == K::A) {
        m.face = CubeFace::Face13;
    } else if (key == K::S) {
        m.face = CubeFace::Face14;
    } else if (key == K::D) {
        m.face = CubeFace::Face15;
    } else if (key == K::F) {
        m.face = CubeFace::Face16;
    } else if (key == K::G) {
        m.face = CubeFace::Face17;
    } else if (key == K::H) {
        m.face = CubeFace::Face18;
    } else {
        handleGlobalPuzzleCommands(key, state, anim, scrambleClock);
        return;
    }
    startFaceTurn(m, anim, pending);
}

// UI: keep GL viewport in sync when the OS window changes size.
void handleResizeEvent(const sf::Event::Resized& rs, sf::RenderWindow& window, Renderer& renderer) {
    renderer.resize(static_cast<int>(rs.size.x), static_cast<int>(rs.size.y));
    applyDiamondWindowShape(window);
}

// UX: begin left-drag as window move (top strip or Alt+click anywhere) or camera orbit.
void handleMousePressed(const sf::Event::MouseButtonPressed& mb, sf::RenderWindow& window, LeftDragKind& leftDrag,
                        sf::Vector2i& dragPos, sf::Vector2i& windowDragGrab) {
    if (mb.button != sf::Mouse::Button::Left) {
        return;
    }
    const bool altHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) ||
                         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);
    if (altHeld || isPointerInHudDragStrip(mb.position, window)) {
        leftDrag = LeftDragKind::MoveWindow;
        windowDragGrab = sf::Mouse::getPosition() - window.getPosition();
    } else {
        leftDrag = LeftDragKind::Orbit;
    }
    dragPos = mb.position;
}

// UX: end left-drag so the next press picks a fresh mode.
void handleMouseReleased(const sf::Event::MouseButtonReleased& mr, LeftDragKind& leftDrag) {
    if (mr.button == sf::Mouse::Button::Left) {
        leftDrag = LeftDragKind::None;
    }
}

// UX: move the borderless frame or accumulate orbit deltas from client drag.
void handleMouseMoved(const sf::Event::MouseMoved& mm, sf::RenderWindow& window, LeftDragKind leftDrag,
                      sf::Vector2i& dragPos, sf::Vector2i windowDragGrab, float& yawDeg, float& pitchDeg) {
    if (leftDrag == LeftDragKind::MoveWindow) {
        window.setPosition(sf::Mouse::getPosition() - windowDragGrab);
    } else if (leftDrag == LeftDragKind::Orbit) {
        const int dx = mm.position.x - dragPos.x;
        const int dy = mm.position.y - dragPos.y;
        dragPos = mm.position;
        applyOrbitDrag(yawDeg, pitchDeg, dx, dy);
    }
}

// UX: wheel zoom is delegated to the renderer camera distance clamp.
void handleWheelScrolled(const sf::Event::MouseWheelScrolled& wheel, Renderer& renderer) {
    renderer.handleMouseWheel(static_cast<int>(wheel.delta));
}

// UX: Escape closes; 0 toggles dev symbol overlay; other keys puzzle router.
void handleKeyPressed(const sf::Event::KeyPressed& kp, sf::RenderWindow& window, CubeState& state, TurnAnimCube& anim,
                      int& depth, sf::Clock& scrambleClock, CubeMove& pending, bool& showDevNames) {
    const auto key = kp.code;
    if (key == sf::Keyboard::Key::Escape) {
        window.close();
        return;
    }
    using Sc = sf::Keyboard::Scan;
    const bool zeroByCode = (key == sf::Keyboard::Key::Num0 || key == sf::Keyboard::Key::Numpad0);
    const bool zeroByScan =
        (kp.scancode == Sc::Num0 || kp.scancode == Sc::Numpad0);
    if (zeroByCode || zeroByScan) {
        showDevNames = !showDevNames;
        return;
    }
    const bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    const int vdir = shift ? -1 : 1;
    handlePuzzleKeys(key, depth, vdir, state, anim, scrambleClock, pending);
}

// UX: drain one frame of SFML events (window shape, orbit, zoom, keys).
void processFrameEvents(sf::RenderWindow& window, Renderer& renderer, CubeState& state, TurnAnimCube& anim, int& depth,
                        float& yawDeg, float& pitchDeg, LeftDragKind& leftDrag, sf::Vector2i& dragPos,
                        sf::Vector2i& windowDragGrab, sf::Clock& scrambleClock, CubeMove& pending,
                        bool& showDevNames) {
    while (const std::optional<sf::Event> ev = window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* rs = ev->getIf<sf::Event::Resized>()) {
            handleResizeEvent(*rs, window, renderer);
        }
        if (const auto* mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
            handleMousePressed(*mb, window, leftDrag, dragPos, windowDragGrab);
        }
        if (const auto* mr = ev->getIf<sf::Event::MouseButtonReleased>()) {
            handleMouseReleased(*mr, leftDrag);
        }
        if (const auto* mm = ev->getIf<sf::Event::MouseMoved>()) {
            handleMouseMoved(*mm, window, leftDrag, dragPos, windowDragGrab, yawDeg, pitchDeg);
        }
        if (const auto* wheel = ev->getIf<sf::Event::MouseWheelScrolled>()) {
            handleWheelScrolled(*wheel, renderer);
        }
        if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
            handleKeyPressed(*kp, window, state, anim, depth, scrambleClock, pending, showDevNames);
        }
    }
}

// Geometry + Lighting: GL scene + cube + overlays (SFML HUD after GL).
void renderFrame(sf::RenderWindow& window, Renderer& renderer, float yawDeg, float pitchDeg, float rollDeg,
                 const CubeMesh& mesh, CubeState& state, TurnAnimCube& anim, std::optional<sf::Text>& hud,
                 std::optional<sf::Text>& devHud, bool showDevNames) {
    renderer.resize(static_cast<int>(window.getSize().x), static_cast<int>(window.getSize().y));
    static_cast<void>(window.setActive(true));
    renderer.beginScene(yawDeg, pitchDeg, rollDeg);
    renderer.drawCube(mesh, state.colors(), kMeshViewScale, anim.active ? &anim : nullptr, yawDeg, pitchDeg,
                      rollDeg);
    if (showDevNames) {
        for (int fi = 0; fi < kCubeFaces; ++fi) {
            const auto face = static_cast<CubeFace>(fi);
            if (renderer.faceFacesCamera(face, yawDeg, pitchDeg, rollDeg)) {
                renderer.drawCubeWireframe(kMeshViewScale, anim.active ? &anim : nullptr, face);
            }
        }
    }
    if (hud.has_value()) {
        drawHudOverlay(window, *hud);
    }
    if (showDevNames && devHud.has_value()) {
        drawDevNamesOnCube(window, renderer, yawDeg, pitchDeg, rollDeg, kMeshViewScale, anim, *devHud);
    }
    window.display();
}

// UI: create borderless window, diamond clip, and GL context; wire initial scramble and main loop.
void runApplication() {
    const sf::ContextSettings settings = makeGlContextSettings();

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(kWindowW, kWindowH)), "Hexahedron", sf::Style::None,
                            sf::State::Windowed, settings);
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);
    static_cast<void>(window.setActive(true));
    applyDiamondWindowShape(window);

    Renderer renderer;
    renderer.initialize();

    const CubeMesh mesh = buildCubeMesh(1.0f);
    CubeState state;
    state.scramble(36, static_cast<unsigned>(std::time(nullptr)));

    TurnAnimCube anim{};
    CubeMove pending{};

    int depth = 1;
    float yawDeg = 35.f;
    float pitchDeg = 22.f;
    const float rollDeg = 45.f;
    LeftDragKind leftDrag = LeftDragKind::None;
    sf::Vector2i dragPos{};
    sf::Vector2i windowDragGrab{};

    sf::Clock frameClock;
    sf::Clock scrambleClock;
    sf::Font font;
    std::optional<sf::Text> hud;
    std::optional<sf::Text> devHud;
    static_cast<void>(tryLoadHudFont(font, hud, devHud));
    bool showDevNames = false;

    while (window.isOpen()) {
        const float dt = frameClock.restart().asSeconds();

        processFrameEvents(window, renderer, state, anim, depth, yawDeg, pitchDeg, leftDrag, dragPos, windowDragGrab,
                           scrambleClock, pending, showDevNames);

        advanceTurnAnimation(dt, anim, state, pending);

        renderFrame(window, renderer, yawDeg, pitchDeg, rollDeg, mesh, state, anim, hud, devHud, showDevNames);
    }
}

} // namespace

int main() {
    runApplication();
    return 0;
}
