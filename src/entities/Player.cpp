#include "entities/Player.hpp"
#include <algorithm>
#include <cmath>

namespace {

constexpr float kGravity = 2200.f;
constexpr float kGravityJumpHeld = 1550.f;
constexpr float kMaxRunSpeed = 280.f;
constexpr float kSprintMult = 1.55f;
constexpr float kGroundAccel = 3200.f;
constexpr float kAirAccel = 2100.f;
constexpr float kGroundFriction = 3200.f;
constexpr float kIceFriction = 850.f;
constexpr float kJumpVel = -680.f;
constexpr float kJumpCutMult = 0.45f;
constexpr float kCoyoteTime = 0.12f;
constexpr float kWallSlideSpeedMax = 220.f;
constexpr float kWallSlideSpeedUpMul = 0.55f;
constexpr float kWallSlideSpeedDownMul = 1.35f;
constexpr float kWallJumpHorizDistance = 150.f;
constexpr float kWallJumpHorizSpeedMult = 1.3f;
constexpr float kWallJumpVelX = kWallJumpHorizDistance * kWallJumpHorizSpeedMult;
constexpr float kWallJumpVelY = -620.f;
constexpr float kWallJumpCooldown = 0.14f;
constexpr float kWallJumpBoostDuration = 0.24f;
constexpr float kWallJumpHorizPhaseDuration = 0.48f;
// За фазу T: v0*T - 0.5*a*T² ≈ v_ref*T при v0 = v_ref * mult
constexpr float kWallJumpHorizDrag =
    2.f * kWallJumpVelX * (kWallJumpHorizSpeedMult - 1.f) / (kWallJumpHorizSpeedMult * kWallJumpHorizPhaseDuration);
constexpr float kWallJumpOpposeAccelMult = 0.32f;
constexpr float kSpringVelY = -920.f;
constexpr float kMaxFallSpeed = 980.f;
constexpr float kKillBelowMargin = 256.f;

} // namespace

void Player::spawnAtWorldCenter(sf::Vector2f worldCenter) {
    m_position = worldCenter - m_size * 0.5f;
    m_velocity = {0.f, 0.f};
}

sf::FloatRect Player::getHitbox() const { return {m_position.x, m_position.y, m_size.x, m_size.y}; }

sf::Vector2f Player::getCenter() const {
    return {m_position.x + m_size.x * 0.5f, m_position.y + m_size.y * 0.5f};
}

void Player::kill() {
    if (m_dead || m_finishedLevel) {
        return;
    }
    m_dead = true;
    ++m_deathCount;
}

void Player::onLevelLoaded(const Level& level) {
    m_timeAlive = 0.f;
    respawn(level);
}

void Player::resetMotionState() {
    m_velocity = {0.f, 0.f};
    m_onGround = false;
    m_wasOnGround = false;
    m_wallLeft = false;
    m_wallRight = false;
    m_coyoteTimer = 0.f;
    m_wasJumpHeld = false;
    m_wallJumpCooldown = 0.f;
    m_wallJumpBoostTimer = 0.f;
    m_wallJumpHorizPhaseTimer = 0.f;
    m_wallJumpBoostDir = 0.f;
}

void Player::respawn(const Level& level) {
    spawnAtWorldCenter(level.spawnPoint());
    m_dead = false;
    m_finishedLevel = false;
    resetMotionState();
    syncGroundState(level);
}

void Player::syncGroundState(const Level& level) {
    const sf::FloatRect hb = getHitbox();
    m_onGround = computeOnGround(level, hb, 0.f);
    m_wasOnGround = m_onGround;
    m_wallLeft = computeWallLeft(level, hb);
    m_wallRight = computeWallRight(level, hb);
}

void Player::separateAxisX(const Level& level, sf::FloatRect& hb, float dirSign) {
    constexpr float eps = 0.25f;
    int guard = 0;
    while (level.overlapsSolid(hb) && guard++ < 1000) {
        hb.left -= dirSign * eps;
    }
}

