#include "core/Game.hpp"
#include "obstacles/Cannon.hpp"
#include "obstacles/Crossbow.hpp"
#include "obstacles/Mine.hpp"
#include "obstacles/Projectile.hpp"
#include "obstacles/RotatingBlade.hpp"
#include "obstacles/Turret.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
float clampDt(float dt) { return std::clamp(dt, 0.f, 0.05f); }

constexpr float kMenuBtnW   = 360.f;
constexpr float kMenuBtnH   = 56.f;
constexpr float kMenuBtnGap = 18.f;

// ─── Вспомогательные функции ──────────────────────────────────────────────────

bool isStandingOn(const sf::FloatRect& player, const sf::FloatRect& platform) {
    constexpr float kTopEps   = 6.f;
    constexpr float kSideInset = 4.f;
    const float playerBottom  = player.top + player.height;
    const bool nearTop = playerBottom >= platform.top - kTopEps &&
                         playerBottom <= platform.top + kTopEps;

    const float playerLeft   = player.left + kSideInset;
    const float playerRight  = player.left + player.width - kSideInset;
    const float platformLeft = platform.left;
    const float platformRight = platform.left + platform.width;
    const bool overlapX = playerRight > platformLeft && playerLeft < platformRight;
    return nearTop && overlapX;
}

// ─── Детектирование "стоит ли игрок на лопасти" ──────────────────────────────
//
// Лопасть НЕ добавляется в dynamic solids (AABB вращающегося спрайта меняется
// на каждом кадре и физически толкает игрока). Вместо этого мы проверяем:
//
//   1. Горизонтальное перекрытие между игроком и AABB лопасти.
//   2. Центр Y игрока выше центра Y лопасти — т.е. игрок подходит СВЕРХУ,
//      а не снизу и не сбоку. Центр лопасти (getPosition()) инвариантен
//      при вращении → проверка стабильна в любой фазе вращения.
//   3. Низ игрока находится в диапазоне [AABB.top - kEpsAbove, AABB.top + 60%H].
//      Нижний предел с запасом компенсирует колебание AABB.top при вращении.
//
RotatingBlade* findBladeSupport(const std::vector<std::unique_ptr<Entity>>& entities,
                                const sf::FloatRect& playerHitbox) {
    // Допуск сверху: игрок может быть чуть выше AABB-верха лопасти.
    // Это стабилизирует детектор при колебании AABB в ходе вращения.
    constexpr float kEpsAbove = 6.f;

    RotatingBlade* support  = nullptr;
    float          bestCY   = 1e9f;   // выбираем самую "верхнюю" лопасть

    const float pLeft    = playerHitbox.left + 4.f;
    const float pRight   = playerHitbox.left + playerHitbox.width - 4.f;
    const float pBottom  = playerHitbox.top  + playerHitbox.height;
    const float pCenterY = playerHitbox.top  + playerHitbox.height * 0.5f;

    for (const auto& e : entities) {
        auto* blade = dynamic_cast<RotatingBlade*>(e.get());
        if (!blade || blade->isExpired()) continue;

        const sf::FloatRect b  = blade->getBounds();
        const sf::Vector2f  bc = blade->center();   // rot-инвариантный центр

        // 1. Горизонтальное перекрытие
        if (pRight <= b.left || pLeft >= b.left + b.width) continue;

        // 2. Игрок подходит сверху: его центр Y < центра лопасти
        if (pCenterY >= bc.y) continue;

        // 3. Низ игрока близко к поверхности лопасти
        const float penetration = pBottom - b.top;
        if (penetration < -kEpsAbove)        continue;  // слишком высоко
        if (penetration > b.height * 0.6f)   continue;  // слишком глубоко (боковой контакт)

        if (bc.y < bestCY) {
            bestCY  = bc.y;
            support = blade;
        }
    }
    return support;
}

} // namespace

// ─── Конструктор / деструктор ────────────────────────────────────────────────

