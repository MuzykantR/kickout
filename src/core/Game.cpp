#include "core/Game.hpp"
#include "obstacles/Crossbow.hpp"
#include "obstacles/Projectile.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

float clampDt(float dt) {
    return std::clamp(dt, 0.f, 0.05f);
}

} // namespace

Game::Game()
    : m_window(sf::VideoMode(W_WIDTH, W_HEIGHT), W_TITLE) {
    m_levelPaths = {
        "assets/levels/level01.json",
        "assets/levels/level02.json",
    };

    try {
        loadResources();
    } catch (const std::exception& e) {
        std::cerr << "Resource Error: " << e.what() << std::endl;
    }

    if (!loadLevelIndex(0)) {
        std::cerr << "Level load failed.\n";
    }

    m_window.setFramerateLimit(60);
    std::cout << "Game Initialized!" << std::endl;
}

Game::~Game() = default;

void Game::loadResources() {
    std::cout << "--- Loading Resources ---" << std::endl;
    loadTextures();
    loadFonts();
    loadSounds();
    std::cout << "--- All Resources Loaded ---" << std::endl;
}

void Game::loadTextures() {
    const std::map<std::string, std::string> textureManifest = {
        {"obs_crossbow", "assets/textures/obstacles/crossbow.png"},
        {"obs_arrow", "assets/textures/obstacles/arrow.png"},
    };

    for (const auto& [id, path] : textureManifest) {
        if (m_textures.find(id) == m_textures.end()) {
            if (!m_textures[id].loadFromFile(path)) {
                throw std::runtime_error("Failed to load: " + path);
            }
            std::cout << "[Texture] Loaded: " << id << std::endl;
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
    if (!m_hudFontLoaded) {
        std::cerr << "[Font] No system font found; HUD text disabled." << std::endl;
    }
}

void Game::loadSounds() {
    auto tryLoadBuffer = [this](const std::string& id, const std::string& path) {
        sf::SoundBuffer buf;
        if (buf.loadFromFile(path)) {
            m_soundBuffers[id] = std::move(buf);
            std::cout << "[Sound] Loaded: " << id << std::endl;
            return true;
        }
        return false;
    };

    tryLoadBuffer("jump", "assets/sounds/jump.wav");
    tryLoadBuffer("death", "assets/sounds/death.wav");

    if (!m_soundBuffers.empty()) {
        m_sfxReady = true;
    }
}

bool Game::loadLevelIndex(size_t index) {
    if (m_levelPaths.empty()) {
        return false;
    }

    const size_t clamped = std::min(index, m_levelPaths.size() - 1);
    std::string err;
    if (!m_level.loadFromFile(m_levelPaths[clamped], err)) {
        std::cerr << err << std::endl;
        const std::string emergency = R"({
  "version": 1,
  "tileSize": 48,
  "grid": { "columns": 7, "rows": 4 },
  "spawn": { "x": 72, "y": 72 },
  "platforms": [
    { "type": "solid", "x": 0, "y": 0, "w": 336, "h": 48 },
    { "type": "solid", "x": 0, "y": 144, "w": 336, "h": 48 },
    { "type": "solid", "x": 0, "y": 0, "w": 48, "h": 192 },
    { "type": "solid", "x": 288, "y": 0, "w": 48, "h": 192 },
    { "type": "finish", "x": 240, "y": 48, "w": 48, "h": 48 }
  ],
  "traps": []
})";
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
    m_gameWon = false;
    return true;
}

void Game::spawnPlacedEntitiesFromLevel() {
    for (const auto& pe : m_level.placedEntities()) {
        spawnEntity(pe);
    }
}

void Game::spawnEntity(const PlacedEntity& pe) {
    if (pe.type == "crossbow" || pe.type == "crossbow_fast") {
        m_entities.push_back(std::make_unique<Crossbow>(
            getTexture("obs_crossbow"),
            getTexture("obs_arrow"),
            pe.position,
            pe.fireInterval,
            pe.projectileVelocity));
    } else {
        std::cerr << "[Factory] Unknown trap type: " << pe.type << std::endl;
    }
}

const sf::Texture& Game::getTexture(const std::string& name) const {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        return it->second;
    }
    throw std::runtime_error("Texture not found in Assets: " + name);
}

void Game::tryPlaySfx(const std::string& name) {
    if (!m_sfxReady) {
        return;
    }
    auto it = m_soundBuffers.find(name);
    if (it == m_soundBuffers.end()) {
        return;
    }
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
    loadLevelIndex(0);
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
        float deltaTime = clampDt(m_clock.restart().asSeconds());

        processEvents();

        if (!m_gameWon) {
            m_player.update(deltaTime, m_level, m_input);

            std::vector<std::unique_ptr<Entity>> spawned;
            for (auto& entity : m_entities) {
                entity->update(deltaTime, spawned);
            }
            std::vector<std::unique_ptr<Entity>> none;
            for (auto& e : spawned) {
                e->update(deltaTime, none);
                m_entities.push_back(std::move(e));
            }

            for (auto& entity : m_entities) {
                auto* proj = dynamic_cast<Projectile*>(entity.get());
                if (proj == nullptr || proj->isExpired()) {
                    continue;
                }
                if (m_level.overlapsSolid(proj->getBounds())) {
                    proj->destroy();
                } else if (proj->getBounds().intersects(m_player.getHitbox())) {
                    m_player.kill();
                }
            }

            m_entities.erase(
                std::remove_if(m_entities.begin(), m_entities.end(),
                               [](const std::unique_ptr<Entity>& e) { return e->isExpired(); }),
                m_entities.end());

            if (m_player.isDead()) {
                tryPlaySfx("death");
                m_player.respawn(m_level);
            }

            if (m_player.reachedFinish()) {
                advanceLevelOrWin();
            }
        }

        const sf::Vector2f camTarget = m_player.getCenter();
        m_camera.follow(camTarget, m_window.getSize(), m_level.pixelSize());
        m_window.setView(m_camera.view());

        m_window.clear(sf::Color(120, 150, 190));
        m_level.draw(m_window);
        for (auto& entity : m_entities) {
            entity->draw(m_window);
        }
        m_player.draw(m_window);

        m_window.setView(m_window.getDefaultView());

        if (m_hudFontLoaded) {
            std::ostringstream oss;
            oss << "Level " << (m_levelIndex + 1) << " / " << m_levelPaths.size();
            oss << "   Deaths: " << m_player.deathCount();
            oss << "   Time: " << std::fixed << std::setprecision(1) << m_player.timeAlive() << "s";
            if (m_gameWon) {
                oss << "   YOU WIN! (R — заново)";
            } else {
                oss << "   Arrows/шипы — смерть. Shift — спринт.";
            }
            m_hud.setString(oss.str());
            m_window.draw(m_hud);
        }

        m_window.display();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                m_window.close();
            }
            if (event.key.code == sf::Keyboard::R) {
                restartFromFirstLevel();
            }
        }
    }

    m_input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
    m_input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::D) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
    m_input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
    m_input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
    m_input.sprint = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

    const bool spaceNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
    m_input.jumpPressed = spaceNow && !m_prevSpaceDown;
    m_prevSpaceDown = spaceNow;
    m_input.jumpHeld = spaceNow;
}