void Player::separateAxisY(const Level& level, sf::FloatRect& hb, float dirSign) {
    constexpr float eps = 0.25f;
    int guard = 0;
    while (level.overlapsSolid(hb) && guard++ < 1000) {
        hb.top -= dirSign * eps;
    }
}

bool Player::computeOnGround(const Level& level, const sf::FloatRect& hb, float vy) const {
    if (vy < -40.f) {
        return false;
    }
    sf::FloatRect foot = hb;
    foot.top += foot.height;
    foot.height = 4.f;
    return level.overlapsSolid(foot);
}

bool Player::computeWallLeft(const Level& level, const sf::FloatRect& hb) const {
    sf::FloatRect side = hb;
    side.left -= 3.f;
    side.width = 4.f;
    return level.overlapsSolid(side);
}

bool Player::computeWallRight(const Level& level, const sf::FloatRect& hb) const {
    sf::FloatRect side = hb;
    side.left += hb.width;
    side.width = 4.f;
    return level.overlapsSolid(side);
}

void Player::update(float dt, const Level& level, const InputState& input) {
    if (m_dead) {
        return;
    }
    if (m_finishedLevel) {
        return;
    }

    const bool wasOnGroundPrev = m_wasOnGround;

    m_timeAlive += dt;

    if (m_wallJumpCooldown > 0.f) {
        m_wallJumpCooldown = std::max(0.f, m_wallJumpCooldown - dt);
    }
    if (m_wallJumpBoostTimer > 0.f) {
        m_wallJumpBoostTimer = std::max(0.f, m_wallJumpBoostTimer - dt);
    }
    if (m_wallJumpHorizPhaseTimer > 0.f) {
        m_wallJumpHorizPhaseTimer = std::max(0.f, m_wallJumpHorizPhaseTimer - dt);
    }

    const float maxSpeed = kMaxRunSpeed * (input.sprint ? kSprintMult : 1.f);

    float inputX = 0.f;
    if (input.left) {
        inputX -= 1.f;
    }
    if (input.right) {
        inputX += 1.f;
    }
    inputX = std::clamp(inputX, -1.f, 1.f);

    const sf::FloatRect hbBeforeMove = getHitbox();
    const Tile footTile = level.sampleGroundBelow(hbBeforeMove);

    const float accel = m_onGround ? kGroundAccel : kAirAccel;
    float airAccelMult = 1.f;
    if (!m_onGround && m_wallJumpBoostTimer > 0.f && inputX != 0.f &&
        inputX * m_wallJumpBoostDir < 0.f) {
        airAccelMult = kWallJumpOpposeAccelMult;
    }
    if (inputX != 0.f) {
        m_velocity.x += inputX * accel * dt * (m_onGround ? 1.f : airAccelMult);
    } else if (m_onGround) {
        const float friction =
            (footTile == Tile::Ice) ? kIceFriction : kGroundFriction;
        const float sign = (m_velocity.x > 0.f) ? 1.f : (m_velocity.x < 0.f ? -1.f : 0.f);
        if (sign != 0.f) {
            const float mag = std::min(std::abs(m_velocity.x), friction * dt);
            m_velocity.x -= sign * mag;
        }
    }

    m_velocity.x = std::clamp(m_velocity.x, -maxSpeed, maxSpeed);

    // Гравитация + wall slide
    // Удержание прыжка: слабее гравитация только на активном подъёме, к вершине — полная (без «левитации» на vy≈0).
    float gravity = kGravity;
    if (input.jumpHeld && m_velocity.y < 0.f) {
        const float rise = std::clamp(-m_velocity.y / -kJumpVel, 0.f, 1.f);
        gravity = kGravity + (kGravityJumpHeld - kGravity) * rise;
    }

    if (!m_onGround && ((m_wallLeft && input.left) || (m_wallRight && input.right)) &&
        m_velocity.y > 0.f) {
        float cap = kWallSlideSpeedMax;
        if (input.up) {
            cap *= kWallSlideSpeedUpMul;
        }
        if (input.down) {
            cap *= kWallSlideSpeedDownMul;
        }
        m_velocity.y = std::min(m_velocity.y + gravity * dt, cap);
    } else {
        m_velocity.y += gravity * dt;
    }
    m_velocity.y = std::min(m_velocity.y, kMaxFallSpeed);

    // Coyote time
    if (m_onGround) {
        m_coyoteTimer = kCoyoteTime;
        m_wallJumpBoostTimer = 0.f;
        m_wallJumpHorizPhaseTimer = 0.f;
        m_wallJumpBoostDir = 0.f;
    } else {
        m_coyoteTimer = std::max(0.f, m_coyoteTimer - dt);
    }

    // Прыжок, wall jump
    if (input.jumpPressed) {
        if (m_onGround || m_coyoteTimer > 0.f) {
            m_velocity.y = kJumpVel;
            m_coyoteTimer = 0.f;
            m_onGround = false;
        } else if ((m_wallLeft || m_wallRight) && m_wallJumpCooldown <= 0.f) {
            const float push = m_wallRight ? -1.f : 1.f;
            m_velocity.x = push * kWallJumpVelX;
            m_velocity.y = kWallJumpVelY;
            m_wallJumpCooldown = kWallJumpCooldown;
            m_wallJumpBoostDir = push;
            m_wallJumpBoostTimer = kWallJumpBoostDuration;
            m_wallJumpHorizPhaseTimer = kWallJumpHorizPhaseDuration;
        }
    }

    if (!m_onGround && m_wallJumpHorizPhaseTimer > 0.f && m_velocity.x * m_wallJumpBoostDir > 0.f) {
        const float mag = std::abs(m_velocity.x);
        const float peeled = std::max(0.f, mag - kWallJumpHorizDrag * dt);
        m_velocity.x = m_wallJumpBoostDir * peeled;
    }

    if (!input.jumpHeld && m_wasJumpHeld && m_velocity.y < 0.f) {
        m_velocity.y *= kJumpCutMult;
    }
    m_wasJumpHeld = input.jumpHeld;

    // Коллизии по X
    sf::FloatRect hb = getHitbox();
    hb.left += m_velocity.x * dt;
    if (level.overlapsSolid(hb)) {
        const float dir = (m_velocity.x > 0.f) ? 1.f : -1.f;
        separateAxisX(level, hb, dir);
        m_velocity.x = 0.f;
    }
    m_position.x = hb.left;

    // Коллизии по Y
    hb = getHitbox();
    hb.top += m_velocity.y * dt;
    if (level.overlapsSolid(hb)) {
        // Падение (vy>0): нужно проталкивать вверх (уменьшать top). Прыжок в потолок (vy<0): вниз (увеличивать top).
        const float dirSign = (m_velocity.y > 0.f) ? 1.f : -1.f;
        separateAxisY(level, hb, dirSign);
        m_velocity.y = 0.f;
    }
    m_position.y = hb.top;

    hb = getHitbox();
    m_onGround = computeOnGround(level, hb, m_velocity.y);
    m_wallLeft = computeWallLeft(level, hb);
    m_wallRight = computeWallRight(level, hb);

    if (m_onGround && !wasOnGroundPrev) {
        const Tile under = level.sampleGroundBelow(hb);
        if (under == Tile::Spring) {
            m_velocity.y = kSpringVelY;
            m_onGround = false;
            m_coyoteTimer = 0.f;
        }
    }

    m_wasOnGround = m_onGround;

    const float killY = level.pixelSize().y + kKillBelowMargin;
    if (m_position.y > killY) {
        kill();
    }

    if (level.overlapsHazard(hb)) {
        kill();
    }

    if (level.overlapsFinish(hb)) {
        m_finishedLevel = true;
    }
}

void Player::draw(sf::RenderTarget& target) const {
    m_debugShape.setSize(m_size);
    m_debugShape.setPosition(m_position);
    m_debugShape.setFillColor(sf::Color(240, 220, 60));
    m_debugShape.setOutlineColor(sf::Color(40, 35, 20));
    m_debugShape.setOutlineThickness(2.f);
    target.draw(m_debugShape);
}
