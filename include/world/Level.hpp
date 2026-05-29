#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <vector>

struct PlacedEntity {
    std::string  type;
    sf::Vector2f position;

    // Launcher-based traps
    float        fireInterval       = 1.5f;
    sf::Vector2f projectileVelocity = {400.f, 0.f};
    float        projectileGravity  = 0.f;
    float        angle              = 0.f;    // градусы, для Cannon
    float        projectileSpeed    = 400.f;  // для Turret/Cannon

    // Mine
    float        triggerRadius      = 80.f;
    float        armDelay           = 1.2f;
    float        blastRadius        = 120.f;

    // RotatingBlade
    float        rotationSpeed      = 180.f;
    sf::Vector2f size               = {0.f, 0.f};

    // LinearSaw / FerrisWheel
    sf::Vector2f travelOffset       = {0.f, 0.f};
    float        travelSpeed        = 220.f;
    int          cabinCount         = 4;
    float        wheelRadius        = 120.f;
};

// Описание динамической платформы, прочитанное из JSON-поля "dynamic_platforms"
struct DynamicPlatformDef {
    enum class Kind : uint8_t { Moving, Vanishing, Conveyor, Wood, Treadmill } kind = Kind::Moving;
    sf::FloatRect bounds{};
    // Moving
    sf::Vector2f moveOffset{0.f, 0.f};
    float moveSpeed = 100.f;
    // Vanishing / Wood
    float deathTime = 1.5f;
    // Conveyor / Treadmill
    sf::Vector2f conveyorVelocity{0.f, 0.f};
    int widthInTiles = 1;
};

enum class Tile : uint8_t {
    Empty = 0,
    Solid,
    Ice,
    Spring,
    Hazard,
    Finish,
};

struct LevelPlatformObject {
    sf::FloatRect bounds;
    Tile kind = Tile::Solid;
    std::string textureId;
};

class Level {
public:
    Level() = default;

    bool loadFromFile(const std::string& path, std::string& outError);
    bool loadFromJsonString(const std::string& jsonUtf8, const std::string& debugName,
                            std::string& outError);

    int width() const { return m_gridColumns; }
    int height() const { return m_gridRows; }
    float tileSize() const { return m_tileSize; }

    sf::Vector2f pixelSize() const {
        return {m_tileSize * static_cast<float>(m_gridColumns),
                m_tileSize * static_cast<float>(m_gridRows)};
    }

    sf::Vector2f spawnPoint() const { return m_spawnPoint; }
    void setSpawnPoint(sf::Vector2f p) { m_spawnPoint = p; }

    const std::vector<LevelPlatformObject>& platforms() const { return m_platforms; }
    const std::vector<PlacedEntity>& placedEntities() const { return m_placedEntities; }
    const std::vector<DynamicPlatformDef>& dynamicPlatformDefs() const { return m_dynPlatformDefs; }
    const std::string& backgroundPath() const { return m_backgroundPath; }

    bool overlapsSolid(const sf::FloatRect& worldRect) const;
    bool overlapsHazard(const sf::FloatRect& worldRect) const;
    bool overlapsFinish(const sf::FloatRect& worldRect) const;

    Tile sampleGroundBelow(const sf::FloatRect& playerBounds) const;

    // Game вызывает эти методы каждый кадр, передавая актуальные AABB
    // движущихся и исчезающих платформ, чтобы коллизии работали корректно
    void clearDynamicSolids() { m_dynamicSolids.clear(); }
    void addDynamicSolid(const sf::FloatRect& r) { m_dynamicSolids.push_back(r); }

    void draw(sf::RenderTarget& target,
              const std::map<std::string, const sf::Texture*>* textureOverrides = nullptr) const;

private:
    static bool platformTypeStringToTile(const std::string& s, Tile& out, std::string& err);
    static int tileLayerOrder(Tile t);
    static sf::FloatRect inflateRect(const sf::FloatRect& r, float amount);
    static sf::Color kindToDebugColor(Tile k);

    float m_tileSize = 48.f;
    int m_gridColumns = 0;
    int m_gridRows = 0;

    std::vector<LevelPlatformObject> m_platforms;
    std::vector<sf::FloatRect> m_finishTriggers;
    std::vector<PlacedEntity> m_placedEntities;
    std::vector<DynamicPlatformDef> m_dynPlatformDefs;

    // Заполняется Game каждый кадр — хитбоксы живых динамических платформ
    std::vector<sf::FloatRect> m_dynamicSolids;

    sf::Vector2f m_spawnPoint{64.f, 64.f};
    std::string m_backgroundPath;
};

#endif