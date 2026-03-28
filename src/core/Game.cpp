#include "core/Game.hpp"
#include <stdexcept>

Game::Game()
    : m_window(sf::VideoMode(W_WIDTH, W_HEIGHT), W_TITLE)
{   
    try {
        loadResources();
    } catch (const std::exception& e) {
        // Выхож из программы при отсутствии ресурсов
        std::cerr << "Resource Error: " << e.what() << std::endl;
    }

    // Ограничение по FPS до 60
    m_window.setFramerateLimit(60); 
    std::cout << "Game Initialized!" << std::endl;
}

Game::~Game() {}

void Game::loadResources() {
    std::cout << "--- Loading Resources ---" << std::endl;
    loadTextures();
    loadFonts();
    loadSounds();
    std::cout << "--- All Resources Loaded ---" << std::endl;
}

void Game::loadTextures() {
    // Список текстур
    std::map<std::string, std::string> texturePaths = {
        {"player", "assets/textures/player/hero.png"},
        {"plt_stone", "assets/textures/platforms/stone.png"},
        {"obs_swing", "assets/textures/obstacles/swing_trap.png"}
    };

    for (const auto& [name, path] : texturePaths) {
        if (!m_textures[name].loadFromFile(path)) {
            throw std::runtime_error("Texture error: " + path);
        }
        std::cout << "[Texture] Loaded: " << name << std::endl;
    }
}

void Game::loadFonts() {
    // Пока пусто
    // m_fonts["main"].loadFromFile("assets/fonts/arial.ttf");
}

void Game::loadSounds() {
    // Пока пусто
    // m_sounds["jump"].loadFromFile("assets/sounds/jump.wav");
}

const sf::Texture& Game::getTexture(const std::string& name) const {
    // Ищем текстуру в контейнере
    auto it = m_textures.find(name);

    // Если наши - возвращаем ссылку(!)
    if (it != m_textures.end()) {
        return it->second;
    }

    // Если не нашли — кидаем ошибку с именем текстуры
    throw std::runtime_error("Texture not found in Assets: " + name);
}

void Game::run() {
    while (m_window.isOpen()) {
        // Считаем время кадра
        float deltaTime = m_clock.restart().asSeconds();
        processEvents();
        update(deltaTime);
        render();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }

        // Закрыть на Escape
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            m_window.close();
        }
    }
}

void Game::update(float deltaTime) {
    // player.update(deltaTime);
    // obstacles.update(deltaTime);
}

void Game::render() {
    m_window.clear(sf::Color(30, 30, 30)); // Темно-серый фон

    // m_window.draw(player);
    // m_window.draw(platform);

    m_window.display();
}

