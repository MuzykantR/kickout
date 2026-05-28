#include "obstacles/Turret.hpp"
#include "obstacles/Projectile.hpp"
#include <cmath>

Turret::Turret(const sf::Texture& tex, const sf::Texture& projTex,
               sf::Vector2f pos, float fireInterval,
               float projSpeed, float projGravity,
               sf::FloatRect levelBounds,
               std::function<sf::Vector2f()> playerPosGetter)
    : Launcher(tex, projTex, pos, fireInterval, levelBounds)
    , m_projSpeed(projSpeed)
    , m_projGravity(projGravity)
    , m_getPlayerPos(std::move(playerPosGetter))
{}

void Turret::fire(std::vector<std::unique_ptr<Entity>>& newEntities) {
    const sf::Vector2f target = m_getPlayerPos();
    const sf::Vector2f origin = getPosition();
    const sf::Vector2f toTarget = target - origin;
    const float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

    sf::Vector2f velocity{m_projSpeed, 0.f};
    if (dist > 1.f)
        velocity = toTarget / dist * m_projSpeed;

    newEntities.push_back(std::make_unique<Projectile>(
        m_projectileTexture,
        origin,
        velocity,
        m_projGravity,
        m_levelBounds));
}
