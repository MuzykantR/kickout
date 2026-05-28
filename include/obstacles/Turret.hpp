#pragma once

#include "obstacles/Launcher.hpp"
#include <functional>

// Аналог Tennis Ball Shooter из UCH: после каждого fireInterval
// вычисляет направление к игроку и выпускает снаряд.
// playerPosGetter — лямбда, возвращающая актуальный центр игрока.
class Turret : public Launcher {
public:
    Turret(const sf::Texture& tex, const sf::Texture& projTex,
           sf::Vector2f pos, float fireInterval,
           float projSpeed, float projGravity,
           sf::FloatRect levelBounds,
           std::function<sf::Vector2f()> playerPosGetter);

    void fire(std::vector<std::unique_ptr<Entity>>& newEntities) override;

private:
    float                         m_projSpeed;
    float                         m_projGravity;
    std::function<sf::Vector2f()> m_getPlayerPos;
};
