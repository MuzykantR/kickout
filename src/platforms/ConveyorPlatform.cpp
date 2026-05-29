#include "platforms/ConveyorPlatform.hpp"
#include <algorithm>

ConveyorPlatform::ConveyorPlatform(const sf::Texture* textureOrNull,
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
        m_fallback.setFillColor(sf::Color(120, 90, 60));     // охра — конвейер
        m_fallback.setOutlineColor(sf::Color(50, 35, 20));
        m_fallback.setOutlineThickness(1.f);
    }
}

void ConveyorPlatform::update(float deltaTime,
                              std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {
    (void)deltaTime;
}

void ConveyorPlatform::draw(sf::RenderWindow& window) {
    if (m_hasTexture) window.draw(m_sprite);
    else              window.draw(m_fallback);
}

sf::FloatRect ConveyorPlatform::getBounds() const {
    return m_bounds;
}

void ConveyorPlatform::setVelocity(sf::Vector2f velocity) {
    m_velocity = velocity;
}
