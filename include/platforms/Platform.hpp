#pragma once
#include <SFML/Graphics.hpp>
#include "core/Entity.hpp"

class Platform : public Entity {
public:
    Platform(const sf::Texture& texture, float x, float y, float width, float height);
    virtual ~Platform() = default;

    // функции из Entity
    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;

    void setColor(sf::Color color);
};