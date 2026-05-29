#include "platforms/ConveyorPlatform.hpp"
#include <algorithm>

ConveyorPlatform::ConveyorPlatform(const sf::Texture& endL,
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

void ConveyorPlatform::update(float deltaTime,
                              std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {
    (void)deltaTime;
}

void ConveyorPlatform::draw(sf::RenderWindow& window) {
    for (const auto& tile : m_tiles) {
        window.draw(tile);
    }
}

sf::FloatRect ConveyorPlatform::getBounds() const {
    return m_bounds;
}

void ConveyorPlatform::setVelocity(sf::Vector2f velocity) {
    m_velocity = velocity;
}
