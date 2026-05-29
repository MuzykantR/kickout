#ifndef LINEAR_SAW_HPP
#define LINEAR_SAW_HPP

#include "core/Entity.hpp"

// Линейная пила (UCH: Linear Saw). Двигается между start и start+travel
// туда-обратно с фиксированной скоростью. Размер фиксирован (ek::kSizeLinearSaw).
// Игрока убивает при ЛЮБОМ соприкосновении.
class LinearSaw : public Entity {
public:
    LinearSaw(const sf::Texture* textureOrNull,
              sf::Vector2f startPosition,    // верхне-левый угол первого положения
              sf::Vector2f travelOffset,
              float speed,
              sf::Vector2f size,
              float spinDegPerSec = 540.f);

    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

private:
    sf::Vector2f       m_start;
    sf::Vector2f       m_end;
    sf::Vector2f       m_current;
    sf::Vector2f       m_size;
    float              m_speed       = 0.f;
    bool               m_movingToEnd = true;
    float              m_spinSpeed   = 540.f;
    float              m_spinAngle   = 0.f;

    bool               m_hasTexture  = false;
    sf::Sprite         m_sprite;
    sf::CircleShape    m_fallback;   // круг — визуально читается как пила
};

#endif
