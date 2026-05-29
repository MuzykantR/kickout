#include "obstacles/BarbedWire.hpp"
#include "core/EntityFactory.hpp"

BarbedWire::BarbedWire(const sf::Texture* textureOrNull,
                       sf::Vector2f topLeft,
                       sf::Vector2f size)
    : m_bounds{topLeft.x, topLeft.y, size.x, size.y} {
    if (textureOrNull != nullptr) {
        m_sprite.setTexture(*textureOrNull, true);
        const sf::Vector2u sz = textureOrNull->getSize();
        if (sz.x > 0u && sz.y > 0u) {
            m_sprite.setScale(size.x / static_cast<float>(sz.x),
                              size.y / static_cast<float>(sz.y));
        }
        m_sprite.setPosition(topLeft);
        m_hasTexture = true;
    } else {
        m_fallback.setSize(size);
        m_fallback.setPosition(topLeft);
        m_fallback.setFillColor(ek::kColorBarbedWire);
        m_fallback.setOutlineColor(sf::Color(60, 10, 10));
        m_fallback.setOutlineThickness(1.f);
    }
}

void BarbedWire::update(float /*dt*/,
                        std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {}

void BarbedWire::draw(sf::RenderWindow& window) {
    if (m_hasTexture) window.draw(m_sprite);
    else              window.draw(m_fallback);
}

sf::FloatRect BarbedWire::getBounds() const { return m_bounds; }
