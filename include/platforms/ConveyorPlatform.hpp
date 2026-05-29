#ifndef CONVEYOR_PLATFORM_HPP
#define CONVEYOR_PLATFORM_HPP

#include "core/Entity.hpp"
#include <vector>

class ConveyorPlatform : public Entity {
public:
    static constexpr float HitboxHeight = 37.f;

    ConveyorPlatform(const sf::Texture& endL,
                     const sf::Texture& middle,
                     const sf::Texture& endR,
                     float x, float y,
                     int widthInTiles,
                     float tileSize = 48.f);

    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

    void setVelocity(sf::Vector2f velocity);
    sf::Vector2f velocity() const { return m_velocity; }

private:
    std::vector<sf::Sprite> m_tiles;
    sf::FloatRect m_bounds{};
    sf::Vector2f m_velocity{0.f, 0.f};
};

#endif