Game::Game()
    : m_window(sf::VideoMode(W_WIDTH, W_HEIGHT), W_TITLE) {
    m_playButtonRect = {
        W_WIDTH * 0.5f - kMenuBtnW * 0.5f,
        W_HEIGHT * 0.55f,
        kMenuBtnW,
        kMenuBtnH,
    };
    m_backButtonRect = {
        W_WIDTH * 0.5f - kMenuBtnW * 0.5f,
        W_HEIGHT - 120.f,
        kMenuBtnW,
        kMenuBtnH,
    };

    try {
        loadResources();
    } catch (const std::exception& e) {
        std::cerr << "Resource Error: " << e.what() << std::endl;
    }

    discoverLevelPaths();
    m_screen = GameScreen::MainMenu;

    m_window.setFramerateLimit(60);
    std::cout << "Game Initialized!" << std::endl;
}

Game::~Game() = default;

void Game::discoverLevelPaths() {
    m_levelPaths.clear();
    namespace fs = std::filesystem;
    const fs::path levelsDir("assets/levels");
    if (!fs::exists(levelsDir) || !fs::is_directory(levelsDir)) {
        std::cerr << "[Levels] Directory not found: assets/levels\n";
        return;
    }
    for (const auto& entry : fs::directory_iterator(levelsDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;
        m_levelPaths.push_back(entry.path().generic_string());
    }
    std::sort(m_levelPaths.begin(), m_levelPaths.end());
    std::cout << "[Levels] Found " << m_levelPaths.size() << " level(s)\n";
}

void Game::loadResources() {
    std::cout << "--- Loading Resources ---" << std::endl;

    sf::Image img;
    img.create(1, 1, sf::Color::White);
    if (!m_whiteTex.loadFromImage(img)) {
        throw std::runtime_error("Failed to create white placeholder texture");
    }

    loadTextures();
    loadFonts();
    loadSounds();

    m_hasMenuBackground = false;
    sf::Texture menuBg;
    if (menuBg.loadFromFile("assets/textures/background.png")) {
        m_textures["menu_bg"] = std::move(menuBg);
        m_menuBackgroundSprite.setTexture(m_textures["menu_bg"]);
        const auto bounds = m_menuBackgroundSprite.getLocalBounds();
        if (bounds.width > 0.f && bounds.height > 0.f) {
            m_menuBackgroundSprite.setScale(
                W_WIDTH / bounds.width,
                W_HEIGHT / bounds.height);
        }
        m_hasMenuBackground = true;
        std::cout << "[Texture] Menu background loaded\n";
    } else {
        std::cerr << "[Texture] Menu background not found (black screen)\n";
    }

    std::cout << "--- All Resources Loaded ---" << std::endl;
}

void Game::loadTextures() {
    const std::map<std::string, std::string> manifest = {
        // ── PLAYER (Option B: один PNG-strip на состояние) ─────────────────
        {"player_idle",    "assets/textures/player/idle.png"},
        {"player_walk",    "assets/textures/player/walk.png"},
        {"player_run",     "assets/textures/player/run.png"},
        {"player_crouch",  "assets/textures/player/crouch.png"},

        // ── OBSTACLES ──────────────────────────────────────────────────────
        {"obs_crossbow",   "assets/textures/obstacles/crossbow.png"},
        {"obs_arrow",      "assets/textures/obstacles/arrow.png"},
        {"obs_cannon",     "assets/textures/obstacles/cannon.png"},
        {"obs_cannonball", "assets/textures/obstacles/cannonball.png"},
        {"obs_turret",     "assets/textures/obstacles/turret.png"},
        {"obs_bullet",     "assets/textures/obstacles/bullet.png"},
        {"obs_mine",       "assets/textures/obstacles/mine.png"},
        {"obs_blades",     "assets/textures/obstacles/blades.png"},
        {"stub_solid",     "assets/textures/platforms/solid.png"},
        {"stub_ice",       "assets/textures/platforms/ice.png"},
        {"stub_spring",    "assets/textures/platforms/spring.png"},
        {"stub_hazard",    "assets/textures/platforms/hazard.png"},
        {"stub_finish",    "assets/textures/platforms/finish.png"},
        {"conv_end_l",     "assets/textures/platforms/end_l.png"},
        {"conv_middle",    "assets/textures/platforms/middle.png"},
        {"conv_end_r",     "assets/textures/platforms/end_r.png"},
        {"vanish_end_l",   "assets/textures/platforms/left_vanish1.png"},
        {"vanish_mid",     "assets/textures/platforms/middle_vanish1.png"},
        {"vanish_end_r",   "assets/textures/platforms/right_vanish1.png"},
    };

    for (const auto& [id, path] : manifest) {
        if (!m_textures[id].loadFromFile(path)) {
            std::cerr << "[Texture] Not found (using fallback): " << path << std::endl;
            m_textures.erase(id);
            continue;
        }
        std::cout << "[Texture] Loaded: " << id << std::endl;
    }

    // ── Подключаем анимации игрока, если есть хотя бы одна из текстур ──
    {
        std::map<std::string, const sf::Texture*> playerSheets;
        auto tryAdd = [&](const std::string& state, const std::string& texKey) {
            auto it = m_textures.find(texKey);
            if (it != m_textures.end()) playerSheets[state] = &it->second;
        };
        tryAdd("idle",   "player_idle");
        tryAdd("walk",   "player_walk");
        tryAdd("run",    "player_run");
        tryAdd("crouch", "player_crouch");

        // Размер одного кадра берём из текстуры idle (если она есть):
        // ширина = texW / 6 (idle — 6 кадров), высота = texH.
        // Если idle нет, остаёмся в fallback-режиме (цветной прямоугольник).
        if (!playerSheets.empty()) {
            const sf::Texture* ref = playerSheets.count("idle") ? playerSheets["idle"]
                                                                : playerSheets.begin()->second;
            const sf::Vector2u sz = ref->getSize();
            const sf::Vector2i frame{
                static_cast<int>(sz.x / 6u),
                static_cast<int>(sz.y)};
            m_player.setupAnimations(playerSheets, frame);
            std::cout << "[Player] Animations bound (" << playerSheets.size()
                      << " state(s); frame=" << frame.x << "x" << frame.y << ")\n";
        } else {
            std::cout << "[Player] No animation textures found — running in fallback mode.\n";
        }
    }
}

void Game::loadFonts() {
    const std::vector<std::string> candidates = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
    };
    for (const auto& path : candidates) {
        sf::Font font;
        if (font.loadFromFile(path)) {
            m_fonts["main"] = std::move(font);
            m_hudFontLoaded = true;
            std::cout << "[Font] Loaded: " << path << std::endl;
            break;
        }
    }
    if (!m_hudFontLoaded)
        std::cerr << "[Font] No system font found; HUD text disabled." << std::endl;
}

