#include "obstacles/Projectile.hpp"
#include <cmath>

static constexpr float kOutOfBoundsMargin = 256.f;

Projectile::Projectile(const sf::Texture& tex, sf::Vector2f pos, sf::Vector2f velocity,
                       float gravity, sf::FloatRect levelBounds)
    : m_velocity(velocity), m_gravity(gravity), m_levelBounds(levelBounds)
{
    initTexture(tex);
    setPosition(pos.x, pos.y);
    m_sprite.setRotation(std::atan2(velocity.y, velocity.x) * 180.f / 3.14159f);
}

void Projectile::update(float deltaTime, std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {
    m_velocity.y += m_gravity * deltaTime;
    move(m_velocity * deltaTime);

    const sf::Vector2f pos = getPosition();
    if (pos.x < m_levelBounds.left   - kOutOfBoundsMargin ||
        pos.x > m_levelBounds.left + m_levelBounds.width  + kOutOfBoundsMargin ||
        pos.y < m_levelBounds.top    - kOutOfBoundsMargin ||
        pos.y > m_levelBounds.top  + m_levelBounds.height + kOutOfBoundsMargin) {
        destroy();
    }
}

void Projectile::draw(sf::RenderWindow& window) {
    window.draw(m_sprite);
}
