#include "obstacles/RotatingBlade.hpp"

RotatingBlade::RotatingBlade(const sf::Texture& tex, sf::Vector2f centerPos,
                             float angularSpeedDeg, sf::Vector2f worldSize)
    : m_angularSpeedDeg(angularSpeedDeg) {
    initTexture(tex);

    const sf::Vector2u sz = tex.getSize();
    m_sprite.setOrigin(static_cast<float>(sz.x) * 0.5f, static_cast<float>(sz.y) * 0.5f);
    if (worldSize.x > 0.f && worldSize.y > 0.f && sz.x > 0u && sz.y > 0u) {
        m_sprite.setScale(
            worldSize.x / static_cast<float>(sz.x),
            worldSize.y / static_cast<float>(sz.y));
    }
    setPosition(centerPos.x, centerPos.y);
}

void RotatingBlade::update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) {
    (void)newEntities;
    m_sprite.rotate(m_angularSpeedDeg * deltaTime);
}

void RotatingBlade::draw(sf::RenderWindow& window) {
    window.draw(m_sprite);
}

sf::Vector2f RotatingBlade::center() const {
    return getPosition();
}

sf::Vector2f RotatingBlade::pointLinearVelocity(const sf::Vector2f& worldPoint) const {
    constexpr float kDegToRad = 3.1415926535f / 180.f;
    const float omega = m_angularSpeedDeg * kDegToRad;
    const sf::Vector2f r = worldPoint - center();
    return {-omega * r.y, omega * r.x};
}
