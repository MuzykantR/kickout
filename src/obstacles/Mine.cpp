#include "obstacles/Mine.hpp"
#include <cmath>

static constexpr float kBlinkRate = 0.18f;

Mine::Mine(const sf::Texture& tex, sf::Vector2f pos,
           float triggerRadius, float armDelay, float blastRadius,
           std::function<sf::Vector2f()> playerPosGetter,
           std::function<void()>         killPlayer)
    : m_triggerRadius(triggerRadius)
    , m_blastRadius(blastRadius)
    , m_armDelay(armDelay)
    , m_getPlayerPos(std::move(playerPosGetter))
    , m_killPlayer(std::move(killPlayer))
{
    initTexture(tex);
    setPosition(pos.x, pos.y);
}

void Mine::update(float dt, std::vector<std::unique_ptr<Entity>>& /*newEntities*/) {
    if (m_state == State::Done) return;

    const sf::Vector2f center    = getPosition();
    const sf::Vector2f playerPos = m_getPlayerPos();
    const sf::Vector2f diff      = playerPos - center;
    const float        distSq    = diff.x * diff.x + diff.y * diff.y;

    if (m_state == State::Idle) {
        if (distSq <= m_triggerRadius * m_triggerRadius) {
            m_state    = State::Armed;
            m_armTimer = m_armDelay;
        }
        return;
    }

    // Armed: blink и отсчёт
    m_armTimer  -= dt;
    m_blinkTimer = std::fmod(m_blinkTimer + dt, kBlinkRate * 2.f);

    if (m_armTimer <= 0.f) {
        if (distSq <= m_blastRadius * m_blastRadius) {
            m_killPlayer();
        }
        m_state = State::Done;
        destroy();
    }
}

void Mine::draw(sf::RenderWindow& window) {
    if (m_state == State::Armed) {
        // Мигание: прячем спрайт на каждый второй интервал
        if (m_blinkTimer > kBlinkRate) return;
    }
    window.draw(m_sprite);
}
