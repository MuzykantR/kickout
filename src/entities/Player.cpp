#include "entities/Player.hpp"
#include <algorithm>
#include <cmath>

namespace {

constexpr float kGravity                 = 2200.f;
constexpr float kGravityJumpHeld         = 1550.f;
constexpr float kMaxRunSpeed             = 280.f;
constexpr float kSprintMult              = 1.55f;
constexpr float kCrouchSpeedMult         = 0.45f;
constexpr float kGroundAccel             = 3200.f;
constexpr float kAirAccel                = 2100.f;
constexpr float kGroundFriction          = 3200.f;
constexpr float kIceFriction             = 850.f;
constexpr float kJumpVel                 = -680.f;
constexpr float kJumpCutMult             = 0.45f;
constexpr float kCoyoteTime              = 0.12f;
constexpr float kWallSlideSpeedMax       = 220.f;
constexpr float kWallSlideSpeedUpMul     = 0.55f;
constexpr float kWallSlideSpeedDownMul   = 1.35f;
constexpr float kWallJumpVelX            = 150.f * 1.3f;
constexpr float kWallJumpVelY            = -620.f;
constexpr float kWallJumpCooldown        = 0.14f;
constexpr float kWallJumpBoostDuration   = 0.24f;
constexpr float kWallJumpHorizPhaseDur   = 0.48f;
constexpr float kWallJumpHorizDrag       = 2.f * kWallJumpVelX * (1.3f - 1.f) / (1.3f * kWallJumpHorizPhaseDur);
constexpr float kWallJumpOpposeAccelMult = 0.32f;
constexpr float kSpringVelY              = -920.f;
constexpr float kMaxFallSpeed            = 980.f;
constexpr float kKillBelowMargin         = 256.f;

} // namespace

// ── Анимации ──────────────────────────────────────────────────────────────────

void Player::setupAnimations(const sf::Texture& sheet, sf::Vector2i frameSize) {
    m_sprite.setTexture(sheet);
    m_hasSheet = true;

    // Хелпер: строит Animation из одного горизонтального ряда (row) спрайт-шита.
    auto makeRow = [&](int row, int count, float fps, bool loop = true) {
        Animation anim;
        anim.frameDuration = 1.f / fps;
        anim.loop          = loop;
        for (int i = 0; i < count; ++i)
            anim.frames.push_back({i * frameSize.x, row * frameSize.y, frameSize.x, frameSize.y});
        return anim;
    };

    // Когда придёт художник, поменять только row/count/fps.
    m_animator.addAnimation("idle",         makeRow(0, 4, 8.f));
    m_animator.addAnimation("walk",         makeRow(1, 6, 12.f));
    m_animator.addAnimation("run",          makeRow(2, 6, 16.f));
    m_animator.addAnimation("jump_rise",    makeRow(3, 2, 8.f,  false));
    m_animator.addAnimation("jump_fall",    makeRow(4, 2, 8.f,  false));
    m_animator.addAnimation("wall_slide",   makeRow(5, 2, 6.f));
    m_animator.addAnimation("crouch_enter", makeRow(6, 3, 12.f, false));
    m_animator.addAnimation("crouch_idle",  makeRow(7, 2, 6.f));
    m_animator.addAnimation("crouch_walk",  makeRow(8, 4, 10.f));
    m_animator.setState("idle");
}

void Player::updateAnimation(const InputState& input) {
    const bool moving = (input.left || input.right) && std::abs(m_velocity.x) > 10.f;
    const bool onWall = !m_onGround && ((m_wallLeft && input.left) || (m_wallRight && input.right));

    if (m_isCrouching) {
        if (moving)
            m_animator.setState("crouch_walk");
        else
            m_animator.setState("crouch_idle");
    } else if (!m_onGround) {
        if (onWall)
            m_animator.setState("wall_slide");
        else if (m_velocity.y < 0.f)
            m_animator.setState("jump_rise");
        else
            m_animator.setState("jump_fall");
    } else {
        if (moving) {
            m_animator.setState(input.sprint ? "run" : "walk");
        } else {
            m_animator.setState("idle");
        }
    }
}

