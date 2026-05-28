#include "platforms/VanishingPlatform.hpp"
#include <algorithm>

VanishingPlatform::VanishingPlatform(const sf::Texture& texture,
                                     float x, float y, float width, float height)
    : Platform(texture, x, y, width, height)
{}

void VanishingPlatform::setDeathTime(float time) {
    m_deathTime    = std::max(time, 0.05f);
    m_maxDeathTime = m_deathTime;
}

void VanishingPlatform::onCollision() {
    m_isTouched = true;
}

void VanishingPlatform::update(float deltaTime,
                               std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {
    if (!m_isTouched || isExpired()) return;

    m_deathTime -= deltaTime;

    const float alpha_value = std::clamp(m_deathTime / m_maxDeathTime, 0.f, 1.f);

    // Сохраняем RGB-цвет, меняем только прозрачность
    sf::Color c = m_sprite.getColor();
    c.a = static_cast<sf::Uint8>(alpha_value * 255.f);
    m_sprite.setColor(c);

    if (m_deathTime <= 0.f) {
        destroy();
    }
}