void Game::loadSounds() {
    auto tryLoad = [this](const std::string& id, const std::string& path) {
        sf::SoundBuffer buf;
        if (buf.loadFromFile(path)) {
            m_soundBuffers[id] = std::move(buf);
            std::cout << "[Sound] Loaded: " << id << std::endl;
        }
    };
    tryLoad("jump",  "assets/sounds/jump.wav");
    tryLoad("death", "assets/sounds/death.wav");
    if (!m_soundBuffers.empty()) m_sfxReady = true;
}

bool Game::loadLevelIndex(size_t index) {
    if (m_levelPaths.empty()) return false;
    const size_t clamped = std::min(index, m_levelPaths.size() - 1);

    std::string err;
    if (!m_level.loadFromFile(m_levelPaths[clamped], err)) {
        std::cerr << err << std::endl;
        const std::string emergency = R"({
  "version":1,"tileSize":48,"grid":{"columns":7,"rows":4},
  "spawn":{"x":72,"y":72},
  "platforms":[
    {"type":"solid","x":0,"y":0,"w":336,"h":48},
    {"type":"solid","x":0,"y":144,"w":336,"h":48},
    {"type":"solid","x":0,"y":0,"w":48,"h":192},
    {"type":"solid","x":288,"y":0,"w":48,"h":192},
    {"type":"finish","x":240,"y":48,"w":48,"h":48}
  ],"traps":[],"dynamic_platforms":[]})";
        std::string err2;
        if (!m_level.loadFromJsonString(emergency, "<fallback>", err2)) {
            std::cerr << err2 << std::endl;
            return false;
        }
    }

    m_levelIndex = clamped;
    m_player.onLevelLoaded(m_level);
    m_entities.clear();
    spawnPlacedEntitiesFromLevel();
    spawnDynamicPlatformsFromLevel();
    setupLevelBackground();
    m_bladeSupportActive = false;
    m_bladeCarryVelocity = {0.f, 0.f};
    m_gameWon = false;
    m_overviewHeld = false;
    m_camera.snapTo(m_player.getCenter(), m_window.getSize(), m_level.pixelSize());
    return true;
}

