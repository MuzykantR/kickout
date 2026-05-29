#include "platforms/VanishingPlatform.hpp"
#include <algorithm>

VanishingPlatform::VanishingPlatform(const sf::Texture* textureOrNull,
                                     float x, float y,
                                     float widthPixels,
                                     float visualHeight)
    : m_bounds{x, y + visualHeight - HitboxHeight, std::max(widthPixels, 1.f), HitboxHeight},
      m_visualHeight(visualHeight),
      m_visualOrigin{x, y} {
    if (textureOrNull != nullptr) {
        m_sprite.setTexture(*textureOrNull, true);
        const sf::Vector2u sz = textureOrNull->getSize();
        if (sz.x > 0u && sz.y > 0u) {
            m_sprite.setScale(m_bounds.width  / static_cast<float>(sz.x),
                              m_visualHeight  / static_cast<float>(sz.y));
        }
        m_sprite.setPosition(m_visualOrigin);
        m_hasTexture = true;
    } else {
        m_fallback.setSize({m_bounds.width, m_visualHeight});
        m_fallback.setPosition(m_visualOrigin);
        m_fallback.setFillColor(sf::Color(255, 165, 60));  // оранж — исчезающая
        m_fallback.setOutlineColor(sf::Color(80, 50, 20));
        m_fallback.setOutlineThickness(1.f);
    }
}

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

    const float alpha = std::clamp(m_deathTime / m_maxDeathTime, 0.f, 1.f);
    const sf::Uint8 a = static_cast<sf::Uint8>(alpha * 255.f);
    if (m_hasTexture) {
        sf::Color c = m_sprite.getColor();
        c.a = a;
        m_sprite.setColor(c);
    } else {
        sf::Color c = m_fallback.getFillColor();
        c.a = a;
        m_fallback.setFillColor(c);
    }

    if (m_deathTime <= 0.f) destroy();
}

void VanishingPlatform::draw(sf::RenderWindow& window) {
    if (m_hasTexture) window.draw(m_sprite);
    else              window.draw(m_fallback);
}

sf::FloatRect VanishingPlatform::getBounds() const {
    return m_bounds;
}