// ── Жизненный цикл ────────────────────────────────────────────────────────────

void Player::spawnAtWorldCenter(sf::Vector2f worldCenter) {
    m_position = worldCenter - m_size * 0.5f;
    m_velocity = {0.f, 0.f};
}

sf::FloatRect Player::getHitbox() const {
    return {m_position.x, m_position.y, m_size.x, m_size.y};
}

sf::Vector2f Player::getCenter() const {
    return {m_position.x + m_size.x * 0.5f, m_position.y + m_size.y * 0.5f};
}

void Player::kill() {
    if (m_dead || m_finishedLevel) return;
    m_dead = true;
    ++m_deathCount;
}

void Player::onLevelLoaded(const Level& level) {
    m_timeAlive = 0.f;
    respawn(level);
}

void Player::resetMotionState() {
    m_velocity                  = {0.f, 0.f};
    m_onGround                  = false;
    m_wasOnGround               = false;
    m_wallLeft                  = false;
    m_wallRight                 = false;
    m_coyoteTimer               = 0.f;
    m_wasJumpHeld               = false;
    m_wallJumpCooldown          = 0.f;
    m_wallJumpBoostTimer        = 0.f;
    m_wallJumpHorizPhaseTimer   = 0.f;
    m_wallJumpBoostDir          = 0.f;
    m_isCrouching               = false;
    m_size.y                    = kStandHeight;
}

void Player::respawn(const Level& level) {
    spawnAtWorldCenter(level.spawnPoint());
    m_dead          = false;
    m_finishedLevel = false;
    resetMotionState();
    syncGroundState(level);
}

void Player::syncGroundState(const Level& level) {
    const sf::FloatRect hb = getHitbox();
    m_onGround    = computeOnGround(level, hb, 0.f);
    m_wasOnGround = m_onGround;
    m_wallLeft    = computeWallLeft(level, hb);
    m_wallRight   = computeWallRight(level, hb);
}

// ── Коллизионные зонды ────────────────────────────────────────────────────────

void Player::separateAxisX(const Level& level, sf::FloatRect& hb, float dirSign) {
    constexpr float eps = 0.25f;
    int guard = 0;
    while (level.overlapsSolid(hb) && guard++ < 1000)
        hb.left -= dirSign * eps;
}

void Player::separateAxisY(const Level& level, sf::FloatRect& hb, float dirSign) {
    constexpr float eps = 0.25f;
    int guard = 0;
    while (level.overlapsSolid(hb) && guard++ < 1000)
        hb.top -= dirSign * eps;
}

bool Player::computeOnGround(const Level& level, const sf::FloatRect& hb, float vy) const {
    if (vy < -40.f) return false;
    sf::FloatRect foot = hb;
    foot.top   += foot.height;
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
    side.left  += hb.width;
    side.width  = 4.f;
    return level.overlapsSolid(side);
}

bool Player::canUncrouch(const Level& level) const {
    sf::FloatRect standing = getHitbox();
    // Хитбокс стоя: сдвигаем верх вверх на (kStandHeight - kCrouchHeight)
    standing.top    -= (kStandHeight - kCrouchHeight);
    standing.height  = kStandHeight;
    return !level.overlapsSolid(standing);
}

// ── Update ────────────────────────────────────────────────────────────────────

