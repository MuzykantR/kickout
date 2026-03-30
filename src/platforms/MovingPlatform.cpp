#include "platforms/MovingPlatform.hpp"
#include <cmath>

MovingPlatform::MovingPlatform(const sf::Texture& texture, float x, float y, float width, float height)
    : Platform(texture, x, y, width, height), m_pointA(x, y), m_pointB(x, y), m_currentGoal(x, y), m_speed(0.f) 
{}

void MovingPlatform::setMovement(sf::Vector2f offset, float speed) {
    m_speed = speed;
    m_pointA = getPosition();
    m_pointB = m_pointA + offset;
    m_currentGoal = m_pointB;
}

void MovingPlatform::update(float deltaTime, std::vector<std::unique_ptr<Entity>>& newEntities) {
    if (m_speed == 0.f) return;

    sf::Vector2f currentPos = getPosition();

    sf::Vector2f distToGoalVec = m_currentGoal - currentPos;
    float distToGoal = std::sqrt(distToGoalVec.x*distToGoalVec.x + distToGoalVec.y*distToGoalVec.y);

    if (distToGoal < m_speed * deltaTime) {
        if (m_currentGoal == m_pointB) {
            m_currentGoal = m_pointA;
        } else {
            m_currentGoal = m_pointB;
        }
    }

    distToGoalVec = m_currentGoal - currentPos;
    distToGoal = std::sqrt(distToGoalVec.x * distToGoalVec.x + distToGoalVec.y * distToGoalVec.y);
    
    // Просто нормализируем вектор, приводим к размерности 1 и двигаем платформу
    if (distToGoal > 0) {
        sf::Vector2f unitDir = distToGoalVec / distToGoal;
        sf::Vector2f offset = unitDir * m_speed * deltaTime;
        m_sprite.move(offset);
    }
}