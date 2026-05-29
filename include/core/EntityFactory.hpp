#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>

// Единая таблица фиксированных габаритов и цветов-заглушек для всех
// игровых объектов кроме игрока. Идея ТЗ (Модуль 4 + Модуль 5):
//
//   * один и тот же тип объекта на всех 10 уровнях имеет ровно один
//     bounding box (см. EntitySpec::size);
//   * если PNG-текстура из манифеста не загрузилась, объект рисуется
//     цветным прямоугольником/спрайтом строго этого же размера, чтобы
//     уровни были проходимы даже без художественных ассетов.
//
// Параметры поведения (скорость снарядов, угол, период огня и т.п.)
// в этой таблице НЕ хранятся — они уровневые (см. PlacedEntity).
struct EntitySpec {
    sf::Vector2f size;            // фиксированный bounding box
    sf::Color    fallbackColor;   // цвет векторного прямоугольника-заглушки
    const char*  textureKey;      // ключ в Game::m_textures (может отсутствовать)
};

namespace ek {  // entity-kinds — компактный namespace

// ── Размеры (в пикселях) — едины для всех уровней ─────────────────────────
inline const sf::Vector2f kSizeCrossbow      {48.f, 48.f};
inline const sf::Vector2f kSizeArrow         {32.f,  8.f};
inline const sf::Vector2f kSizeCannon        {64.f, 64.f};
inline const sf::Vector2f kSizeCannonball    {24.f, 24.f};
inline const sf::Vector2f kSizeTurret        {48.f, 48.f};
inline const sf::Vector2f kSizeBullet        {12.f, 12.f};
inline const sf::Vector2f kSizeMine          {32.f, 32.f};
inline const sf::Vector2f kSizeRotatingBlade {72.f, 72.f};
inline const sf::Vector2f kSizeBarbedWire    {96.f, 24.f};   // 2 тайла × 0.5 тайла
inline const sf::Vector2f kSizeLinearSaw     {48.f, 48.f};
inline const sf::Vector2f kSizeFerrisHub     {32.f, 32.f};
inline const sf::Vector2f kSizeFerrisCabin   {64.f, 24.f};
inline const sf::Vector2f kSizeWood          {96.f, 32.f};   // длина по умолчанию
inline const sf::Vector2f kSizeTreadmill     {144.f, 32.f};  // 3 тайла × 0.66 тайла

// ── Цвета-заглушки (когда нет текстуры) ───────────────────────────────────
inline const sf::Color kColorCrossbow        {130, 90, 50};
inline const sf::Color kColorArrow           {220, 200, 80};
inline const sf::Color kColorCannon          {80, 80, 90};
inline const sf::Color kColorCannonball      {30, 30, 35};
inline const sf::Color kColorTurret          {110, 110, 120};
inline const sf::Color kColorBullet          {255, 230, 80};
inline const sf::Color kColorMine            {200, 60, 60};
inline const sf::Color kColorBlade           {180, 200, 220};
inline const sf::Color kColorBarbedWire      {220, 50, 50};
inline const sf::Color kColorLinearSaw       {200, 200, 215};
inline const sf::Color kColorFerrisHub       {180, 140, 60};
inline const sf::Color kColorFerrisCabin     {120, 80, 40};
inline const sf::Color kColorWood            {150, 100, 50};
inline const sf::Color kColorTreadmill       {90, 90, 100};

} // namespace ek
