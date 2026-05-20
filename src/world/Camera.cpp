#include "world/Camera.hpp"
#include <algorithm>

static float clampCameraCenter(float center, float halfWindow, float levelExtent) {
    const float maxBeforeClamp = levelExtent - halfWindow;
    if (maxBeforeClamp <= halfWindow) {
        return levelExtent * 0.5f;
    }
    return std::clamp(center, halfWindow, maxBeforeClamp);
}

void Camera::follow(sf::Vector2f targetCenter, sf::Vector2u windowSize, sf::Vector2f levelPixelSize) {
    const float halfW = static_cast<float>(windowSize.x) * 0.5f;
    const float halfH = static_cast<float>(windowSize.y) * 0.5f;

    const float cx = clampCameraCenter(targetCenter.x, halfW, levelPixelSize.x);
    const float cy = clampCameraCenter(targetCenter.y, halfH, levelPixelSize.y);

    m_view.setSize(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    m_view.setCenter(cx, cy);
}
