#pragma once
#include "platforms/Platform.hpp"

class MovingPlatform : public Platform {
public:
    MovingPlatform(const sf::Texture& texture, float x, float y, float width, float height);

    void setMovement(sf::Vector2f offset, float speed);
    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;

    // Смещение за последний кадр — используется для переноса игрока.
    sf::Vector2f getDelta() const { return m_delta; }

private:
    sf::Vector2f m_pointA;
    sf::Vector2f m_pointB;
    sf::Vector2f m_delta{0.f, 0.f};
    float m_speed     = 0.f;
    bool  m_movingToB = true;
};