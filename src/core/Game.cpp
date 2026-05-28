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

bool isStandingOn(const sf::FloatRect& player, const sf::FloatRect& platform) {
    constexpr float kTopEps = 6.f;
    constexpr float kSideInset = 4.f;
    const float playerBottom = player.top + player.height;
    const bool nearTop = playerBottom >= platform.top - kTopEps &&
                         playerBottom <= platform.top + kTopEps;

    const float playerLeft = player.left + kSideInset;
    const float playerRight = player.left + player.width - kSideInset;
    const float platformLeft = platform.left;
    const float platformRight = platform.left + platform.width;
    const bool overlapX = playerRight > platformLeft && playerLeft < platformRight;
    return nearTop && overlapX;
}

RotatingBlade* findBladeSupport(const std::vector<std::unique_ptr<Entity>>& entities,
                                const sf::FloatRect& playerHitbox) {
    RotatingBlade* support = nullptr;
    float bestTop = 1e9f;
    for (const auto& e : entities) {
        auto* blade = dynamic_cast<RotatingBlade*>(e.get());
        if (!blade || blade->isExpired()) continue;
        const sf::FloatRect b = blade->getBounds();
        if (!isStandingOn(playerHitbox, b)) continue;
        if (b.top < bestTop) {
            bestTop = b.top;
            support = blade;
        }
    }
    return support;
}
} // namespace

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
    };

    for (const auto& [id, path] : manifest) {
        if (!m_textures[id].loadFromFile(path)) {
            std::cerr << "[Texture] Not found (using fallback): " << path << std::endl;
            m_textures.erase(id);
            continue;
        }
        m_textures[id].setRepeated(true);
        std::cout << "[Texture] Loaded: " << id << std::endl;
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
    }

    m_levelBackgroundSprite.setTexture(it->second);
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
                m_whiteTex,
                def.bounds.left, def.bounds.top,
                def.bounds.width, def.bounds.height);
            vp->setColor(sf::Color(255, 160, 50));
            vp->setDeathTime(def.deathTime);
            m_entities.push_back(std::move(vp));
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

    if (m_hudFontLoaded) {
        sf::Text title("Kickout", m_fonts.at("main"), 72);
        const auto tb = title.getLocalBounds();
        title.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
        title.setPosition(W_WIDTH * 0.5f, W_HEIGHT * 0.28f);
        title.setFillColor(sf::Color::White);
        title.setOutlineColor(sf::Color::Black);
        title.setOutlineThickness(3.f);
        m_window.draw(title);
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

void Game::updatePlaying(float dt) {
    if (m_gameWon) return;

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

    m_level.clearDynamicSolids();
    for (auto& e : m_entities) {
        if (e->isExpired()) continue;
        if (dynamic_cast<MovingPlatform*>(e.get()) ||
            dynamic_cast<VanishingPlatform*>(e.get()) ||
            dynamic_cast<RotatingBlade*>(e.get())) {
            m_level.addDynamicSolid(e->getBounds());
        }
    }

    // Перенос игрока: MovingPlatform (позиционный сдвиг).
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

    // Перенос игрока: RotatingBlade (линейная скорость поверхности).
    {
        const sf::FloatRect preHb = m_player.getHitbox();
        RotatingBlade* supportBlade = findBladeSupport(m_entities, preHb);
        if (supportBlade != nullptr && !m_input.jumpPressed) {
            const sf::Vector2f carryVel = supportBlade->pointLinearVelocity(m_player.getCenter());
            m_player.applyPlatformCarry(carryVel * dt);
        }
    }

    m_player.update(dt, m_level, m_input);

    const sf::FloatRect playerHitbox = m_player.getHitbox();
    for (auto& e : m_entities) {
        auto* vp = dynamic_cast<VanishingPlatform*>(e.get());
        if (!vp || vp->isExpired()) continue;
        if (isStandingOn(playerHitbox, vp->getBounds())) {
            vp->onCollision();
        }
    }

    for (auto& e : m_entities) {
        auto* proj = dynamic_cast<Projectile*>(e.get());
        if (!proj || proj->isExpired()) continue;
        if (m_level.overlapsSolid(proj->getBounds())) {
            proj->destroy();
        } else if (proj->getBounds().intersects(m_player.getHitbox())) {
            m_player.kill();
        }
    }

    // Лезвие убивает игрока при контакте сбоку или снизу.
    // Стоять сверху — безопасно (isStandingOn).
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

    RotatingBlade* supportAfter = findBladeSupport(m_entities, m_player.getHitbox());
    if (supportAfter != nullptr && !m_input.jumpPressed) {
        m_bladeSupportActive = true;
        m_bladeCarryVelocity = supportAfter->pointLinearVelocity(m_player.getCenter());
    } else {
        if (m_bladeSupportActive) {
            m_player.addExternalVelocity(m_bladeCarryVelocity);
        }
        m_bladeSupportActive = false;
        m_bladeCarryVelocity = {0.f, 0.f};
    }

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
    m_camera.follow(m_player.getCenter(), m_window.getSize(), m_level.pixelSize());

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
        for (const auto& [id, tex] : m_textures) {
            if (id == "menu_bg") continue;
            if (m_hasLevelBackground && id == m_level.backgroundPath()) continue;
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

    const bool spaceNow    = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
    m_input.jumpPressed    = spaceNow && !m_prevSpaceDown;
    m_prevSpaceDown        = spaceNow;
    m_input.jumpHeld       = spaceNow;
}
