#include "obstacles/FerrisWheel.hpp"
#include "core/EntityFactory.hpp"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kDegToRad = 3.14159265358979323846f / 180.f;
}

FerrisWheel::FerrisWheel(const sf::Texture* hubTex,
                         const sf::Texture* cabinTex,
                         sf::Vector2f center,
                         float radius,
                         int cabinCount,
                         float angularSpeedDeg,
                         sf::Vector2f cabinSize)
    : m_center(center),
      m_radius(std::max(20.f, radius)),
      m_cabinCount(std::max(2, cabinCount)),
      m_angularSpeed(angularSpeedDeg),
      m_cabinSize(cabinSize) {
    // Hub (центр)
    if (hubTex) {
        m_hubSprite.setTexture(*hubTex, true);
        const sf::Vector2u sz = hubTex->getSize();
        if (sz.x > 0u && sz.y > 0u) {
            m_hubSprite.setOrigin(static_cast<float>(sz.x) * 0.5f,
                                  static_cast<float>(sz.y) * 0.5f);
            m_hubSprite.setScale(ek::kSizeFerrisHub.x / static_cast<float>(sz.x),
                                 ek::kSizeFerrisHub.y / static_cast<float>(sz.y));
        }
        m_hasHub = true;
    } else {
        const float r = ek::kSizeFerrisHub.x * 0.5f;
        m_hubFallback.setRadius(r);
        m_hubFallback.setOrigin(r, r);
        m_hubFallback.setFillColor(ek::kColorFerrisHub);
        m_hubFallback.setOutlineColor(sf::Color(60, 40, 10));
        m_hubFallback.setOutlineThickness(2.f);
    }

    // Cabin — один спрайт переиспользуется для всех кабин (только меняем позицию).
    if (cabinTex) {
        m_cabinSprite.setTexture(*cabinTex, true);
        const sf::Vector2u sz = cabinTex->getSize();
        if (sz.x > 0u && sz.y > 0u) {
            m_cabinSprite.setScale(m_cabinSize.x / static_cast<float>(sz.x),
                                   m_cabinSize.y / static_cast<float>(sz.y));
        }
        m_hasCabinTex = true;
    } else {
        m_cabinFallback.setSize(m_cabinSize);
        m_cabinFallback.setFillColor(ek::kColorFerrisCabin);
        m_cabinFallback.setOutlineColor(sf::Color(40, 25, 10));
        m_cabinFallback.setOutlineThickness(1.f);
    }
}

sf::Vector2f FerrisWheel::cabinCenter(int i) const {
    const float a = (m_phaseDeg + i * 360.f / static_cast<float>(m_cabinCount)) * kDegToRad;
    return {m_center.x + std::cos(a) * m_radius,
            m_center.y + std::sin(a) * m_radius};
}

sf::FloatRect FerrisWheel::cabinBounds(int i) const {
    const sf::Vector2f c = cabinCenter(i);
    return {c.x - m_cabinSize.x * 0.5f,
            c.y - m_cabinSize.y * 0.5f,
            m_cabinSize.x, m_cabinSize.y};
}

sf::Vector2f FerrisWheel::cabinLinearVelocity(int i) const {
    const float omega = m_angularSpeed * kDegToRad;            // рад/сек
    const float a     = (m_phaseDeg + i * 360.f / static_cast<float>(m_cabinCount)) * kDegToRad;
    // v = ω × r, в 2D: (-sin·R·ω, cos·R·ω)
    return {-std::sin(a) * m_radius * omega,
             std::cos(a) * m_radius * omega};
}

void FerrisWheel::update(float dt, std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {
    m_phaseDeg += m_angularSpeed * dt;
    if (m_phaseDeg > 360.f) m_phaseDeg -= 360.f;
    if (m_phaseDeg < -360.f) m_phaseDeg += 360.f;
}

void FerrisWheel::draw(sf::RenderWindow& window) {
    // Спицы (линиями)
    sf::Color spokeColor(80, 60, 30);
    for (int i = 0; i < m_cabinCount; ++i) {
        const sf::Vector2f c = cabinCenter(i);
        sf::Vertex line[] = {
            sf::Vertex(m_center, spokeColor),
            sf::Vertex(c,        spokeColor)
        };
        window.draw(line, 2, sf::Lines);
    }

    // Кабины
    for (int i = 0; i < m_cabinCount; ++i) {
        const sf::Vector2f c = cabinCenter(i);
        if (m_hasCabinTex) {
            m_cabinSprite.setPosition(c.x - m_cabinSize.x * 0.5f,
                                      c.y - m_cabinSize.y * 0.5f);
            window.draw(m_cabinSprite);
        } else {
            m_cabinFallback.setPosition(c.x - m_cabinSize.x * 0.5f,
                                        c.y - m_cabinSize.y * 0.5f);
            window.draw(m_cabinFallback);
        }
    }

    // Hub
    if (m_hasHub) {
        m_hubSprite.setPosition(m_center);
        m_hubSprite.setRotation(m_phaseDeg);
        window.draw(m_hubSprite);
    } else {
        m_hubFallback.setPosition(m_center);
        window.draw(m_hubFallback);
    }
}

sf::FloatRect FerrisWheel::getBounds() const {
    const float r = m_radius + std::max(m_cabinSize.x, m_cabinSize.y) * 0.5f;
    return {m_center.x - r, m_center.y - r, r * 2.f, r * 2.f};
}
