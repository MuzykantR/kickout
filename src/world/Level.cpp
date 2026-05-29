#include "world/Level.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <utility>

bool Level::rectIntersects(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.intersects(b);
}

sf::FloatRect Level::inflateRect(const sf::FloatRect& r, float amount) {
    return {r.left - amount, r.top - amount, r.width + 2.f * amount, r.height + 2.f * amount};
}

sf::Color Level::kindToDebugColor(Tile k) {
    switch (k) {
    case Tile::Solid:   return {55, 55, 72};
    case Tile::Ice:     return {150, 210, 255};
    case Tile::Spring:  return {255, 200, 80};
    case Tile::Hazard:  return {200, 40, 40};
    case Tile::Finish:  return {80, 200, 90};
    default:            return sf::Color::White;
    }
}

bool Level::platformTypeStringToTile(const std::string& s, Tile& out, std::string& err) {
    if (s == "solid")                  { out = Tile::Solid;  return true; }
    if (s == "ice")                    { out = Tile::Ice;    return true; }
    if (s == "spring")                 { out = Tile::Spring; return true; }
    if (s == "hazard" || s == "spikes"){ out = Tile::Hazard; return true; }
    if (s == "finish")                 { out = Tile::Finish; return true; }
    err = "Unknown platform type: " + s;
    return false;
}

int Level::tileLayerOrder(Tile t) {
    switch (t) {
    case Tile::Solid:   return 0;
    case Tile::Ice:     return 1;
    case Tile::Spring:  return 2;
    case Tile::Finish:  return 3;
    case Tile::Hazard:  return 4;
    default:            return -1;
    }
}

