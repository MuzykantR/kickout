#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/Entity.hpp"
#include "core/InputState.hpp"
#include "entities/Player.hpp"
#include "world/Camera.hpp"
#include "world/Level.hpp"

class Game {
public:
    Game();
    ~Game();

    void run();

    const sf::Texture& getTexture(const std::string& name) const;
    void spawnEntity(const PlacedEntity& trap);

private:
    void processEvents();
    void update(float deltaTime);
    void render();

    void loadResources();
    void loadTextures();
    void loadFonts();
    void loadSounds();

    bool loadLevelIndex(size_t index);
    void spawnPlacedEntitiesFromLevel();
    void advanceLevelOrWin();
    void restartFromFirstLevel();
    void tryPlaySfx(const std::string& name);

    const unsigned int W_WIDTH = 1920;
    const unsigned int W_HEIGHT = 1080;
    const std::string W_TITLE = "Kickout";

    Level m_level;
    Camera m_camera;
    Player m_player;
    InputState m_input;

    std::vector<std::string> m_levelPaths;
    size_t m_levelIndex = 0;
    bool m_gameWon = false;

    std::vector<std::unique_ptr<Entity>> m_entities;

    std::map<std::string, sf::Texture> m_textures;
    std::map<std::string, sf::Font> m_fonts;
    std::map<std::string, sf::SoundBuffer> m_soundBuffers;

    sf::Sound m_sfx;
    bool m_sfxReady = false;

    sf::Clock m_clock;
    sf::RenderWindow m_window;

    sf::Text m_hud;
    bool m_hudFontLoaded = false;
    /// Предыдущий кадр: Space зажата — для прыжка только по новому нажатию (без автоповтора)
    bool m_prevSpaceDown = false;
};

#endif
