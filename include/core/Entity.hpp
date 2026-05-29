#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class Entity {
public:
    virtual ~Entity() {}

    void initTexture(const sf::Texture& texture) {
        m_sprite.setTexture(texture);
    }

    virtual void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;

    void setPosition(float x, float y) { m_sprite.setPosition(x, y); }
    sf::Vector2f getPosition() const   { return m_sprite.getPosition(); }

    virtual bool isExpired() const { return m_isExpired; }
    void destroy() { m_isExpired = true; }

    void move(sf::Vector2f offset)            { m_sprite.move(offset); }
    void move(float offsetX, float offsetY)   { m_sprite.move(offsetX, offsetY); }

    virtual sf::FloatRect getBounds() const { return m_sprite.getGlobalBounds(); }

protected:
    sf::Sprite m_sprite;
    bool m_isExpired = false;
};

#endif
