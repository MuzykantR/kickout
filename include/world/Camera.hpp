#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <SFML/Graphics.hpp>

// Динамическая камера:
//   * Плавный Lerp за целью.
//   * Zoom-in в покое, zoom-out + look-ahead при движении.
//   * Режим обзора всего уровня (overviewHeld == true).
//   * Поддержка тряски (camera shake) — addShake(amplitude) + автозатухание.
//
// Горизонтально камера ограничена пределами уровня; вертикально — нет
// (верх открыт, низ — death-pit, оба должны быть видимы).
class Camera {
public:
    void update(float dt,
                sf::Vector2f targetCenter,
                sf::Vector2f targetVelocity,
                sf::Vector2u windowSize,
                sf::Vector2f levelPixelSize,
                bool overviewHeld);

    void snapTo(sf::Vector2f targetCenter,
                sf::Vector2u windowSize,
                sf::Vector2f levelPixelSize);

    void addShake(float amplitudePixels);

    const sf::View& view() const { return m_view; }
    sf::View&       view()       { return m_view; }

private:
    sf::View     m_view;
    sf::Vector2f m_center{0.f, 0.f};
    sf::Vector2f m_lookahead{0.f, 0.f};
    float        m_zoom        = 1.f;
    float        m_shake       = 0.f;
    bool         m_initialized = false;
};

#endif
