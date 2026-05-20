## Kickout — платформер (SFML)

Сборка: **CMake** ≥ 3.20, **C++17**, **SFML 2.6.2** (каталог в `CMakeLists.txt`). Команды из корня: `cmake -B build`, затем `cmake --build build`.

**Управление:** `A/D` или стрелки — движение, `Space` — прыжок (удержание выше), `Shift` — спринт, `W/S` при скольжении вдоль стены — медленнее/быстрее скольжение, `R` — рестарт с первого уровня, `Esc` — выход.

**Уровни — JSON** (`assets/levels/*.json`). Сборка подтягивает **nlohmann/json** через CMake `FetchContent`.

Поля верхнего уровня:

- `version` — число (сейчас `1`).
- `tileSize` — размер ячейки сетки в пикселях (число).
- `grid` — `{ "columns": N, "rows": M }`: **границы мира** (камера, «яма» смерти); сетка коллизий **не** хранится — только список объектов.
- `spawn` — `{ "x", "y" }`: центр игрока в **мировых пикселях**. Учитывай высоту хитбокса (~52px): центр не должен поднимать верх тела в потолок первого ряда (иначе пересечение с `solid`).
- `platforms` — массив **неподвижных объектов** (неизменяемые AABB): `{ "type", "x", "y", "w", "h" }`, `(x,y)` — левый верх.
  - `type`: `"solid"` | `"ice"` | `"spring"` | `"hazard"` | `"spikes"` | `"finish"`.
  - `texture` (опционально): строковый ключ; если в игре передана карта текстур в `Level::draw`, объект рисуется спрайтом, иначе — **цветная заглушка**.
- `traps` — динамические ловушки (как сущности): `crossbow` / `crossbow_fast`, позиция — **центр**, см. поля `fireInterval`, `projectileVelocity`.

Отрисовка: сначала объекты с меньшим «слоем», затём hazard поверх. Коллизии — пересечение хитбокса игрока/снаряда с `platforms`.

**Черновик из старой ASCII-сетки** (только для редактора/репо): `python scripts/grid_to_level_json.py scripts/sample_grids/level01.grid.txt assets/levels/level01.json`

По желанию положите `assets/sounds/jump.wav` и `death.wav` — иначе игра молча обходится без них.

```mermaid
classDiagram
    %% Базовый уровень
    class Entity {
        <<abstract>>
        +update(dt, spawnQueue)*
        +draw(window)*
        +getBounds() FloatRect
        #m_sprite Sprite
    }

    %% Ветка лаунчеров (Пусковые установки)
    Entity <|-- Launcher
    class Launcher {
        <<abstract>>
        +update(dt, spawnQueue)
        #fire(spawnQueue)*
        #m_timer float
        #m_fireInterval float
    }

    Launcher <|-- Crossbow : "Стреляет стрелами"
    Launcher <|-- HockeyMachine : "Стреляет шайбами"
    Launcher <|-- TennisShooter : "Стреляет мячами"

    %% Ветка снарядов
    Entity <|-- Projectile
    class Projectile {
        +update(dt, spawnQueue)
        +destroy()
        +isExpired() bool
        #m_velocity Vector2f
        #m_gravity float
        #m_isExpired bool
    }

    %% Особые случаи
    Entity <|-- BoxingGlove : "Сложная логика (туда-обратно)"
    Entity <|-- WreckingBall : "Физика маятника"
    Entity <|-- BarbedWire : "Просто статичный Hazard"

    note for Projectile "Стрелы, Шайбы и Мячи используют этот класс\nс разными параметрами скорости и гравитации"
    note for BoxingGlove "Не наследует Projectile, так как\nне исчезает и возвращается"
```