void Game::setupLevelBackground() {
    m_hasLevelBackground = false;
    const std::string& path = m_level.backgroundPath();
    if (path.empty()) return;

    auto it = m_textures.find(path);
    if (it == m_textures.end()) {
        sf::Texture tex;
        if (!tex.loadFromFile(path)) {
            std::cerr << "[Level BG] Not found: " << path << std::endl;
            return;
        }
        it = m_textures.emplace(path, std::move(tex)).first;
        std::cout << "[Level BG] Loaded: " << path << std::endl;
    }

    m_levelBackgroundSprite.setTexture(it->second, true);
    const auto bounds = m_levelBackgroundSprite.getLocalBounds();
    m_levelBackgroundSprite.setOrigin(0.f, 0.f);
    if (bounds.width > 0.f && bounds.height > 0.f) {
        m_levelBackgroundSprite.setScale(
            static_cast<float>(W_WIDTH) / bounds.width,
            static_cast<float>(W_HEIGHT) / bounds.height);
    }
    m_levelBackgroundSprite.setPosition(0.f, 0.f);
    m_hasLevelBackground = true;
}

void Game::spawnPlacedEntitiesFromLevel() {
    for (const auto& pe : m_level.placedEntities()) {
        spawnEntity(pe);
    }
}

void Game::spawnDynamicPlatformsFromLevel() {
    for (const auto& def : m_level.dynamicPlatformDefs()) {
        if (def.kind == DynamicPlatformDef::Kind::Moving) {
            auto mp = std::make_unique<MovingPlatform>(
                m_whiteTex,
                def.bounds.left, def.bounds.top,
                def.bounds.width, def.bounds.height);
            mp->setColor(sf::Color(100, 180, 255));
            mp->setMovement(def.moveOffset, def.moveSpeed);
            m_entities.push_back(std::move(mp));

        } else if (def.kind == DynamicPlatformDef::Kind::Vanishing) {
            auto vp = std::make_unique<VanishingPlatform>(
                getTexture("vanish_end_l"),
                getTexture("vanish_mid"),
                getTexture("vanish_end_r"),
                def.bounds.left, def.bounds.top,
                def.widthInTiles,
                m_level.tileSize());
            vp->setDeathTime(def.deathTime);
            m_entities.push_back(std::move(vp));

        } else if (def.kind == DynamicPlatformDef::Kind::Conveyor) {
            auto cp = std::make_unique<ConveyorPlatform>(
                getTexture("conv_end_l"),
                getTexture("conv_middle"),
                getTexture("conv_end_r"),
                def.bounds.left, def.bounds.top,
                def.widthInTiles,
                m_level.tileSize());
            cp->setVelocity(def.conveyorVelocity);
            m_entities.push_back(std::move(cp));
        }
    }
}

void Game::respawnCurrentLevel() {
    m_entities.clear();
    spawnPlacedEntitiesFromLevel();
    spawnDynamicPlatformsFromLevel();
    m_player.respawn(m_level);
    m_bladeSupportActive = false;
    m_bladeCarryVelocity = {0.f, 0.f};
}

void Game::spawnEntity(const PlacedEntity& pe) {
    const sf::FloatRect bounds{{0.f, 0.f}, m_level.pixelSize()};

    if (pe.type == "crossbow" || pe.type == "crossbow_fast") {
        m_entities.push_back(std::make_unique<Crossbow>(
            tryGetTexture("obs_crossbow"),
            tryGetTexture("obs_arrow"),
            pe.position,
            pe.fireInterval,
            pe.projectileVelocity,
            bounds));

    } else if (pe.type == "cannon") {
        m_entities.push_back(std::make_unique<Cannon>(
            tryGetTexture("obs_cannon"),
            tryGetTexture("obs_cannonball"),
            pe.position,
            pe.fireInterval,
            pe.angle,
            pe.projectileSpeed,
            pe.projectileGravity,
            bounds));

    } else if (pe.type == "turret") {
        m_entities.push_back(std::make_unique<Turret>(
            tryGetTexture("obs_turret"),
            tryGetTexture("obs_bullet"),
            pe.position,
            pe.fireInterval,
            pe.projectileSpeed,
            pe.projectileGravity,
            bounds,
            [this]() { return m_player.getCenter(); }));

    } else if (pe.type == "mine") {
        m_entities.push_back(std::make_unique<Mine>(
            tryGetTexture("obs_mine"),
            pe.position,
            pe.triggerRadius,
            pe.armDelay,
            pe.blastRadius,
            [this]() { return m_player.getCenter(); },
            [this]() { m_player.kill(); }));

    } else if (pe.type == "blades" || pe.type == "rotating_blades") {
        m_entities.push_back(std::make_unique<RotatingBlade>(
            tryGetTexture("obs_blades"),
            pe.position,
            pe.rotationSpeed,
            pe.size));
    } else {
        std::cerr << "[Factory] Unknown trap type: " << pe.type << std::endl;
    }
}

