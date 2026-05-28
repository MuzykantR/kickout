#ifndef ROTATING_BLADE_HPP
#define ROTATING_BLADE_HPP

#include "core/Entity.hpp"
#include <SFML/Graphics/Rect.hpp>

class RotatingBlade : public Entity {
public:
    RotatingBlade(const sf::Texture& tex, sf::Vector2f centerPos,
                  float angularSpeedDeg = 180.f,
                  sf::Vector2f worldSize = {0.f, 0.f});

    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;
    sf::Vector2f pointLinearVelocity(const sf::Vector2f& worldPoint) const;
    sf::Vector2f center() const;

private:
    float m_angularSpeedDeg = 180.f;
};

#endif
