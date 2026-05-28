#include "platforms/MovingPlatform.hpp"
#include <cmath>

MovingPlatform::MovingPlatform(const sf::Texture& texture, float x, float y, float width, float height)
    : Platform(texture, x, y, width, height), m_pointA(x, y), m_pointB(x, y)
{}

void MovingPlatform::setMovement(sf::Vector2f offset, float speed) {
    m_speed     = speed;
    m_pointA    = getPosition();
    m_pointB    = m_pointA + offset;
    m_movingToB = true;
}

void MovingPlatform::update(float deltaTime, std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {
    if (m_speed == 0.f) { m_delta = {0.f, 0.f}; return; }

    const sf::Vector2f prev    = getPosition();
    const sf::Vector2f goal    = m_movingToB ? m_pointB : m_pointA;
    const sf::Vector2f toGoal  = goal - prev;
    const float        dist    = std::sqrt(toGoal.x * toGoal.x + toGoal.y * toGoal.y);
    const float        step    = m_speed * deltaTime;

    if (dist <= step) {
        m_sprite.setPosition(goal);
        m_movingToB = !m_movingToB;
    } else {
        m_sprite.move(toGoal / dist * step);
    }

    m_delta = getPosition() - prev;
}
