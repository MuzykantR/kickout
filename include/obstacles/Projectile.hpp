#ifndef PROJECTILE_HPP
#define PROJECTILE_HPP

#include "core/Entity.hpp"
#include <vector>
#include <memory>

class Projectile : public Entity {
public:
    Projectile(const sf::Texture& tex, sf::Vector2f pos, sf::Vector2f velocity, float gravity = 0.0f);

    // Переопределяем методы для базового Entity
    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;

protected:
    sf::Vector2f m_velocity;
    float m_gravity;
};

#endif

