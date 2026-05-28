#pragma once

#include "core/Entity.hpp"
#include <vector>
#include <memory>

class Projectile : public Entity {
public:
    Projectile(const sf::Texture& tex, sf::Vector2f pos, sf::Vector2f velocity,
               float gravity, sf::FloatRect levelBounds);

    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;

protected:
    sf::Vector2f  m_velocity;
    float         m_gravity;
    sf::FloatRect m_levelBounds;
};
