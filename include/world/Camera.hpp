#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <SFML/Graphics.hpp>

class Camera {
public:
    void follow(sf::Vector2f targetCenter, sf::Vector2u windowSize, sf::Vector2f levelPixelSize);

    const sf::View& view() const { return m_view; }
    sf::View& view() { return m_view; }

private:
    sf::View m_view;
};

#endif
