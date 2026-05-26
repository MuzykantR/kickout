#!/usr/bin/env python3
"""Экспорт: ASCII-сетка -> level.json (формат движка kickout).

Пример:
  python scripts/grid_to_level_json.py scripts/sample_grids/level01.grid.txt assets/levels/level01.json
"""
import json
import sys
from pathlib import Path

TYPE_MAP = {
    "#": "solid",
    "1": "solid",
    "I": "ice",
    "i": "ice",
    "^": "spring",
    "X": "hazard",
    "x": "hazard",
    "S": "hazard",
    "F": "finish",
    "f": "finish",
}


def export(grid_path: Path, out_path: Path, tile_size: float = 48.0) -> None:
    lines = [ln.rstrip("\r") for ln in grid_path.read_text(encoding="utf-8").splitlines() if ln.strip()]
    if not lines:
        raise SystemExit("empty grid")
    w = len(lines[0])
    if any(len(L) != w for L in lines):
        raise SystemExit("ragged grid")

    h = len(lines)
    spawn = None
    traps = []

    platforms = []

    for ty, row in enumerate(lines):
        tx = 0
        while tx < w:
            ch = row[tx]
            if ch in ". ":
                tx += 1
                continue
            if ch == "@":
                cx = (tx + 0.5) * tile_size
                cy = (ty + 0.5) * tile_size
                spawn = {"x": cx, "y": cy}
                tx += 1
                continue
            if ch == "c":
                traps.append(
                    {
                        "type": "crossbow",
                        "x": (tx + 0.5) * tile_size,
                        "y": (ty + 0.5) * tile_size,
                        "fireInterval": 1.5,
                        "projectileVelocity": [400, 0],
                    }
                )
                tx += 1
                continue
            if ch == "h":
                traps.append(
                    {
                        "type": "crossbow_fast",
                        "x": (tx + 0.5) * tile_size,
                        "y": (ty + 0.5) * tile_size,
                        "fireInterval": 0.85,
                        "projectileVelocity": [600, 0],
                    }
                )
                tx += 1
                continue
            if ch not in TYPE_MAP:
                raise SystemExit(f"bad char {ch!r} at {tx},{ty}")

            kind = TYPE_MAP[ch]
            t0 = tx
            while tx < w and row[tx] == ch:
                tx += 1
            pw = (tx - t0) * tile_size
            px = t0 * tile_size
            py = ty * tile_size
            platforms.append(
                {
                    "type": kind,
                    "x": px,
                    "y": py,
                    "w": pw,
                    "h": tile_size,
                    "texture": f"stub_{kind}",
                }
            )

    if spawn is None:
        spawn = {"x": tile_size * 0.5, "y": tile_size * 0.5}

    doc = {
        "version": 1,
        "tileSize": tile_size,
        "grid": {"columns": w, "rows": h},
        "spawn": spawn,
        "platforms": platforms,
        "traps": traps,
    }
    out_path.write_text(json.dumps(doc, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"wrote {out_path}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: grid_to_level_json.py <grid.txt> <out.json>")
        sys.exit(1)
    export(Path(sys.argv[1]), Path(sys.argv[2]))
