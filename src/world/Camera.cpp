#include "world/Camera.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {

// Параметры зума и панорамирования
constexpr float kZoomIdle             = 0.85f;   // < 1 — ближе к герою (зум-ин)
constexpr float kZoomWalk             = 1.00f;   // нейтральная позиция
constexpr float kZoomRun              = 1.20f;   // > 1 — шире обзор (зум-аут)
constexpr float kSpeedWalkThreshold   = 60.f;
constexpr float kSpeedRunThreshold    = 360.f;
constexpr float kZoomLerpRate         = 4.f;     // 1/сек: скорость экспон. сглаживания
constexpr float kCenterLerpRate       = 7.f;     // 1/сек
constexpr float kLookaheadLerpRate    = 3.f;     // 1/сек

constexpr float kLookaheadMaxX        = 220.f;
constexpr float kLookaheadMaxY        = 140.f;
constexpr float kLookaheadVxRef       = 280.f;   // выше — лук-ахед уходит в максимум
constexpr float kLookaheadVyRef       = 600.f;

constexpr float kOverviewZoomMul      = 1.08f;   // запас по краям при обзоре
constexpr float kOverviewLerpRate     = 5.f;

constexpr float kShakeDecay           = 6.f;     // 1/сек

float expSmooth(float dt, float ratePerSec) {
    return 1.f - std::exp(-ratePerSec * dt);
}

sf::Vector2f lerp(sf::Vector2f a, sf::Vector2f b, float t) {
    return a + (b - a) * t;
}

float lerp1(float a, float b, float t) { return a + (b - a) * t; }

float clampCenter(float c, float halfView, float levelExtent) {
    if (levelExtent <= 2.f * halfView) return levelExtent * 0.5f;
    return std::clamp(c, halfView, levelExtent - halfView);
}

float randUnit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.f - 1.f;
}

} // namespace

void Camera::snapTo(sf::Vector2f targetCenter,
                    sf::Vector2u windowSize,
                    sf::Vector2f levelPixelSize) {
    m_center      = targetCenter;
    m_lookahead   = {0.f, 0.f};
    m_zoom        = 1.f;
    m_shake       = 0.f;
    m_initialized = true;

    m_view.setSize(static_cast<float>(windowSize.x),
                   static_cast<float>(windowSize.y));
    const float halfW = static_cast<float>(windowSize.x) * 0.5f;
    m_view.setCenter(clampCenter(m_center.x, halfW, levelPixelSize.x), m_center.y);
}

void Camera::addShake(float amplitudePixels) {
    m_shake = std::max(m_shake, amplitudePixels);
}

void Camera::update(float dt,
                    sf::Vector2f targetCenter,
                    sf::Vector2f targetVelocity,
                    sf::Vector2u windowSize,
                    sf::Vector2f levelPixelSize,
                    bool overviewHeld) {
    if (!m_initialized) {
        snapTo(targetCenter, windowSize, levelPixelSize);
    }

    const float winW = static_cast<float>(windowSize.x);
    const float winH = static_cast<float>(windowSize.y);

    // ── 1. Целевой зум ───────────────────────────────────────────────────────
    float targetZoom;
    if (overviewHeld) {
        const float fitX = levelPixelSize.x / winW;
        const float fitY = levelPixelSize.y / winH;
        targetZoom = std::max(1.f, std::max(fitX, fitY)) * kOverviewZoomMul;
    } else {
        const float speed = std::sqrt(targetVelocity.x * targetVelocity.x +
                                      targetVelocity.y * targetVelocity.y);
        if (speed <= kSpeedWalkThreshold) {
            targetZoom = kZoomIdle;
        } else if (speed >= kSpeedRunThreshold) {
            targetZoom = kZoomRun;
        } else if (speed < kSpeedRunThreshold * 0.5f) {
            const float t = (speed - kSpeedWalkThreshold) /
                            (kSpeedRunThreshold * 0.5f - kSpeedWalkThreshold);
            targetZoom = lerp1(kZoomIdle, kZoomWalk, std::clamp(t, 0.f, 1.f));
        } else {
            const float t = (speed - kSpeedRunThreshold * 0.5f) /
                            (kSpeedRunThreshold * 0.5f);
            targetZoom = lerp1(kZoomWalk, kZoomRun, std::clamp(t, 0.f, 1.f));
        }
    }

    const float zoomBlend = expSmooth(dt, overviewHeld ? kOverviewLerpRate : kZoomLerpRate);
    m_zoom = lerp1(m_zoom, targetZoom, zoomBlend);

    // ── 2. Look-ahead (только в обычном режиме) ──────────────────────────────
    sf::Vector2f lookTarget{0.f, 0.f};
    if (!overviewHeld) {
        const float laX = std::clamp(targetVelocity.x / kLookaheadVxRef, -1.f, 1.f) * kLookaheadMaxX;
        const float laY = (targetVelocity.y > 0.f)
            ? std::clamp(targetVelocity.y / kLookaheadVyRef, 0.f, 1.f) * kLookaheadMaxY
            : 0.f;
        lookTarget = {laX, laY};
    }
    m_lookahead = lerp(m_lookahead, lookTarget, expSmooth(dt, kLookaheadLerpRate));

    // ── 3. Целевой центр ────────────────────────────────────────────────────
    sf::Vector2f finalTarget = overviewHeld
        ? sf::Vector2f{levelPixelSize.x * 0.5f, levelPixelSize.y * 0.5f}
        : targetCenter + m_lookahead;

    m_center = lerp(m_center, finalTarget,
                    expSmooth(dt, overviewHeld ? kOverviewLerpRate : kCenterLerpRate));

    // ── 4. Применяем view ───────────────────────────────────────────────────
    const float viewW = winW * m_zoom;
    const float viewH = winH * m_zoom;
    m_view.setSize(viewW, viewH);

    // Только горизонтальный clamp; верх и низ — открытые.
    const float halfW = viewW * 0.5f;
    float cx = m_center.x;
    if (!overviewHeld) cx = clampCenter(cx, halfW, levelPixelSize.x);

    // ── 5. Camera shake ─────────────────────────────────────────────────────
    sf::Vector2f shakeOffset{0.f, 0.f};
    if (m_shake > 0.01f) {
        shakeOffset = {randUnit() * m_shake, randUnit() * m_shake};
        m_shake = std::max(0.f, m_shake - kShakeDecay * dt * m_shake);
    } else {
        m_shake = 0.f;
    }

    m_view.setCenter(cx + shakeOffset.x, m_center.y + shakeOffset.y);
}
