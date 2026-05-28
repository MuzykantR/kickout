#pragma once

#include "core/Entity.hpp"
#include <functional>
#include <vector>
#include <memory>

// Мина: при приближении игрока переходит в вооружённое состояние,
// отсчитывает задержку, затем убивает игрока в радиусе взрыва и
// уничтожается. Аналог Bomb из UCH.
class Mine : public Entity {
public:
    Mine(const sf::Texture& tex, sf::Vector2f pos,
         float triggerRadius, float armDelay, float blastRadius,
         std::function<sf::Vector2f()> playerPosGetter,
         std::function<void()>         killPlayer);

    void update(float dt, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;

private:
    enum class State { Idle, Armed, Done };

    State  m_state        = State::Idle;
    float  m_triggerRadius;
    float  m_blastRadius;
    float  m_armDelay;
    float  m_armTimer     = 0.f;
    float  m_blinkTimer   = 0.f;

    std::function<sf::Vector2f()> m_getPlayerPos;
    std::function<void()>         m_killPlayer;
};
