#pragma once

#include "obstacles/Launcher.hpp"

// Стреляет снарядом-ядром с дугой. Угол (deg) и гравитация на снаряд
// задаются при создании; скорость — через projectileVelocity из JSON.
class Cannon : public Launcher {
public:
    Cannon(const sf::Texture& tex, const sf::Texture& projTex,
           sf::Vector2f pos, float fireInterval,
           float angledeg, float projSpeed, float projGravity,
           sf::FloatRect levelBounds);

    void fire(std::vector<std::unique_ptr<Entity>>& newEntities) override;

private:
    sf::Vector2f m_projVelocity;
    float        m_projGravity;
};
