#include "platforms/TreadmillPlatform.hpp"
#include "core/EntityFactory.hpp"
#include <algorithm>

TreadmillPlatform::TreadmillPlatform(const sf::Texture* textureOrNull,
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
            m_sprite.setScale(m_bounds.width / static_cast<float>(sz.x),
                              m_visualHeight / static_cast<float>(sz.y));
        }
        m_sprite.setPosition(m_visualOrigin);
        m_hasTexture = true;
    } else {
        m_fallback.setSize({m_bounds.width, m_visualHeight});
        m_fallback.setPosition(m_visualOrigin);
        m_fallback.setFillColor(ek::kColorTreadmill);
        m_fallback.setOutlineColor(sf::Color(40, 40, 50));
        m_fallback.setOutlineThickness(1.f);
    }
}

void TreadmillPlatform::update(float /*dt*/,
                               std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {}

void TreadmillPlatform::draw(sf::RenderWindow& window) {
    if (m_hasTexture) window.draw(m_sprite);
    else              window.draw(m_fallback);
}

sf::FloatRect TreadmillPlatform::getBounds() const {
    return m_bounds;
}

void TreadmillPlatform::setVelocity(sf::Vector2f v) {
    m_velocity = v;
}
