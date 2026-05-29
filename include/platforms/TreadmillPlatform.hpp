#ifndef TREADMILL_PLATFORM_HPP
#define TREADMILL_PLATFORM_HPP

#include "core/Entity.hpp"

// Беговая дорожка (UCH: Treadmill). От ConveyorPlatform отличается смысловой
// ролью: фиксированная по ТЗ скорость, отдельная тематическая текстура и
// другой fallback-цвет. Физика — идентична: толкает игрока вдоль `velocity`.
class TreadmillPlatform : public Entity {
public:
    static constexpr float HitboxHeight = 32.f;

    TreadmillPlatform(const sf::Texture* textureOrNull,
                      float x, float y,
                      float widthPixels,
                      float visualHeight = 32.f);

    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

    void setVelocity(sf::Vector2f velocity);
    sf::Vector2f velocity() const { return m_velocity; }

private:
    sf::FloatRect      m_bounds{};
    sf::Vector2f       m_velocity{0.f, 0.f};
    bool               m_hasTexture = false;
    sf::Sprite         m_sprite;
    sf::RectangleShape m_fallback;
    float              m_visualHeight = 32.f;
    sf::Vector2f       m_visualOrigin{0.f, 0.f};
};

#endif
