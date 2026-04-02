#ifndef VANISHING_PLATFORM_HPP
#define VANISHING_PLATFORM_HPP

#include "platforms/Platform.hpp"

class VanishingPlatform : public Platform {
public:
    VanishingPlatform(const sf::Texture& texture, float x, float y, float width, float height);

    void setDeathTime(float time);
    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void onCollision();

private:
    bool m_isTouched = false;
    float m_deathTime = 0.5f;
    float m_maxDeathTime = 0.5f;
};

#endif