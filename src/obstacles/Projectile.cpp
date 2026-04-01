#include "obstacles/Projectile.hpp"
#include <cmath>

Projectile::Projectile(const sf::Texture& tex, sf::Vector2f pos, sf::Vector2f velocity, float gravity)
    : m_velocity(velocity), m_gravity(gravity)
{
    initTexture(tex);
    setPosition(pos.x, pos.y);
    
    // Разворачиваем спрайт по направлению полета
    float angle = std::atan2(velocity.y, velocity.x) * 180.f / 3.14159f;
    m_sprite.setRotation(angle);
}

void Projectile::update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) {
    // Движение
    m_velocity.y += m_gravity * deltaTime;
    this->move(m_velocity * deltaTime);

    // Проверка на вылет за границы (например, карта 2000x2000)
    sf::Vector2f pos = getPosition();
    if (pos.x < -500 || pos.x > 2500 || pos.y < -500 || pos.y > 2500) {
        destroy(); 
    }
}

void Projectile::draw(sf::RenderWindow& window) {
    window.draw(m_sprite);
}