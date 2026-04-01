#include "core/Game.hpp"
#include "obstacles/Crossbow.hpp"
#include <stdexcept>
#include <algorithm>

Game::Game()
    : m_window(sf::VideoMode(W_WIDTH, W_HEIGHT), W_TITLE)
{   
    try {
        loadResources();
        auto crossbow = std::make_unique<Crossbow>(
        getTexture("obs_crossbow"), 
        getTexture("obs_arrow"), 
        sf::Vector2f(100.f, 400.f)
        );
        m_entities.push_back(std::move(crossbow));
    } catch (const std::exception& e) {
        // Выход из программы при отсутствии ресурсов
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
    // Список: { "ID текстуры", "Путь к файлу" }
    const std::map<std::string, std::string> textureManifest = {
        {"obs_crossbow", "assets/textures/obstacles/crossbow.png"},
        {"obs_arrow",    "assets/textures/obstacles/arrow.png"},
    };

    for (const auto& [id, path] : textureManifest) {
        // Проверяем, не загружена ли уже текстура
        if (m_textures.find(id) == m_textures.end()) {
            if (!m_textures[id].loadFromFile(path)) {
                throw std::runtime_error("Failed to load: " + path);
            }
            std::cout << "[Texture] Loaded: " << id << std::endl;
        }
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

void Game::spawnEntity(const std::string& type, sf::Vector2f pos) {
    if (type == "crossbow") {   
        m_entities.push_back(std::make_unique<Crossbow>(
            getTexture("obs_crossbow"), 
            getTexture("obs_arrow"), 
            pos
        ));
    } 
    else if (type == "platform") {
        // Логика для платформ
    }
    else if (type == "player") {
        // Логика для игрока
    }
    
    std::cout << "[Factory] Spawned: " << type << " at (" << pos.x << ", " << pos.y << ")" << std::endl;
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
    // 1. Очередь для новых объектов (стрел)
    std::vector<std::unique_ptr<Entity>> newEntities;

    // 2. Обновляем всех, кто уже есть в мире
    for (auto& entity : m_entities) {
        entity->update(deltaTime, newEntities);
    }

    // 3. Переносим созданные стрелы в основной список
    for (auto& ne : newEntities) {
        m_entities.push_back(std::move(ne));
    }

    // 4. Очистка "мертвых" объектов (те, что улетели за экран)
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(),
            [](const std::unique_ptr<Entity>& e) {
                return e->isExpired();
            }),
        m_entities.end()
    );
}

void Game::render() {
    m_window.clear(sf::Color(255, 255, 255));

    for (auto& entity : m_entities) {
        entity->draw(m_window);
    }

    m_window.display();
}

