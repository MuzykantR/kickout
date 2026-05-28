#include "obstacles/Cannon.hpp"
#include "obstacles/Projectile.hpp"
#include <cmath>

static constexpr float kPi = 3.14159265f;

Cannon::Cannon(const sf::Texture& tex, const sf::Texture& projTex,
               sf::Vector2f pos, float fireInterval,
               float angledeg, float projSpeed, float projGravity,
               sf::FloatRect levelBounds)
    : Launcher(tex, projTex, pos, fireInterval, levelBounds)
    , m_projGravity(projGravity)
{
    const float rad  = angledeg * kPi / 180.f;
    m_projVelocity   = {std::cos(rad) * projSpeed, std::sin(rad) * projSpeed};
}

void Cannon::fire(std::vector<std::unique_ptr<Entity>>& newEntities) {
    newEntities.push_back(std::make_unique<Projectile>(
        m_projectileTexture,
        getPosition(),
        m_projVelocity,
        m_projGravity,
        m_levelBounds));
}
