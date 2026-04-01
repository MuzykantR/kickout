#include "obstacles/Crossbow.hpp"
#include "obstacles/Projectile.hpp"

Crossbow::Crossbow(const sf::Texture& tex, const sf::Texture& projTex, sf::Vector2f pos)
    : Launcher(tex, projTex, pos, DEFAULT_FIRE_INTERVAL) {}

void Crossbow::fire(std::vector<std::unique_ptr<Entity>>& newEntities) {
    sf::Vector2f arrowVelocity(400.0f, 0.0f); 

    // Передаем текстуру
    auto arrow = std::make_unique<Projectile>(
        m_projectileTexture, 
        getPosition(), 
        arrowVelocity, 
        0.0f // гравитация 0
    );

    // Отдаем стрелу в общую очередь спавна
    newEntities.push_back(std::move(arrow));
}