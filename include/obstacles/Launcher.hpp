#ifndef LAUNCHER_HPP
#define LAUNCHER_HPP

#include "core/Entity.hpp"
#include <vector>
#include <memory>

class Launcher : public Entity {
public:
    Launcher(const sf::Texture& tex, const sf::Texture& projTex, sf::Vector2f pos, float interval);

    // Логика таймера
    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;

protected:
    const sf::Texture& m_projectileTexture;
    float m_timer = 0.0f;
    float m_fireInterval;

    virtual void fire(std::vector<std::unique_ptr<Entity>>& newEntities) = 0;
};

#endif