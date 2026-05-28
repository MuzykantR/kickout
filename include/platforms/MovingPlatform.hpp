#pragma once
#include "platforms/Platform.hpp"

class MovingPlatform : public Platform {
public:
    MovingPlatform(const sf::Texture& texture, float x, float y, float width, float height);

    // offset это координаты на которые сдвинется платформы от текущей позиции
    void setMovement(sf::Vector2f offset, float speed);
    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;

private:
    sf::Vector2f m_pointA;
    sf::Vector2f m_pointB;
    float m_speed    = 0.f;
    bool  m_movingToB = true;
};