const sf::Texture& Game::getTexture(const std::string& name) const {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) return it->second;
    throw std::runtime_error("Texture not found: " + name);
}

const sf::Texture& Game::tryGetTexture(const std::string& name) const {
    auto it = m_textures.find(name);
    return (it != m_textures.end()) ? it->second : m_whiteTex;
}

void Game::tryPlaySfx(const std::string& name) {
    if (!m_sfxReady) return;
    auto it = m_soundBuffers.find(name);
    if (it == m_soundBuffers.end()) return;
    m_sfx.setBuffer(it->second);
    m_sfx.play();
}

void Game::advanceLevelOrWin() {
    m_player.clearFinish();
    if (m_levelIndex + 1 >= m_levelPaths.size()) {
        m_gameWon = true;
        return;
    }
    loadLevelIndex(m_levelIndex + 1);
}

void Game::restartFromFirstLevel() {
    m_gameWon = false;
    if (!m_levelPaths.empty())
        loadLevelIndex(0);
}

void Game::buildLevelSelectButtons() {
    m_levelButtonRects.clear();
    float y = 220.f;
    const float cx = W_WIDTH * 0.5f;
    for (size_t i = 0; i < m_levelPaths.size(); ++i) {
        m_levelButtonRects.push_back({cx - kMenuBtnW * 0.5f, y, kMenuBtnW, kMenuBtnH});
        y += kMenuBtnH + kMenuBtnGap;
    }
}

void Game::drawButton(const sf::FloatRect& rect, const std::string& label) {
    sf::RectangleShape box({rect.width, rect.height});
    box.setPosition(rect.left, rect.top);
    box.setFillColor(sf::Color(30, 30, 30, 210));
    box.setOutlineColor(sf::Color::White);
    box.setOutlineThickness(2.f);
    m_window.draw(box);

    if (!m_hudFontLoaded) return;

    sf::Text text(label, m_fonts.at("main"), 30);
    const auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f,
                   bounds.top + bounds.height * 0.5f);
    text.setPosition(rect.left + rect.width * 0.5f,
                     rect.top + rect.height * 0.5f);
    text.setFillColor(sf::Color::White);
    m_window.draw(text);
}

void Game::drawMainMenu() {
    m_window.setView(m_window.getDefaultView());
    if (m_hasMenuBackground) {
        m_window.clear();
        m_window.draw(m_menuBackgroundSprite);
    } else {
        m_window.clear(sf::Color::Black);
    }

    drawButton(m_playButtonRect, "Play");
}

void Game::drawLevelSelect() {
    m_window.setView(m_window.getDefaultView());
    if (m_hasMenuBackground) {
        m_window.clear();
        m_window.draw(m_menuBackgroundSprite);
        sf::RectangleShape dim({static_cast<float>(W_WIDTH), static_cast<float>(W_HEIGHT)});
        dim.setFillColor(sf::Color(0, 0, 0, 140));
        m_window.draw(dim);
    } else {
        m_window.clear(sf::Color::Black);
    }

    if (m_hudFontLoaded) {
        sf::Text heading("Select Level", m_fonts.at("main"), 52);
        const auto hb = heading.getLocalBounds();
        heading.setOrigin(hb.left + hb.width * 0.5f, hb.top + hb.height * 0.5f);
        heading.setPosition(W_WIDTH * 0.5f, 120.f);
        heading.setFillColor(sf::Color::White);
        heading.setOutlineColor(sf::Color::Black);
        heading.setOutlineThickness(2.f);
        m_window.draw(heading);
    }

    if (m_levelPaths.empty()) {
        drawButton(
            {W_WIDTH * 0.5f - kMenuBtnW * 0.5f, W_HEIGHT * 0.45f, kMenuBtnW, kMenuBtnH},
            "No levels found");
    } else {
        for (size_t i = 0; i < m_levelButtonRects.size(); ++i) {
            const std::filesystem::path p(m_levelPaths[i]);
            drawButton(m_levelButtonRects[i], p.stem().string());
        }
    }

    drawButton(m_backButtonRect, "Back");
}

