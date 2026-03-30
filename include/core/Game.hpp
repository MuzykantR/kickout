#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <map>

class Game {
public:
    Game();
    ~Game();
    
    // Запуск игры
    void run();

    // Доступ к текстурам
    const sf::Texture& getTexture(const std::string& name) const;

private:
    // Обработка ввода
    void processEvents();

    // Обновление логики игры (физика, перемещение)
    void update(float deltaTime);

    // Отрисовка кадра
    void render();

    // Диспетчеры загрузки
    void loadResources(); 
    void loadTextures();
    void loadFonts();
    void loadSounds();

private:
    // Константы окна
    const unsigned int W_WIDTH = 1280;
    const unsigned int W_HEIGHT = 720;
    const std::string W_TITLE = "Kickout";

    // Хранилища структур
    std::map<std::string, sf::Texture> m_textures;
    std::map<std::string, sf::Font>    m_fonts;
    std::map<std::string, sf::SoundBuffer> m_sounds;
    
    // Переменные контроля времени
    sf::Clock m_clock;
    sf::RenderWindow m_window;

};

#endif