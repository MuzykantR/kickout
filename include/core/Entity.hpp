#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class Entity {
public:
    virtual ~Entity() {}

    // Вызывается в конструкторе дочернего класса
    void initTexture(const sf::Texture& texture) {
        m_sprite.setTexture(texture);
        // по умолчанию берет весь размер текстуры
    }

    // Эти методы обязательно должны быть переопределены
    // update - добавляет в вектор newEntities снаряды (необходимо для obstacles, пока бесполезно для platforms/player)
    virtual void update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) = 0; 
    // Отрисовка объекта
    virtual void draw(sf::RenderWindow& window) = 0;

    // Общие методы
    void setPosition(float x, float y) { m_sprite.setPosition(x, y); }
    sf::Vector2f getPosition() const { return m_sprite.getPosition(); }

    virtual bool isExpired() const { return m_isExpired; }     // Существует ли еще элемент
    void destroy() { m_isExpired = true; }

    // Перемещение объекта относительно текущей позиции
    void move(sf::Vector2f offset) { 
        m_sprite.move(offset); 
    }

    // Перегрузка для удобства (через x, y)
    void move(float offsetX, float offsetY) { 
        m_sprite.move(offsetX, offsetY); 
    }
    
    // Хитбокс объекта
    sf::FloatRect getBounds() const { return m_sprite.getGlobalBounds(); }

protected:
    sf::Sprite m_sprite;
    sf::Texture m_texture;
    bool m_isExpired = false;
};

#endif