void Game::handleMenuClick(sf::Vector2f pos) {
    if (m_screen == GameScreen::MainMenu) {
        if (m_playButtonRect.contains(pos)) {
            buildLevelSelectButtons();
            m_screen = GameScreen::LevelSelect;
        }
        return;
    }

    if (m_screen == GameScreen::LevelSelect) {
        if (m_backButtonRect.contains(pos)) {
            m_screen = GameScreen::MainMenu;
            return;
        }
        for (size_t i = 0; i < m_levelButtonRects.size(); ++i) {
            if (!m_levelButtonRects[i].contains(pos)) continue;
            if (loadLevelIndex(i))
                m_screen = GameScreen::Playing;
            return;
        }
    }
}

// ─── updatePlaying ────────────────────────────────────────────────────────────
//
//  Порядок обновления за один тик:
//
//  1. Обновляем все Entity (лопасти, пушки, снаряды …).
//  2. Регистрируем MovingPlatform и VanishingPlatform как dynamic solids.
//     RotatingBlade сюда НЕ добавляем: её AABB колышется при вращении и
//     будет физически выталкивать игрока.
//  3. Если на прошлом тике игрок стоял на лопасти — переносим его
//     заранее (до физики) на вектор касательной скорости * dt.
//  4. Запускаем player.update() (физика, коллизии с solid-тайлами и
//     платформами из п.2).
//  5. Вручную разрешаем коллизию «игрок ↔ лопасть»:
//     – находим лопасть, на которой стоит игрок (по центру, стабильно);
//     – снапим игрока к поверхности лопасти (вверх на глубину проникновения);
//     – обнуляем нисходящую скорость и принудительно ставим флаг onGround,
//       чтобы следующий тик использовал наземные параметры физики.
//  6. При НАМЕРЕННОМ прыжке (jumpPressed) передаём игроку инерцию лопасти.
//     При простом соскальзывании с края — импульс НЕ даём.
//
void Game::updatePlaying(float dt) {
    if (m_gameWon) return;

    // ── 1. Обновление Entity ─────────────────────────────────────────────────
    std::vector<std::unique_ptr<Entity>> spawned;
    for (auto& e : m_entities) {
        e->update(dt, spawned);
    }
    {
        std::vector<std::unique_ptr<Entity>> none;
        for (auto& e : spawned) {
            e->update(dt, none);
            m_entities.push_back(std::move(e));
        }
    }

    // ── 2. Регистрируем dynamic solids (только платформы, НЕ лопасти) ───────
    m_level.clearDynamicSolids();
    for (auto& e : m_entities) {
        if (e->isExpired()) continue;
        if (dynamic_cast<MovingPlatform*>(e.get()) ||
            dynamic_cast<VanishingPlatform*>(e.get()) ||
            dynamic_cast<ConveyorPlatform*>(e.get())) {
            m_level.addDynamicSolid(e->getBounds());
        }
        // RotatingBlade намеренно пропускается — обработка ниже вручную.
    }

    // ── 3. Перенос игрока ДО физики (если стоял на лопасти в прошлом тике) ──
    //
    // Используем m_bladeCarryVelocity, которую обновили в прошлом тике.
    // Это устраняет задержку: игрок уже "переехал" вместе с лопастью,
    // прежде чем player.update() решает коллизии.
    if (m_bladeSupportActive && !m_input.jumpPressed) {
        m_player.applyExternalDisplacement(m_bladeCarryVelocity * dt);
    }

    // ── 3b. Перенос игрока: MovingPlatform (позиционный сдвиг) ─────────────
    {
        const sf::FloatRect hb = m_player.getHitbox();
        sf::FloatRect foot{hb.left + 2.f, hb.top + hb.height, hb.width - 4.f, 6.f};
        for (auto& e : m_entities) {
            auto* mp = dynamic_cast<MovingPlatform*>(e.get());
            if (!mp || mp->isExpired()) continue;
            if (foot.intersects(mp->getBounds())) {
                m_player.applyPlatformCarry(mp->getDelta());
                break;
            }
        }
    }

    // ── 4. Физика игрока ─────────────────────────────────────────────────────
    m_player.update(dt, m_level, m_input);

    // ── 5. Ручная коллизия «игрок ↔ лопасть» ────────────────────────────────
    {
        const sf::FloatRect playerHb = m_player.getHitbox();
        RotatingBlade* hitBlade = findBladeSupport(m_entities, playerHb);

        if (hitBlade && !m_input.jumpPressed) {
            // ── Снап: прижимаем игрока к поверхности лопасти ────────────────
            const sf::FloatRect b        = hitBlade->getBounds();
            const float playerBottom     = m_player.getHitbox().top + m_player.getHitbox().height;
            const float penetration      = playerBottom - b.top;
            if (penetration > 0.f) {
                // Выталкиваем ровно на глубину проникновения.
                m_player.applyExternalDisplacement({0.f, -penetration});
            }

            // Обнуляем нисходящую скорость, иначе на следующем тике гравитация
            // снова опустит игрока в лопасть (и нам придётся снапить снова).
            m_player.zeroFallVelocity();

            // Говорим физике: "мы на земле". На следующем тике player.update()
            // прочитает m_onGround=true и применит наземные ускорение и трение.
            m_player.forceOnGround();

            // Обновляем вектор переноса для следующего тика (п.3).
            m_bladeSupportActive = true;
            m_bladeCarryVelocity = hitBlade->pointLinearVelocity(m_player.getCenter());

        } else if (m_bladeSupportActive) {
            // Игрок потерял контакт с лопастью.
            if (m_input.jumpPressed) {
                // Намеренный прыжок: передаём инерцию вращения,
                // чтобы игрок "вылетел" по касательной — как в классических платформерах.
                m_player.addExternalVelocity(m_bladeCarryVelocity);
            }
            // При простом соскальзывании с края импульс НЕ добавляем:
            // игрок просто падает под действием гравитации.
            m_bladeSupportActive = false;
            m_bladeCarryVelocity = {0.f, 0.f};
        }
    }

    // ── 6. Исчезающие платформы ──────────────────────────────────────────────
    const sf::FloatRect playerHitbox = m_player.getHitbox();
    for (auto& e : m_entities) {
        auto* vp = dynamic_cast<VanishingPlatform*>(e.get());
        if (!vp || vp->isExpired()) continue;
        if (isStandingOn(playerHitbox, vp->getBounds())) {
            vp->onCollision();
        }
    }

    // ── 6b. Конвейерные ленты ────────────────────────────────────────────────
    for (auto& e : m_entities) {
        auto* belt = dynamic_cast<ConveyorPlatform*>(e.get());
        if (!belt || belt->isExpired()) continue;
        if (!isStandingOn(m_player.getHitbox(), belt->getBounds())) continue;
        m_player.applyExternalDisplacement(belt->velocity() * dt);
    }

    // ── 7. Коллизия снарядов ─────────────────────────────────────────────────
    for (auto& e : m_entities) {
        auto* proj = dynamic_cast<Projectile*>(e.get());
        if (!proj || proj->isExpired()) continue;
        if (m_level.overlapsSolid(proj->getBounds())) {
            proj->destroy();
        } else if (proj->getBounds().intersects(m_player.getHitbox())) {
            m_player.kill();
        }
    }

    // ── 8. Лезвие убивает при контакте сбоку/снизу (сверху стоять можно) ─────
    {
        const sf::FloatRect phb = m_player.getHitbox();
        for (auto& e : m_entities) {
            auto* blade = dynamic_cast<RotatingBlade*>(e.get());
            if (!blade || blade->isExpired()) continue;
            if (blade->getBounds().intersects(phb) && !isStandingOn(phb, blade->getBounds())) {
                m_player.kill();
                break;
            }
        }
    }

    // ── 9. Очистка мёртвых Entity ─────────────────────────────────────────────
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(),
                       [](const std::unique_ptr<Entity>& e) {
                           return e->isExpired();
                       }),
        m_entities.end());

    if (m_player.isDead()) {
        tryPlaySfx("death");
        respawnCurrentLevel();
    }
    if (m_player.reachedFinish()) {
        advanceLevelOrWin();
    }
}

