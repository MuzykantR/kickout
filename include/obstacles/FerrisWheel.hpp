#ifndef FERRIS_WHEEL_HPP
#define FERRIS_WHEEL_HPP

#include "core/Entity.hpp"

// Чёртово колесо (UCH: Ferris Wheel).
// Один объект-родитель управляет N кабинками-платформами фиксированного
// размера (ek::kSizeFerrisCabin), расположенными равномерно по окружности.
// Кабинки регистрируются как dynamic solid'ы в Game, а игрок переносится
// линейной скоростью точки обода, если стоит сверху.
class FerrisWheel : public Entity {
public:
    FerrisWheel(const sf::Texture* hubTex,
                const sf::Texture* cabinTex,
                sf::Vector2f center,
                float radius,
                int  cabinCount,
                float angularSpeedDeg,
                sf::Vector2f cabinSize);

    void update(float dt, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

    int  cabinCount()                 const { return m_cabinCount; }
    sf::FloatRect cabinBounds(int i)  const;
    sf::Vector2f  cabinLinearVelocity(int i) const;

private:
    sf::Vector2f cabinCenter(int i) const;

    sf::Vector2f m_center;
    float        m_radius;
    int          m_cabinCount;
    float        m_angularSpeed;   // град/сек
    float        m_phaseDeg = 0.f;
    sf::Vector2f m_cabinSize;

    bool               m_hasHub      = false;
    bool               m_hasCabinTex = false;
    sf::Sprite         m_hubSprite;
    sf::Sprite         m_cabinSprite;
    sf::CircleShape    m_hubFallback;
    sf::RectangleShape m_cabinFallback;
};

#endif
