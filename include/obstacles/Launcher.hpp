#pragma once

#include "core/Entity.hpp"
#include <vector>
#include <memory>

class Launcher : public Entity {
public:
    Launcher(const sf::Texture& tex, const sf::Texture& projTex,
             sf::Vector2f pos, float interval, sf::FloatRect levelBounds);

    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;

protected:
    const sf::Texture& m_projectileTexture;
    sf::FloatRect      m_levelBounds;
    float              m_timer        = 0.f;
    float              m_fireInterval = 1.5f;

    virtual void fire(std::vector<std::unique_ptr<Entity>>& newEntities) = 0;
};