void Game::drawPlaying() {
    // Камера обновляется в фазе отрисовки, чтобы использовать самый свежий
    // dt и текущую скорость игрока. Тряска и зум сглаживаются экспоненциально.
    const float dt = std::min(1.f / 30.f, m_clock.getElapsedTime().asSeconds());
    (void)dt;
    // dt у нас уже потрачен в updatePlaying; для камеры используем фиксированный
    // шаг 1/60, чтобы анимация камеры не зависела от скачков dt.
    constexpr float kCameraStep = 1.f / 60.f;
    m_camera.update(kCameraStep,
                    m_player.getCenter(),
                    m_player.velocity(),
                    m_window.getSize(),
                    m_level.pixelSize(),
                    m_overviewHeld);

    if (m_hasLevelBackground) {
        m_window.clear();
        m_window.setView(m_window.getDefaultView());
        m_window.draw(m_levelBackgroundSprite);
    } else {
        m_window.clear(sf::Color(120, 150, 190));
    }

    m_window.setView(m_camera.view());
    {
        std::map<std::string, const sf::Texture*> texMap;
        const std::string& levelBg = m_level.backgroundPath();
        for (const auto& [id, tex] : m_textures) {
            if (id == "menu_bg") continue;
            if (m_hasLevelBackground && id == levelBg) continue;
            texMap[id] = &tex;
        }
        m_level.draw(m_window, &texMap);
    }
    for (auto& e : m_entities) {
        e->draw(m_window);
    }
    m_player.draw(m_window);

    m_window.setView(m_window.getDefaultView());
    if (m_hudFontLoaded) {
        std::ostringstream oss;
        oss << "Level " << (m_levelIndex + 1) << "/" << m_levelPaths.size()
            << "   Deaths: " << m_player.deathCount()
            << "   Time: " << std::fixed << std::setprecision(1)
            << m_player.timeAlive() << "s";
        if (m_gameWon)
            oss << "   YOU WIN! (R — заново)";
        else
            oss << "   Синие = движутся  Оранжевые = исчезают";
        m_hud.setString(oss.str());
        m_window.draw(m_hud);
    }
}

