#include "obstacles/Launcher.hpp"

Launcher::Launcher(const sf::Texture& tex, const sf::Texture& projTex, sf::Vector2f pos, float interval)
    : m_projectileTexture(projTex), m_fireInterval(interval), m_timer(0.0f)
{
    initTexture(tex);
    setPosition(pos.x, pos.y);
}
void Launcher::draw(sf::RenderWindow& window) {
    window.draw(m_sprite);
}

void Launcher::update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) {
    m_timer += deltaTime;

    if (m_timer >= m_fireInterval) {
        fire(newEntities);
        m_timer = 0.0f;
    }
}