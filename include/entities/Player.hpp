#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <SFML/Graphics.hpp>
#include "world/Level.hpp"
#include "core/InputState.hpp"

class Player {
public:
    void spawnAtWorldCenter(sf::Vector2f worldCenter);
    void respawn(const Level& level);

    void update(float dt, const Level& level, const InputState& input);

    void draw(sf::RenderTarget& target) const;

    sf::FloatRect getHitbox() const;
    sf::Vector2f getCenter() const;

    bool isDead() const { return m_dead; }
    void kill();

    bool reachedFinish() const { return m_finishedLevel; }
    void clearFinish() { m_finishedLevel = false; }

    int deathCount() const { return m_deathCount; }
    float timeAlive() const { return m_timeAlive; }

    void onLevelLoaded(const Level& level);

private:
    static void separateAxisX(const Level& level, sf::FloatRect& hb, float dirSign);
    static void separateAxisY(const Level& level, sf::FloatRect& hb, float dirSign);

    bool computeOnGround(const Level& level, const sf::FloatRect& hb, float vy) const;
    bool computeWallLeft(const Level& level, const sf::FloatRect& hb) const;
    bool computeWallRight(const Level& level, const sf::FloatRect& hb) const;

    void resetMotionState();
    void syncGroundState(const Level& level);

    sf::Vector2f m_position{0.f, 0.f};
    sf::Vector2f m_velocity{0.f, 0.f};
    sf::Vector2f m_size{26.f, 52.f};

    bool m_onGround = false;
    bool m_wasOnGround = false;
    bool m_wallLeft = false;
    bool m_wallRight = false;

    float m_coyoteTimer = 0.f;
    bool m_wasJumpHeld = false;

    float m_wallJumpCooldown = 0.f;
    float m_wallJumpBoostTimer = 0.f;
    float m_wallJumpHorizPhaseTimer = 0.f;
    float m_wallJumpBoostDir = 0.f;

    bool m_dead = false;
    bool m_finishedLevel = false;
    int m_deathCount = 0;
    float m_timeAlive = 0.f;

    mutable sf::RectangleShape m_debugShape;
};

#endif
