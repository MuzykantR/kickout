#pragma once

#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include "core/SpriteAnimator.hpp"
#include "core/InputState.hpp"
#include "world/Level.hpp"

class Player {
public:
    void spawnAtWorldCenter(sf::Vector2f worldCenter);
    void respawn(const Level& level);

    void update(float dt, const Level& level, const InputState& input);
    void draw(sf::RenderTarget& target) const;

    // Sprite-sheet режим — Option B: один PNG на состояние.
    // Ключи (могут отсутствовать; если есть хотя бы "idle" — включается спрайтовый режим):
    //   "idle"   (6 frames)
    //   "walk"   (8 frames)
    //   "run"    (10 frames)
    //   "crouch" (4 frames, не зацикленная)
    void setupAnimations(const std::map<std::string, const sf::Texture*>& sheets,
                         sf::Vector2i frameSize);

    sf::FloatRect getHitbox() const;
    sf::Vector2f  getCenter() const;
    sf::Vector2f  velocity() const { return m_velocity; }
    void addExternalVelocity(const sf::Vector2f& dv) { m_velocity += dv; }

    bool isDead()          const { return m_dead; }
    void kill();

    void applyPlatformCarry(sf::Vector2f delta)           { m_position += delta; }
    void applyExternalDisplacement(const sf::Vector2f& d) { m_position += d; }

    bool reachedFinish()   const { return m_finishedLevel; }
    void clearFinish()           { m_finishedLevel = false; }

    int   deathCount() const { return m_deathCount; }
    float timeAlive()  const { return m_timeAlive; }

    void onLevelLoaded(const Level& level);

    void zeroFallVelocity();
    void forceOnGround();

private:
    static void separateAxisX(const Level& level, sf::FloatRect& hb, float dirSign);
    static void separateAxisY(const Level& level, sf::FloatRect& hb, float dirSign);

    bool computeOnGround(const Level& level, const sf::FloatRect& hb, float vy) const;
    bool computeWallLeft (const Level& level, const sf::FloatRect& hb) const;
    bool computeWallRight(const Level& level, const sf::FloatRect& hb) const;

    bool canUncrouch(const Level& level) const;

    void resetMotionState();
    void syncGroundState(const Level& level);
    void updateAnimation(const InputState& input);
    void killIfOutOfBounds(const Level& level);

    // ── физика ──
    sf::Vector2f m_position{0.f, 0.f};
    sf::Vector2f m_velocity{0.f, 0.f};

    static constexpr float kStandHeight  = 52.f;
    static constexpr float kCrouchHeight = 26.f;
    static constexpr float kWidth        = 26.f;

    sf::Vector2f m_size{kWidth, kStandHeight};

    bool  m_onGround     = false;
    bool  m_wasOnGround  = false;
    bool  m_wallLeft     = false;
    bool  m_wallRight    = false;

    float m_coyoteTimer              = 0.f;
    bool  m_wasJumpHeld              = false;
    float m_wallJumpCooldown         = 0.f;
    float m_wallJumpBoostTimer       = 0.f;
    float m_wallJumpHorizPhaseTimer  = 0.f;
    float m_wallJumpBoostDir         = 0.f;

    bool m_isCrouching = false;

    // ── состояние ──
    bool  m_dead          = false;
    bool  m_finishedLevel = false;
    int   m_deathCount    = 0;
    float m_timeAlive     = 0.f;

    bool  m_facingRight   = true;

    // ── визуал ──
    SpriteAnimator             m_animator;
    bool                       m_hasSheet    = false;
    sf::Vector2i               m_frameSize{0, 0};
    std::map<std::string, const sf::Texture*> m_stateTextures;
    mutable sf::Sprite         m_sprite;
    mutable sf::RectangleShape m_debugShape;
};
