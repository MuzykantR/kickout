#include "platforms/Platform.hpp"

Platform::Platform(const sf::Texture& texture, float x, float y, float width, float height) {
    initTexture(texture);

    // Задаём размеры спрайта в зависимости от картинки
    float scaleX = width / m_sprite.getLocalBounds().width;
    float scaleY = height / m_sprite.getLocalBounds().height;
    m_sprite.setScale(scaleX, scaleY);

    setPosition(x, y);
}

void Platform::update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) {
}

void Platform::draw(sf::RenderWindow& window) {
    window.draw(m_sprite);
}

void Platform::setColor(sf::Color color) {
    m_sprite.setColor(color);
}