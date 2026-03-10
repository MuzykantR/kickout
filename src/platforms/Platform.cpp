#include "platforms/Platform.hpp"

using namespace sf;

Platform::Platform(float startX, float startY, float width, float height) {
    shape.setSize(Vector2f(width, height));

    shape.setPosition(startX, startY);

    shape.setFillColor(Color(139, 69, 19)); 
    
    shape.setOutlineThickness(2.0f);

    shape.setOutlineColor(Color::Black);
}


void Platform::draw(RenderWindow& window) {
    window.draw(shape);
}

// Для проверки коллизий
FloatRect Platform::getBounds() const {
    return shape.getGlobalBounds();
}

Vector2f Platform::getPosition() const {
    return shape.getPosition();
}

Color Platform::getColor() const {
    return shape.getFillColor();
}

void Platform::setColor(int red, int green, int blue, int alpha) {
    shape.setFillColor(Color(red, green, blue, alpha));
}

void Platform::setColor(Color newColor) {
    shape.setFillColor(newColor);
}