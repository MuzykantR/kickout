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
#include "platforms/MovingPlatform.hpp"
#include "platforms/VanishingPlatform.hpp"
#include "platforms/ConveyorPlatform.hpp"
#include "platforms/WoodPlatform.hpp"
#include "platforms/TreadmillPlatform.hpp"
#include "world/Camera.hpp"
#include "world/Level.hpp"

enum class GameScreen { MainMenu, LevelSelect, Playing };

class Game {
public:
    Game();
    ~Game();

    void run();

    const sf::Texture& getTexture(const std::string& name) const;
    const sf::Texture& tryGetTexture(const std::string& name) const;
    void spawnEntity(const PlacedEntity& trap);

private:
    void processEvents();

    void loadResources();
    void loadTextures();
    void loadFonts();
    void loadSounds();

    void discoverLevelPaths();
    bool loadLevelIndex(size_t index);
    void spawnPlacedEntitiesFromLevel();
    void spawnDynamicPlatformsFromLevel();   // создаёт MovingPlatform / VanishingPlatform
    void respawnCurrentLevel();              // сброс сущностей + respawn игрока без перезагрузки файла
    void advanceLevelOrWin();
    void restartFromFirstLevel();
    void tryPlaySfx(const std::string& name);

    void buildLevelSelectButtons();
    void drawButton(const sf::FloatRect& rect, const std::string& label);
    void drawMainMenu();
    void drawLevelSelect();
    void updatePlaying(float dt);
    void drawPlaying();
    void handleMenuClick(sf::Vector2f pos);

    const unsigned int W_WIDTH  = 1920;
    const unsigned int W_HEIGHT = 1080;
    const std::string  W_TITLE  = "Kickout";

    Level  m_level;
    Camera m_camera;
    Player m_player;
    InputState m_input;

    GameScreen m_screen = GameScreen::MainMenu;

    std::vector<std::string> m_levelPaths;
    size_t m_levelIndex = 0;
    bool   m_gameWon    = false;

    bool m_hasMenuBackground = false;
    sf::Sprite m_menuBackgroundSprite;
    bool m_hasLevelBackground = false;
    sf::Sprite m_levelBackgroundSprite;

    void setupLevelBackground();

    sf::FloatRect m_playButtonRect;
    sf::FloatRect m_backButtonRect;
    std::vector<sf::FloatRect> m_levelButtonRects;
    bool m_bladeSupportActive = false;
    sf::Vector2f m_bladeCarryVelocity{0.f, 0.f};

    std::vector<std::unique_ptr<Entity>> m_entities;

    std::map<std::string, sf::Texture> m_textures;
    std::map<std::string, sf::Font>    m_fonts;
    std::map<std::string, sf::SoundBuffer> m_soundBuffers;

    sf::Sound m_sfx;
    bool      m_sfxReady = false;

    bool m_overviewHeld = false;   // Зажата клавиша обзора всего уровня (C).

    // ── Переход между состояниями уровня (fade-out → action → fade-in) ──
    enum class Transition { None, Respawn, Advance };
    Transition m_transition       = Transition::None;
    float      m_fadeAlpha        = 0.f;   // 0..1
    float      m_fadeDir          = 0.f;   // +1 → in (towards black), -1 → out (towards transparent)
    static constexpr float kFadeSpeed = 3.0f;

    // Однопиксельная белая текстура — заглушка для Entity-платформ
    sf::Texture m_whiteTex;

    sf::Clock       m_clock;
    sf::RenderWindow m_window;

    sf::Text m_hud;
    bool     m_hudFontLoaded = false;
    bool     m_prevSpaceDown = false;
};

#endif