void Game::run() {
    if (m_hudFontLoaded) {
        m_hud.setFont(m_fonts["main"]);
        m_hud.setCharacterSize(28);
        m_hud.setFillColor(sf::Color::White);
        m_hud.setOutlineColor(sf::Color::Black);
        m_hud.setOutlineThickness(2.f);
        m_hud.setPosition(16.f, 12.f);
    }

    while (m_window.isOpen()) {
        const float dt = clampDt(m_clock.restart().asSeconds());
        processEvents();

        switch (m_screen) {
        case GameScreen::MainMenu:
            drawMainMenu();
            break;
        case GameScreen::LevelSelect:
            drawLevelSelect();
            break;
        case GameScreen::Playing:
            updatePlaying(dt);
            drawPlaying();
            break;
        }

        m_window.display();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            m_window.close();

        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left) {
            const sf::Vector2f pos(
                static_cast<float>(event.mouseButton.x),
                static_cast<float>(event.mouseButton.y));
            if (m_screen == GameScreen::MainMenu ||
                m_screen == GameScreen::LevelSelect) {
                handleMenuClick(pos);
            }
        }

        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                if (m_screen == GameScreen::Playing)
                    m_screen = GameScreen::LevelSelect;
                else
                    m_window.close();
            }
            if (event.key.code == sf::Keyboard::R &&
                m_screen == GameScreen::Playing) {
                restartFromFirstLevel();
            }
        }
    }

    if (m_screen != GameScreen::Playing) {
        m_input = InputState{};
        m_prevSpaceDown = false;
        return;
    }

    m_input.left  = sf::Keyboard::isKeyPressed(sf::Keyboard::A)     ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
    m_input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::D)     ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
    m_input.up    = sf::Keyboard::isKeyPressed(sf::Keyboard::W)     ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
    m_input.down  = sf::Keyboard::isKeyPressed(sf::Keyboard::S)     ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
    m_input.sprint = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

    // Зажатая C — обзор всего уровня. (LShift занят спринтом.)
    m_overviewHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::C);

    const bool spaceNow    = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
    m_input.jumpPressed    = spaceNow && !m_prevSpaceDown;
    m_prevSpaceDown        = spaceNow;
    m_input.jumpHeld       = spaceNow;
}