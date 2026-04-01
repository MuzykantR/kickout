#ifndef CROSSBOW_HPP
#define CROSSBOW_HPP

#include "obstacles/Launcher.hpp"

class Crossbow : public Launcher {
public:
    static constexpr float DEFAULT_FIRE_INTERVAL = 1.5f;

    Crossbow(const sf::Texture& tex, const sf::Texture& projTex, sf::Vector2f pos);
    
    void fire(std::vector<std::unique_ptr<Entity>>& newEntities) override;
};

#endif