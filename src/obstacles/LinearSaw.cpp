#include "obstacles/LinearSaw.hpp"
#include "core/EntityFactory.hpp"
#include <cmath>

LinearSaw::LinearSaw(const sf::Texture* textureOrNull,
                     sf::Vector2f startPosition,
                     sf::Vector2f travelOffset,
                     float speed,
                     sf::Vector2f size,
                     float spinDegPerSec)
    : m_start(startPosition),
      m_end(startPosition + travelOffset),
      m_current(startPosition),
      m_size(size),
      m_speed(std::max(0.f, speed)),
      m_spinSpeed(spinDegPerSec) {
    if (textureOrNull != nullptr) {
        m_sprite.setTexture(*textureOrNull, true);
        const sf::Vector2u sz = textureOrNull->getSize();
        if (sz.x > 0u && sz.y > 0u) {
            m_sprite.setOrigin(static_cast<float>(sz.x) * 0.5f,
                               static_cast<float>(sz.y) * 0.5f);
            m_sprite.setScale(size.x / static_cast<float>(sz.x),
                              size.y / static_cast<float>(sz.y));
        }
        m_hasTexture = true;
    } else {
        const float r = std::min(size.x, size.y) * 0.5f;
        m_fallback.setRadius(r);
        m_fallback.setOrigin(r, r);
        m_fallback.setFillColor(ek::kColorLinearSaw);
        m_fallback.setOutlineColor(sf::Color(60, 60, 80));
        m_fallback.setOutlineThickness(2.f);
    }
}

void LinearSaw::update(float dt, std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {
    m_spinAngle += m_spinSpeed * dt;

    if (m_speed <= 0.f) return;

    const sf::Vector2f goal = m_movingToEnd ? m_end : m_start;
    const sf::Vector2f toGoal = goal - m_current;
    const float dist = std::sqrt(toGoal.x * toGoal.x + toGoal.y * toGoal.y);
    const float step = m_speed * dt;
    if (dist <= step) {
        m_current = goal;
        m_movingToEnd = !m_movingToEnd;
    } else {
        m_current += sf::Vector2f{toGoal.x / dist * step, toGoal.y / dist * step};
    }
}

void LinearSaw::draw(sf::RenderWindow& window) {
    const sf::Vector2f center{m_current.x + m_size.x * 0.5f,
                              m_current.y + m_size.y * 0.5f};
    if (m_hasTexture) {
        m_sprite.setPosition(center);
        m_sprite.setRotation(m_spinAngle);
        window.draw(m_sprite);
    } else {
        m_fallback.setPosition(center);
        m_fallback.setRotation(m_spinAngle);
        window.draw(m_fallback);
    }
}

sf::FloatRect LinearSaw::getBounds() const {
    return {m_current.x, m_current.y, m_size.x, m_size.y};
}
