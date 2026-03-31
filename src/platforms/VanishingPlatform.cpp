#include "platforms/VanishingPlatform.hpp"

VanishingPlatform::VanishingPlatform(const sf::Texture& texture, float x, float y, float width, float height)
    : Platform(texture, x, y, width, height) 
{}

void VanishingPlatform::setDeathTime(float time) {
    m_deathTime = time;
    m_maxDeathTime = time;
}

void VanishingPlatform::onCollision() {
    m_isTouched = true;
}

void VanishingPlatform::update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) {
    if (!m_isTouched || isExpired()) return;

    m_deathTime -= deltaTime;

    float alpha_value = m_deathTime / m_maxDeathTime;
    if (alpha_value < 0.f) alpha_value = 0.f;

    if (alpha_value < 0.f) alpha_value = 0.f;
    if (alpha_value > 1.f) alpha_value = 1.f;

    int alpha = static_cast<int>(alpha_value * 255);
    m_sprite.setColor(sf::Color(255, 255, 255, alpha));

    if (m_deathTime <= 0.f) {
        destroy();
    }
}