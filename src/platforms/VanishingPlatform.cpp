#include "platforms/VanishingPlatform.hpp"
#include <algorithm>

VanishingPlatform::VanishingPlatform(const sf::Texture& endL,
                                     const sf::Texture& middle,
                                     const sf::Texture& endR,
                                     float x, float y,
                                     int widthInTiles,
                                     float tileSize)
    : m_bounds{
          x,
          y + tileSize - HitboxHeight,
          0.f,
          HitboxHeight} {
    widthInTiles = std::max(1, widthInTiles);
    m_bounds.width = static_cast<float>(widthInTiles) * tileSize;
    m_tiles.reserve(static_cast<size_t>(widthInTiles));

    for (int i = 0; i < widthInTiles; ++i) {
        const sf::Texture* tex = &middle;
        if (widthInTiles == 1) {
            tex = &middle;
        } else if (i == 0) {
            tex = &endL;
        } else if (i == widthInTiles - 1) {
            tex = &endR;
        }

        sf::Sprite spr(*tex);
        spr.setPosition(x + static_cast<float>(i) * tileSize, y);
        m_tiles.push_back(std::move(spr));
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
    const sf::Uint8 alphaByte = static_cast<sf::Uint8>(alpha * 255.f);
    for (auto& tile : m_tiles) {
        sf::Color c = tile.getColor();
        c.a = alphaByte;
        tile.setColor(c);
    }

    if (m_deathTime <= 0.f) {
        destroy();
    }
}

void VanishingPlatform::draw(sf::RenderWindow& window) {
    for (const auto& tile : m_tiles) {
        window.draw(tile);
    }
}

sf::FloatRect VanishingPlatform::getBounds() const {
    return m_bounds;
}
