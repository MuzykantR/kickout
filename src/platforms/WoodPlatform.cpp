#include "platforms/WoodPlatform.hpp"
#include "core/EntityFactory.hpp"
#include <algorithm>
#include <cstdlib>

WoodPlatform::WoodPlatform(const sf::Texture* textureOrNull,
                           float x, float y,
                           float widthPixels,
                           float lifeTime,
                           float visualHeight)
    : m_bounds{x, y + visualHeight - HitboxHeight, std::max(widthPixels, 1.f), HitboxHeight},
      m_lifeTime(std::max(lifeTime, 0.1f)),
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
        m_fallback.setFillColor(ek::kColorWood);
        m_fallback.setOutlineColor(sf::Color(60, 35, 15));
        m_fallback.setOutlineThickness(1.f);
    }
}

void WoodPlatform::onCollision() {
    m_touched = true;
}

void WoodPlatform::update(float deltaTime,
                          std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {
    if (isExpired()) return;
    if (!m_touched) return;

    m_timer += deltaTime;
    // Лёгкая вибрация — амплитуда растёт по мере приближения к разрушению.
    const float t = std::clamp(m_timer / m_lifeTime, 0.f, 1.f);
    m_shake = t * 3.f;

    const float jx = (static_cast<float>(std::rand() % 1000) / 500.f - 1.f) * m_shake;
    const float jy = (static_cast<float>(std::rand() % 1000) / 500.f - 1.f) * m_shake;
    const sf::Vector2f drawPos = m_visualOrigin + sf::Vector2f{jx, jy};
    if (m_hasTexture) m_sprite.setPosition(drawPos);
    else              m_fallback.setPosition(drawPos);

    if (m_timer >= m_lifeTime) destroy();
}

void WoodPlatform::draw(sf::RenderWindow& window) {
    if (m_hasTexture) window.draw(m_sprite);
    else              window.draw(m_fallback);
}

sf::FloatRect WoodPlatform::getBounds() const {
    return m_bounds;
}