bool Level::loadFromJsonString(const std::string& jsonUtf8, const std::string& debugName,
                               std::string& outError) {
    m_placedEntities.clear();
    m_platforms.clear();
    m_finishTriggers.clear();
    m_dynPlatformDefs.clear();
    m_backgroundPath.clear();

    try {
        nlohmann::json j = nlohmann::json::parse(jsonUtf8);
        (void)j.value("version", 1);

        if (j.contains("background") && j["background"].is_string()) {
            m_backgroundPath = j["background"].get<std::string>();
        }

        m_tileSize = j.value("tileSize", 48.f);
        if (m_tileSize <= 0.f) {
            outError = debugName + ": tileSize must be positive";
            return false;
        }

        if (!j.contains("grid") || !j["grid"].contains("columns") ||
            !j["grid"].contains("rows")) {
            outError = debugName + ": missing grid.columns / grid.rows";
            return false;
        }
        m_gridColumns = j["grid"]["columns"].get<int>();
        m_gridRows    = j["grid"]["rows"].get<int>();
        if (m_gridColumns <= 0 || m_gridRows <= 0) {
            outError = debugName + ": invalid grid size";
            return false;
        }

        if (j.contains("spawn")) {
            const auto& sp = j["spawn"];
            m_spawnPoint = {sp.value("x", 0.f), sp.value("y", 0.f)};
        } else {
            m_spawnPoint = {m_tileSize * 0.5f, m_tileSize * 0.5f};
        }

        if (j.contains("platforms") && j["platforms"].is_array()) {
            for (const auto& p : j["platforms"]) {
                if (!p.contains("type")) {
                    outError = debugName + ": platform without type";
                    return false;
                }
                const std::string typ = p["type"].get<std::string>();
                Tile tile{};
                std::string e2;
                if (!platformTypeStringToTile(typ, tile, e2)) {
                    outError = debugName + ": " + e2;
                    return false;
                }
                const float pw = p.value("w", 0.f);
                const float ph = p.value("h", 0.f);
                if (pw <= 0.f || ph <= 0.f) {
                    outError = debugName + ": platform with non-positive size";
                    return false;
                }
                LevelPlatformObject obj;
                obj.bounds    = {p.value("x", 0.f), p.value("y", 0.f), pw, ph};
                obj.kind      = tile;
                obj.textureId = p.contains("texture") && p["texture"].is_string()
                                    ? p["texture"].get<std::string>()
                                    : "stub_" + typ;
                m_platforms.push_back(std::move(obj));
            }
        }

        std::stable_sort(m_platforms.begin(), m_platforms.end(),
                         [](const LevelPlatformObject& a, const LevelPlatformObject& b) {
                             return tileLayerOrder(a.kind) < tileLayerOrder(b.kind);
                         });

        for (const auto& pl : m_platforms) {
            if (pl.kind == Tile::Finish) {
                m_finishTriggers.push_back(pl.bounds);
            }
        }
        if (j.contains("finishZones") && j["finishZones"].is_array()) {
            for (const auto& z : j["finishZones"]) {
                const float zw = z.value("w", 0.f);
                const float zh = z.value("h", 0.f);
                if (zw <= 0.f || zh <= 0.f) {
                    outError = debugName + ": finishZones entry needs positive w,h";
                    return false;
                }
                m_finishTriggers.push_back({z.value("x", 0.f), z.value("y", 0.f), zw, zh});
            }
        }

        if (j.contains("traps") && j["traps"].is_array()) {
            for (const auto& t : j["traps"]) {
                if (!t.contains("type")) {
                    outError = debugName + ": trap without type";
                    return false;
                }
                PlacedEntity pe;
                pe.type     = t["type"].get<std::string>();
                pe.position = {t.value("x", 0.f), t.value("y", 0.f)};

                const bool hasFi = t.contains("fireInterval");
                const bool hasPv = t.contains("projectileVelocity") &&
                                   t["projectileVelocity"].is_array() &&
                                   t["projectileVelocity"].size() >= 2;

                if (pe.type == "crossbow_fast") {
                    pe.fireInterval         = hasFi ? t["fireInterval"].get<float>() : 0.85f;
                    pe.projectileVelocity   = hasPv
                        ? sf::Vector2f{t["projectileVelocity"][0].get<float>(),
                                       t["projectileVelocity"][1].get<float>()}
                        : sf::Vector2f{600.f, 0.f};
                } else if (pe.type == "blades" || pe.type == "rotating_blades") {
                    pe.rotationSpeed = t.value("rotationSpeed", t.value("bladeSpeed", 180.f));
                    pe.size = {
                        t.value("w", 0.f),
                        t.value("h", 0.f)
                    };
                } else {
                    pe.fireInterval         = hasFi ? t["fireInterval"].get<float>() : 1.5f;
                    pe.projectileVelocity   = hasPv
                        ? sf::Vector2f{t["projectileVelocity"][0].get<float>(),
                                       t["projectileVelocity"][1].get<float>()}
                        : sf::Vector2f{400.f, 0.f};
                }
                m_placedEntities.push_back(std::move(pe));
            }
        }

        // ── Динамические платформы ──────────────────────────────────────────
        if (j.contains("dynamic_platforms") && j["dynamic_platforms"].is_array()) {
            for (const auto& dp : j["dynamic_platforms"]) {
                if (!dp.contains("type")) {
                    outError = debugName + ": dynamic_platform without type";
                    return false;
                }
                const std::string typ = dp["type"].get<std::string>();
                DynamicPlatformDef def;
                const float px = dp.value("x", 0.f);
                const float py = dp.value("y", 0.f);

                if (typ == "conveyor" || typ == "conveyor_belt") {
                    def.kind = DynamicPlatformDef::Kind::Conveyor;
                    def.widthInTiles = dp.value("widthInTiles", 1);
                    if (def.widthInTiles <= 0) {
                        outError = debugName + ": conveyor widthInTiles must be positive";
                        return false;
                    }
                    const float beltW = m_tileSize * static_cast<float>(def.widthInTiles);
                    def.bounds = {px, py, beltW, m_tileSize};

                    const float speed = dp.value("speed", 150.f);
                    float dirX = 1.f;
                    float dirY = 0.f;
                    if (dp.contains("direction") && dp["direction"].is_array() &&
                        dp["direction"].size() >= 2) {
                        dirX = dp["direction"][0].get<float>();
                        dirY = dp["direction"][1].get<float>();
                    } else {
                        dirX = dp.value("directionX", 1.f);
                        dirY = dp.value("directionY", 0.f);
                    }
                    const float len = std::sqrt(dirX * dirX + dirY * dirY);
                    if (len > 1e-4f) {
                        dirX /= len;
                        dirY /= len;
                    }
                    def.conveyorVelocity = {dirX * speed, dirY * speed};
                } else if (typ == "vanishing") {
                    def.kind = DynamicPlatformDef::Kind::Vanishing;
                    def.widthInTiles = dp.value("widthInTiles", 1);
                    if (def.widthInTiles <= 0) {
                        outError = debugName + ": vanishing widthInTiles must be positive";
                        return false;
                    }
                    const float beltW = m_tileSize * static_cast<float>(def.widthInTiles);
                    def.bounds = {px, py, beltW, m_tileSize};
                    def.deathTime = dp.value("deathTime", 1.5f);
                } else {
                    const float pw = dp.value("w", 0.f);
                    const float ph = dp.value("h", 0.f);
                    if (pw <= 0.f || ph <= 0.f) {
                        outError = debugName + ": dynamic_platform with non-positive size";
                        return false;
                    }
                    def.bounds = {px, py, pw, ph};

                    if (typ == "moving") {
                        def.kind       = DynamicPlatformDef::Kind::Moving;
                        def.moveOffset = {dp.value("offsetX", 0.f), dp.value("offsetY", 0.f)};
                        def.moveSpeed  = dp.value("speed", 100.f);
                    } else {
                        outError = debugName + ": unknown dynamic_platform type: " + typ;
                        return false;
                    }
                }
                m_dynPlatformDefs.push_back(std::move(def));
            }
        }

        return true;
    } catch (const nlohmann::json::parse_error& e) {
        outError = std::string("JSON parse error in ") + debugName + ": " + e.what();
        return false;
    } catch (const nlohmann::json::exception& e) {
        outError = std::string("JSON error in ") + debugName + ": " + e.what();
        return false;
    }
}

