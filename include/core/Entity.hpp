#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <SFML/Graphics.hpp>

class Entity {
public:
    virtual ~Entity() {}

    // Вызывается в конструкторе дочернего класса
    void initTexture(const sf::Texture& texture) {
        m_sprite.setTexture(texture);
        // по умолчанию берет весь размер структуры
    }

    // Эти методы обязательно должны быть переопределены
    virtual void update(float deltaTime) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;

    // Общие методы
    void setPosition(float x, float y) {m_sprite.setPosition(x, y); }
    sf::Vector2f getPosition() const { return m_sprite.getPosition(); }
    
    // Хитбокс объекта
    sf::FloatRect getBounds() const { return m_sprite.getGlobalBounds(); }

protected:
    sf::Sprite m_sprite;
    sf::Texture m_texture;
};

#endif