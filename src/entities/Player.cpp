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
constexpr float kKillMargin              = 256.f;

} // namespace

// ── Анимации ──────────────────────────────────────────────────────────────────

void Player::setupAnimations(const std::map<std::string, const sf::Texture*>& sheets,
                             sf::Vector2i frameSize) {
    m_stateTextures = sheets;
    m_frameSize     = frameSize;
    m_hasSheet      = !sheets.empty() && frameSize.x > 0 && frameSize.y > 0;
    if (!m_hasSheet) return;

    auto makeStrip = [&](int frameCount, float frameDuration, bool loop) {
        Animation anim;
        anim.frameDuration = frameDuration;
        anim.loop          = loop;
        anim.frames.reserve(static_cast<size_t>(frameCount));
        for (int i = 0; i < frameCount; ++i) {
            anim.frames.emplace_back(i * frameSize.x, 0, frameSize.x, frameSize.y);
        }
        return anim;
    };

    m_animator.addAnimation("idle",   makeStrip(6,  0.12f, true));
    m_animator.addAnimation("walk",   makeStrip(8,  0.08f, true));
    m_animator.addAnimation("run",    makeStrip(10, 0.06f, true));
    m_animator.addAnimation("crouch", makeStrip(4,  0.10f, false));
    m_animator.setState("idle");

    auto idleIt = m_stateTextures.find("idle");
    if (idleIt != m_stateTextures.end() && idleIt->second) {
        m_sprite.setTexture(*idleIt->second, true);
    }
}

void Player::updateAnimation(const InputState& input) {
    const bool moving = (input.left || input.right) && std::abs(m_velocity.x) > 10.f;

    if (m_isCrouching) {
        m_animator.setState("crouch");
    } else if (!m_onGround) {
        // В воздухе показываем walk/run (в зависимости от скорости),
        // а в покое возвращаемся к idle.
        if (std::abs(m_velocity.x) > 150.f)
            m_animator.setState(input.sprint ? "run" : "walk");
        else
            m_animator.setState("idle");
    } else {
        if (moving)
            m_animator.setState(input.sprint ? "run" : "walk");
        else
            m_animator.setState("idle");
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

void Player::zeroFallVelocity() {
    if (m_velocity.y > 0.f) m_velocity.y = 0.f;
}

void Player::forceOnGround() {
    m_onGround    = true;
    m_wasOnGround = true;
    m_coyoteTimer = kCoyoteTime;
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
    m_facingRight               = true;
}

void Player::respawn(const Level& level) {
    spawnAtWorldCenter(level.spawnPoint());
    m_dead          = false;
    m_finishedLevel = false;
    resetMotionState();
    m_animator.setState("idle");
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
    standing.top    -= (kStandHeight - kCrouchHeight);
    standing.height  = kStandHeight;
    return !level.overlapsSolid(standing);
}

void Player::killIfOutOfBounds(const Level& level) {
    // Открытый верх: смерти при подъёме НЕТ.
    // Падение в яму (низ) — смерть.
    if (m_position.y > level.pixelSize().y + kKillMargin) { kill(); return; }
    // Вылет за горизонтальные границы уровня — смерть.
    if (m_position.x + m_size.x < -kKillMargin)                          { kill(); return; }
    if (m_position.x > level.pixelSize().x + kKillMargin)                { kill(); return; }
}

// ── Update ────────────────────────────────────────────────────────────────────

void Player::update(float dt, const Level& level, const InputState& input) {
    if (m_dead || m_finishedLevel) return;

    const bool wasOnGroundPrev = m_wasOnGround;
    m_timeAlive += dt;

    m_wallJumpCooldown        = std::max(0.f, m_wallJumpCooldown        - dt);
    m_wallJumpBoostTimer      = std::max(0.f, m_wallJumpBoostTimer      - dt);
    m_wallJumpHorizPhaseTimer = std::max(0.f, m_wallJumpHorizPhaseTimer - dt);

    // ── Приседание ───────────────────────────────────────────────────────────
    if (m_onGround) {
        const bool wantCrouch = input.down;
        if (wantCrouch && !m_isCrouching) {
            m_isCrouching = true;
            const float shrink = kStandHeight - kCrouchHeight;
            m_position.y += shrink;
            m_size.y      = kCrouchHeight;
        } else if (!wantCrouch && m_isCrouching && canUncrouch(level)) {
            m_isCrouching = false;
            m_position.y -= (kStandHeight - kCrouchHeight);
            m_size.y      = kStandHeight;
        }
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

    // Обновляем направление взгляда
    if (m_velocity.x >  10.f) m_facingRight = true;
    else if (m_velocity.x < -10.f) m_facingRight = false;

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

        if (m_onGround && !wasOnGroundPrev && level.sampleGroundBelow(hb) == Tile::Spring) {
            m_velocity.y = kSpringVelY;
            m_onGround   = false;
            m_coyoteTimer = 0.f;
        }
    }

    m_wasOnGround = m_onGround;

    // ── Смерть / финиш ───────────────────────────────────────────────────────
    killIfOutOfBounds(level);

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
        const std::string& state = m_animator.state();
        const sf::Texture* tex   = nullptr;
        auto it = m_stateTextures.find(state);
        if (it != m_stateTextures.end() && it->second) {
            tex = it->second;
        } else {
            auto fallback = m_stateTextures.find("idle");
            if (fallback != m_stateTextures.end()) tex = fallback->second;
        }

        if (tex) {
            m_sprite.setTexture(*tex, false);
            m_sprite.setTextureRect(m_animator.currentRect());

            const float frameW = std::max(1, m_frameSize.x);
            const float frameH = std::max(1, m_frameSize.y);
            float sx = m_size.x / frameW;
            float sy = m_size.y / frameH;

            // Горизонтальный flip: масштабируем по X в минус, но компенсируем
            // позицию, чтобы спрайт визуально остался в границах хитбокса.
            sf::Vector2f drawPos = m_position;
            if (!m_facingRight) {
                sx = -sx;
                drawPos.x += m_size.x;
            }
            m_sprite.setScale(sx, sy);
            m_sprite.setPosition(drawPos);
            target.draw(m_sprite);
            return;
        }
    }

    // Безопасный fallback — цветной прямоугольник фиксированного размера.
    m_debugShape.setSize(m_size);
    m_debugShape.setPosition(m_position);
    m_debugShape.setFillColor(m_isCrouching
        ? sf::Color(200, 180, 40)
        : sf::Color(240, 220, 60));
    m_debugShape.setOutlineColor(sf::Color(40, 35, 20));
    m_debugShape.setOutlineThickness(2.f);
    target.draw(m_debugShape);
}