bool Level::parseLine(const std::string& line, std::vector<Tile>& row, int& outPlayerCol,
                      bool& outHasPlayer, std::vector<std::pair<int, std::string>>& outSpawnsInRow) {
    row.clear();
    outSpawnsInRow.clear();
    int col = 0;
    for (char ch : line) {
        if (ch == '\r') continue;
        switch (ch) {
        case '#': case '1': row.push_back(Tile::Solid);  break;
        case 'I': case 'i': row.push_back(Tile::Ice);    break;
        case '^':            row.push_back(Tile::Spring); break;
        case 'X': case 'x': case 'S': row.push_back(Tile::Hazard); break;
        case 'F': case 'f': row.push_back(Tile::Finish); break;
        case '@':
            row.push_back(Tile::Empty);
            outPlayerCol = col;
            outHasPlayer = true;
            break;
        case 'c':
            row.push_back(Tile::Empty);
            outSpawnsInRow.push_back({col, "crossbow"});
            break;
        case 'h':
            row.push_back(Tile::Empty);
            outSpawnsInRow.push_back({col, "crossbow_fast"});
            break;
        case '.': case ' ': row.push_back(Tile::Empty); break;
        default: return false;
        }
        ++col;
    }
    return !row.empty();
}

bool Level::loadFromString(const std::string& content, const std::string& debugName,
                           std::string& outError) {
    m_placedEntities.clear();
    m_platforms.clear();
    m_finishTriggers.clear();
    m_dynPlatformDefs.clear();

    std::istringstream in(content);
    std::string line;
    std::vector<std::vector<Tile>> rows;
    bool hasSpawn = false;
    int spawnCol = 0;
    int spawnRow = 0;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::vector<Tile> row;
        int playerCol      = -1;
        bool lineHasPlayer = false;
        std::vector<std::pair<int, std::string>> rowSpawns;
        if (!parseLine(line, row, playerCol, lineHasPlayer, rowSpawns)) {
            outError = "Bad character in level: " + debugName;
            return false;
        }
        if (!rows.empty() && row.size() != rows.front().size()) {
            outError = "Ragged level row in: " + debugName;
            return false;
        }
        const int currentRowIndex = static_cast<int>(rows.size());
        const float half = m_tileSize * 0.5f;
        for (const auto& spawn : rowSpawns) {
            m_placedEntities.push_back(
                {spawn.second,
                 {static_cast<float>(spawn.first) * m_tileSize + half,
                  static_cast<float>(currentRowIndex) * m_tileSize + half}});
        }
        if (lineHasPlayer) {
            if (hasSpawn) {
                outError = "Multiple @ spawn markers in: " + debugName;
                return false;
            }
            hasSpawn = true;
            spawnCol = playerCol;
            spawnRow = static_cast<int>(rows.size());
        }
        rows.push_back(std::move(row));
    }

    if (rows.empty()) {
        outError = "Empty level: " + debugName;
        return false;
    }

    m_gridColumns = static_cast<int>(rows.front().size());
    m_gridRows    = static_cast<int>(rows.size());

    for (int y = 0; y < m_gridRows; ++y) {
        for (int x = 0; x < m_gridColumns; ++x) {
            const Tile t = rows[static_cast<size_t>(y)][static_cast<size_t>(x)];
            if (t == Tile::Empty) continue;
            LevelPlatformObject obj;
            obj.bounds    = {static_cast<float>(x) * m_tileSize,
                             static_cast<float>(y) * m_tileSize,
                             m_tileSize, m_tileSize};
            obj.kind      = t;
            obj.textureId = "stub_ascii_cell";
            m_platforms.push_back(std::move(obj));
        }
    }

    std::stable_sort(m_platforms.begin(), m_platforms.end(),
                     [](const LevelPlatformObject& a, const LevelPlatformObject& b) {
                         return tileLayerOrder(a.kind) < tileLayerOrder(b.kind);
                     });

    for (const auto& pl : m_platforms) {
        if (pl.kind == Tile::Finish) {
            m_finishTriggers.push_back(pl.bounds);
        }
    }

    const float half = m_tileSize * 0.5f;
    m_spawnPoint = hasSpawn
        ? sf::Vector2f{static_cast<float>(spawnCol) * m_tileSize + half,
                       static_cast<float>(spawnRow) * m_tileSize + half}
        : sf::Vector2f{half, half};

    return true;
}

