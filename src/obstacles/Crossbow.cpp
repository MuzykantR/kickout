#include "obstacles/Crossbow.hpp"
#include "obstacles/Projectile.hpp"

Crossbow::Crossbow(const sf::Texture& tex, const sf::Texture& projTex, sf::Vector2f pos,
                   float fireInterval, sf::Vector2f projectileVelocity)
    : Launcher(tex, projTex, pos, fireInterval), m_projectileVelocity(projectileVelocity) {}

void Crossbow::fire(std::vector<std::unique_ptr<Entity>>& newEntities) {
    newEntities.push_back(std::make_unique<Projectile>(
        m_projectileTexture,
        getPosition(),
        m_projectileVelocity,
        0.0f));
}
