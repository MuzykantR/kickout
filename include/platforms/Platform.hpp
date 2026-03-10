#pragma once
#include <SFML/Graphics.hpp>

using namespace sf;

class Platform {
protected:
    RectangleShape shape; 

public:
    Platform(float startX, float startY, float width, float height);

    virtual ~Platform() = default;

    virtual void draw(RenderWindow& window);

    FloatRect getBounds() const;
    Vector2f getPosition() const;
    Color getColor() const;

    void setColor(int red, int green, int blue, int alpha = 255);
    void setColor(Color newColor);

};