void Player::update(float dt, const Level& level, const InputState& input) {
    if (m_dead || m_finishedLevel) return;

    const bool wasOnGroundPrev = m_wasOnGround;
    m_timeAlive += dt;

    // Таймеры wall jump
    m_wallJumpCooldown        = std::max(0.f, m_wallJumpCooldown        - dt);
    m_wallJumpBoostTimer      = std::max(0.f, m_wallJumpBoostTimer      - dt);
    m_wallJumpHorizPhaseTimer = std::max(0.f, m_wallJumpHorizPhaseTimer - dt);

    // ── Приседание ───────────────────────────────────────────────────────────
    if (m_onGround) {
        const bool wantCrouch = input.down;
        if (wantCrouch && !m_isCrouching) {
            m_isCrouching = true;
            const float shrink = kStandHeight - kCrouchHeight;
            m_position.y += shrink;      // нижняя граница остаётся на месте
            m_size.y      = kCrouchHeight;
        } else if (!wantCrouch && m_isCrouching && canUncrouch(level)) {
            m_isCrouching = false;
            m_position.y -= (kStandHeight - kCrouchHeight);
            m_size.y      = kStandHeight;
        }
    } else if (m_isCrouching) {
        // В воздухе нельзя присесть; при приземлении состояние сохранится
        // и снимется только после отпускания DOWN на земле.
    }

    // ── Горизонтальное движение ──────────────────────────────────────────────
    float inputX = 0.f;
    if (input.left)  inputX -= 1.f;
    if (input.right) inputX += 1.f;

    float maxSpeed = kMaxRunSpeed * (input.sprint ? kSprintMult : 1.f);
    if (m_isCrouching) maxSpeed *= kCrouchSpeedMult;

    const sf::FloatRect hbBeforeMove = getHitbox();
    const Tile footTile = level.sampleGroundBelow(hbBeforeMove);

    const float accel       = m_onGround ? kGroundAccel : kAirAccel;
    float       airAccelMult = 1.f;
    if (!m_onGround && m_wallJumpBoostTimer > 0.f && inputX != 0.f && inputX * m_wallJumpBoostDir < 0.f)
        airAccelMult = kWallJumpOpposeAccelMult;

    if (inputX != 0.f) {
        m_velocity.x += inputX * accel * dt * (m_onGround ? 1.f : airAccelMult);
    } else if (m_onGround) {
        const float friction = (footTile == Tile::Ice) ? kIceFriction : kGroundFriction;
        const float sign     = (m_velocity.x > 0.f) ? 1.f : (m_velocity.x < 0.f ? -1.f : 0.f);
        if (sign != 0.f)
            m_velocity.x -= sign * std::min(std::abs(m_velocity.x), friction * dt);
    }

    m_velocity.x = std::clamp(m_velocity.x, -maxSpeed, maxSpeed);

    // ── Гравитация + wall slide ──────────────────────────────────────────────
    float gravity = kGravity;
    if (input.jumpHeld && m_velocity.y < 0.f) {
        const float rise = std::clamp(-m_velocity.y / -kJumpVel, 0.f, 1.f);
        gravity = kGravity + (kGravityJumpHeld - kGravity) * rise;
    }

    const bool onWall = !m_onGround && ((m_wallLeft && input.left) || (m_wallRight && input.right));
    if (onWall && m_velocity.y > 0.f) {
        float cap = kWallSlideSpeedMax;
        if (input.up)   cap *= kWallSlideSpeedUpMul;
        if (input.down) cap *= kWallSlideSpeedDownMul;
        m_velocity.y = std::min(m_velocity.y + gravity * dt, cap);
    } else {
        m_velocity.y += gravity * dt;
    }
    m_velocity.y = std::min(m_velocity.y, kMaxFallSpeed);

    // ── Coyote time ──────────────────────────────────────────────────────────
    if (m_onGround) {
        m_coyoteTimer             = kCoyoteTime;
        m_wallJumpBoostTimer      = 0.f;
        m_wallJumpHorizPhaseTimer = 0.f;
        m_wallJumpBoostDir        = 0.f;
    } else {
        m_coyoteTimer = std::max(0.f, m_coyoteTimer - dt);
    }

    // ── Прыжок / wall jump ───────────────────────────────────────────────────
    if (input.jumpPressed && !m_isCrouching) {
        if (m_onGround || m_coyoteTimer > 0.f) {
            m_velocity.y  = kJumpVel;
            m_coyoteTimer = 0.f;
            m_onGround    = false;
        } else if ((m_wallLeft || m_wallRight) && m_wallJumpCooldown <= 0.f) {
            const float push          = m_wallRight ? -1.f : 1.f;
            m_velocity.x              = push * kWallJumpVelX;
            m_velocity.y              = kWallJumpVelY;
            m_wallJumpCooldown        = kWallJumpCooldown;
            m_wallJumpBoostDir        = push;
            m_wallJumpBoostTimer      = kWallJumpBoostDuration;
            m_wallJumpHorizPhaseTimer = kWallJumpHorizPhaseDur;
        }
    }

    if (!m_onGround && m_wallJumpHorizPhaseTimer > 0.f && m_velocity.x * m_wallJumpBoostDir > 0.f) {
        const float mag    = std::abs(m_velocity.x);
        const float peeled = std::max(0.f, mag - kWallJumpHorizDrag * dt);
        m_velocity.x       = m_wallJumpBoostDir * peeled;
    }

    if (!input.jumpHeld && m_wasJumpHeld && m_velocity.y < 0.f)
        m_velocity.y *= kJumpCutMult;
    m_wasJumpHeld = input.jumpHeld;

    // ── Коллизии по X ────────────────────────────────────────────────────────
    {
        sf::FloatRect hb = getHitbox();
        hb.left += m_velocity.x * dt;
        if (level.overlapsSolid(hb)) {
            separateAxisX(level, hb, (m_velocity.x > 0.f) ? 1.f : -1.f);
            m_velocity.x = 0.f;
        }
        m_position.x = hb.left;
    }

    // ── Коллизии по Y ────────────────────────────────────────────────────────
    {
        sf::FloatRect hb = getHitbox();
        hb.top += m_velocity.y * dt;
        if (level.overlapsSolid(hb)) {
            separateAxisY(level, hb, (m_velocity.y > 0.f) ? 1.f : -1.f);
            m_velocity.y = 0.f;
        }
        m_position.y = hb.top;
    }

    {
        sf::FloatRect hb = getHitbox();
        m_onGround  = computeOnGround(level, hb, m_velocity.y);
        m_wallLeft  = computeWallLeft(level, hb);
        m_wallRight = computeWallRight(level, hb);

        // Spring bounce
        if (m_onGround && !wasOnGroundPrev && level.sampleGroundBelow(hb) == Tile::Spring) {
            m_velocity.y = kSpringVelY;
            m_onGround   = false;
            m_coyoteTimer = 0.f;
        }
    }

    m_wasOnGround = m_onGround;

    // ── Смерть / финиш ───────────────────────────────────────────────────────
    if (m_position.y > level.pixelSize().y + kKillBelowMargin)
        kill();

    const sf::FloatRect hbFinal = getHitbox();
    if (level.overlapsHazard(hbFinal)) kill();
    if (level.overlapsFinish(hbFinal)) m_finishedLevel = true;

    // ── Анимация ─────────────────────────────────────────────────────────────
    updateAnimation(input);
    m_animator.update(dt);
}

// ── Draw ──────────────────────────────────────────────────────────────────────

void Player::draw(sf::RenderTarget& target) const {
    if (m_hasSheet) {
        m_sprite.setTextureRect(m_animator.currentRect());
        const auto bounds = m_sprite.getLocalBounds();
        if (bounds.width > 0.f && bounds.height > 0.f) {
            m_sprite.setScale(m_size.x / bounds.width, m_size.y / bounds.height);
        }
        m_sprite.setPosition(m_position);
        target.draw(m_sprite);
    } else {
        m_debugShape.setSize(m_size);
        m_debugShape.setPosition(m_position);
        m_debugShape.setFillColor(m_isCrouching
            ? sf::Color(200, 180, 40)
            : sf::Color(240, 220, 60));
        m_debugShape.setOutlineColor(sf::Color(40, 35, 20));
        m_debugShape.setOutlineThickness(2.f);
        target.draw(m_debugShape);
    }
}
