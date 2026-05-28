#include "core/SpriteAnimator.hpp"

void SpriteAnimator::addAnimation(const std::string& name, Animation anim) {
    m_animations[name] = std::move(anim);
}

void SpriteAnimator::setState(const std::string& name) {
    if (m_current == name) return;
    m_current = name;
    m_frame   = 0;
    m_elapsed = 0.f;
}

void SpriteAnimator::update(float dt) {
    auto it = m_animations.find(m_current);
    if (it == m_animations.end() || it->second.frames.empty()) return;

    const Animation& anim = it->second;
    m_elapsed += dt;
    while (m_elapsed >= anim.frameDuration) {
        m_elapsed -= anim.frameDuration;
        ++m_frame;
        if (m_frame >= static_cast<int>(anim.frames.size())) {
            m_frame = anim.loop ? 0 : static_cast<int>(anim.frames.size()) - 1;
        }
    }
}

sf::IntRect SpriteAnimator::currentRect() const {
    auto it = m_animations.find(m_current);
    if (it == m_animations.end() || it->second.frames.empty())
        return {};
    return it->second.frames[static_cast<size_t>(m_frame)];
}
