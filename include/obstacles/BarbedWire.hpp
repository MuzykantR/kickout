#ifndef BARBED_WIRE_HPP
#define BARBED_WIRE_HPP

#include "core/Entity.hpp"

// Колючая проволока (UCH: Barbed Wire). Статичная ловушка фиксированного
// размера (ek::kSizeBarbedWire) — убивает при любом касании. Размер на
// всех уровнях одинаков, чтобы соответствовать требованию Модуля 4.
class BarbedWire : public Entity {
public:
    BarbedWire(const sf::Texture* textureOrNull,
               sf::Vector2f topLeft,
               sf::Vector2f size);

    void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

private:
    sf::FloatRect      m_bounds{};
    bool               m_hasTexture = false;
    sf::Sprite         m_sprite;
    sf::RectangleShape m_fallback;
};

#endif
