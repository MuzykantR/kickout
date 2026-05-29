#ifndef VANISHING_PLATFORM_HPP
#define VANISHING_PLATFORM_HPP

#include "core/Entity.hpp"

class VanishingPlatform : public Entity {
public:
    static constexpr float HitboxHeight = 37.f;

    VanishingPlatform(const sf::Texture* textureOrNull,
                      float x, float y,
                      float widthPixels,
                      float visualHeight = 48.f);

    void setDeathTime(float time);
    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;
    void onCollision();

private:
    sf::FloatRect      m_bounds{};
    bool               m_isTouched    = false;
    float              m_deathTime    = 0.5f;
    float              m_maxDeathTime = 0.5f;

    bool               m_hasTexture   = false;
    sf::Sprite         m_sprite;
    sf::RectangleShape m_fallback;
    float              m_visualHeight = 48.f;
    sf::Vector2f       m_visualOrigin{0.f, 0.f};
};

#endif
