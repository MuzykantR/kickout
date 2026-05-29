#ifndef VANISHING_PLATFORM_HPP
#define VANISHING_PLATFORM_HPP

#include "core/Entity.hpp"
#include <vector>

class VanishingPlatform : public Entity {
public:
    static constexpr float HitboxHeight = 37.f;

    VanishingPlatform(const sf::Texture& endL,
                      const sf::Texture& middle,
                      const sf::Texture& endR,
                      float x, float y,
                      int widthInTiles,
                      float tileSize = 48.f);

    void setDeathTime(float time);
    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;
    void onCollision();

private:
    std::vector<sf::Sprite> m_tiles;
    sf::FloatRect m_bounds{};
    bool m_isTouched = false;
    float m_deathTime = 0.5f;
    float m_maxDeathTime = 0.5f;
};

#endif
