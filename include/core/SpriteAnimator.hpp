#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct Animation {
    std::vector<sf::IntRect> frames;
    float frameDuration = 0.1f;
    bool  loop          = true;
};

// Компонент анимации — не зависит от Entity.
// Владелец: вызывает update(dt), затем читает currentRect() и ставит в спрайт.
class SpriteAnimator {
public:
    void addAnimation(const std::string& name, Animation anim);

    // Сменить состояние (сбрасывает таймер и кадр, если имя отличается).
    void setState(const std::string& name);

    void update(float dt);

    sf::IntRect currentRect() const;
    const std::string& state() const { return m_current; }

private:
    std::unordered_map<std::string, Animation> m_animations;
    std::string m_current;
    int         m_frame   = 0;
    float       m_elapsed = 0.f;
};
