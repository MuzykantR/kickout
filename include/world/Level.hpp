#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <utility>
#include <vector>

struct PlacedEntity {
    std::string type;
    sf::Vector2f position;
    float fireInterval = 1.5f;
    sf::Vector2f projectileVelocity{400.f, 0.f};
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
    bool loadFromString(const std::string& content, const std::string& debugName, std::string& outError);
    bool loadFromJsonString(const std::string& jsonUtf8, const std::string& debugName, std::string& outError);

    int width() const { return m_gridColumns; }
    int height() const { return m_gridRows; }
    float tileSize() const { return m_tileSize; }

    sf::Vector2f pixelSize() const {
        return {m_tileSize * static_cast<float>(m_gridColumns), m_tileSize * static_cast<float>(m_gridRows)};
    }

    sf::Vector2f spawnPoint() const { return m_spawnPoint; }
    void setSpawnPoint(sf::Vector2f p) { m_spawnPoint = p; }

    const std::vector<LevelPlatformObject>& platforms() const { return m_platforms; }
    const std::vector<PlacedEntity>& placedEntities() const { return m_placedEntities; }

    bool overlapsSolid(const sf::FloatRect& worldRect) const;
    bool overlapsHazard(const sf::FloatRect& worldRect) const;
    /// Зона победы: отдельные AABB + запас (кромка коллизии SFML)
    bool overlapsFinish(const sf::FloatRect& worldRect) const;

    Tile sampleGroundBelow(const sf::FloatRect& playerBounds) const;

    void draw(sf::RenderTarget& target,
              const std::map<std::string, const sf::Texture*>* textureOverrides = nullptr) const;

private:
    static bool platformTypeStringToTile(const std::string& s, Tile& out, std::string& err);
    static int tileLayerOrder(Tile t);
    static bool rectIntersects(const sf::FloatRect& a, const sf::FloatRect& b);
    static sf::FloatRect inflateRect(const sf::FloatRect& r, float amount);
    static sf::Color kindToDebugColor(Tile k);

    bool parseLine(const std::string& line, std::vector<Tile>& row, int& outPlayerCol, bool& outHasPlayer,
                   std::vector<std::pair<int, std::string>>& outSpawnsInRow);

    float m_tileSize = 48.f;
    int m_gridColumns = 0;
    int m_gridRows = 0;

    std::vector<LevelPlatformObject> m_platforms;
    std::vector<sf::FloatRect> m_finishTriggers;
    std::vector<PlacedEntity> m_placedEntities;
    sf::Vector2f m_spawnPoint{64.f, 64.f};
};

#endif
