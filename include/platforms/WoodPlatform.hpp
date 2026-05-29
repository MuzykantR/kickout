#ifndef WOOD_PLATFORM_HPP
#define WOOD_PLATFORM_HPP

#include "core/Entity.hpp"

// Деревянная платформа (по UCH wiki: Wood).
// После первого касания она держится ровно `lifeTime` секунд,
// слегка вибрирует и обрушивается. Один цельный спрайт, растянутый
// под физический FloatRect (никаких end_l/middle/end_r тайлов).
class WoodPlatform : public Entity {
public:
    static constexpr float HitboxHeight = 32.f;

    WoodPlatform(const sf::Texture* textureOrNull,
                 float x, float y,
                 float widthPixels,
                 float lifeTime    = 1.0f,
                 float visualHeight = 32.f);

    void onCollision();
    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

private:
    sf::FloatRect      m_bounds{};
    bool               m_touched   = false;
    float              m_timer     = 0.f;
    float              m_lifeTime  = 1.0f;
    float              m_shake     = 0.f;

    bool               m_hasTexture = false;
    sf::Sprite         m_sprite;
    sf::RectangleShape m_fallback;
    float              m_visualHeight = 32.f;
    sf::Vector2f       m_visualOrigin{0.f, 0.f};
};

#endif