bool Level::loadFromFile(const std::string& path, std::string& outError) {
    std::ifstream f(path);
    if (!f) {
        outError = "Cannot open level file: " + path;
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    const std::string content = ss.str();

    const std::string lower = [&] {
        std::string s = path;
        for (char& c : s)
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        return s;
    }();

    const bool isJson = lower.size() >= 5 &&
                        lower.compare(lower.size() - 5, 5, ".json") == 0;
    return isJson ? loadFromJsonString(content, path, outError)
                  : loadFromString(content, path, outError);
}

// ── Коллизии ───────────────────────────────────────────────────────────────

bool Level::overlapsSolid(const sf::FloatRect& worldRect) const {
    for (const auto& pl : m_platforms) {
        if (pl.kind == Tile::Solid || pl.kind == Tile::Ice || pl.kind == Tile::Spring) {
            if (rectIntersects(worldRect, pl.bounds)) return true;
        }
    }
    // Динамические платформы (движущиеся / ещё не исчезнувшие)
    for (const auto& r : m_dynamicSolids) {
        if (rectIntersects(worldRect, r)) return true;
    }
    return false;
}

bool Level::overlapsHazard(const sf::FloatRect& worldRect) const {
    for (const auto& pl : m_platforms) {
        if (pl.kind == Tile::Hazard && rectIntersects(worldRect, pl.bounds)) return true;
    }
    return false;
}

bool Level::overlapsFinish(const sf::FloatRect& worldRect) const {
    constexpr float kPad         = 14.f;
    constexpr float kExtraBottom = 20.f;
    for (sf::FloatRect zone : m_finishTriggers) {
        zone = inflateRect(zone, kPad);
        zone.height += kExtraBottom;
        if (rectIntersects(worldRect, zone)) return true;
    }
    return false;
}

Tile Level::sampleGroundBelow(const sf::FloatRect& playerBounds) const {
    sf::FloatRect foot = playerBounds;
    foot.top  += playerBounds.height;
    foot.height = 6.f;

    bool  found   = false;
    float bestTop = 1e9f;
    Tile  best    = Tile::Empty;

    for (const auto& pl : m_platforms) {
        if (pl.kind != Tile::Solid && pl.kind != Tile::Ice && pl.kind != Tile::Spring) continue;
        if (!rectIntersects(foot, pl.bounds)) continue;
        if (!found || pl.bounds.top < bestTop) {
            bestTop = pl.bounds.top;
            best    = pl.kind;
            found   = true;
        }
    }
    // Динамические платформы — обычный Solid (нет льда/пружин)
    for (const auto& r : m_dynamicSolids) {
        if (rectIntersects(foot, r)) {
            if (!found || r.top < bestTop) {
                bestTop = r.top;
                best    = Tile::Solid;
                found   = true;
            }
        }
    }
    return best;
}

// ── Отрисовка ──────────────────────────────────────────────────────────────

void Level::draw(sf::RenderTarget& target,
                 const std::map<std::string, const sf::Texture*>* textureOverrides) const {
    for (const auto& pl : m_platforms) {
        const sf::Texture* tex = nullptr;
        if (textureOverrides != nullptr && !pl.textureId.empty()) {
            auto it = textureOverrides->find(pl.textureId);
            if (it != textureOverrides->end()) tex = it->second;
        }

        if (tex != nullptr) {
            sf::Sprite spr(*tex);
            spr.setPosition(pl.bounds.left, pl.bounds.top);
            const sf::Vector2f sz{pl.bounds.width, pl.bounds.height};
            const sf::Vector2f ts(static_cast<float>(tex->getSize().x),
                                  static_cast<float>(tex->getSize().y));
            spr.setTextureRect({0, 0, (int)pl.bounds.width, (int)pl.bounds.height});
            target.draw(spr);
        } else {
            sf::RectangleShape sh({std::max(1.f, pl.bounds.width  - 1.f),
                                   std::max(1.f, pl.bounds.height - 1.f)});
            sh.setPosition(pl.bounds.left + 0.5f, pl.bounds.top + 0.5f);
            sh.setFillColor(kindToDebugColor(pl.kind));
            sh.setOutlineColor({30, 30, 40});
            sh.setOutlineThickness(1.f);
            target.draw(sh);
        }
    }
}