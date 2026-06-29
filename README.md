# BulletMap — High Performance Bullet Manager for Godot 4

A GDExtension bullet management system written in C++, designed for games that need to simulate and render large numbers of bullets efficiently.

Instead of using individual nodes per bullet, BulletMap uses Godot's MultiMesh and writes directly to the rendering buffer, keeping CPU overhead low even with many active bullets.

---

## How it works

Bullets are stored as slots in a flat data structure. When a bullet is fired, it takes a slot from the dead bullet pool. When it expires, the slot is returned to the pool. No memory is allocated or freed during normal gameplay.

Collision and rendering are handled in the same pass where possible, avoiding redundant iteration over active bullets.

Bullets are sorted into **movement buckets** and optionally **visuals buckets** at spawn time. Each bucket maps to a specific processing method via a function pointer array, making it straightforward to add new movement or visual types without modifying existing logic.

Collision targets are organized into named **collision groups**. Each group holds a list of Node2D targets with circular hitboxes. On collision, the target's `Hit(Vector2)` method is called with the bullet ID.

Bullet IDs are two-part (`Vector2i`: index + instance counter) to prevent stale references from affecting bullets that have been recycled into a new instance.

---

## Features

- MultiMesh instancing with direct buffer writes
- Object pooling — slots are recycled, no per-frame allocation
- Named collision groups with circular hitbox checking
- Atlas sprite support
- Angular velocity for curved and spinning bullet paths
- Movement bucket system for extensible movement types
- Optional visuals bucket for decoupled visual processing
- Pause/unpause and shoot control
- Bullet data readable and swappable at runtime via `GetBulletBasicData`, `GetBulletPosition`, `SwapBulletData`

---

## Current movement types

| Type | Description |
|---|---|
| `MOVEMENT_NORMAL` | Moves bullet along its rotation with angular velocity, updates render buffer |
| `MOVEMENT_NORMAL_NORENDER` | Same movement, skips render buffer update |

---

## Work in progress

- Additional movement types (planned)
- Visuals bucket processing not yet implemented
- Some `BulletData` fields not yet accessible via `SwapBulletData`
- `ClearMap` gradual clearing mode stubbed but not implemented

---

## Basic API

```gdscript
# Spawn a bullet
var id = bullet_map.Shoot(bullet_resource, BulletMap.MOVEMENT_NORMAL, BulletMap.VISUALS_NONE)

# Read bullet data
var speed = bullet_map.GetBulletBasicData(id, BulletMap.BULLET_DATA_SPEED)
var pos = bullet_map.GetBulletPosition(id) # returns Vector3(x, y, rotation)

# Collision groups
bullet_map.AddNewCollisionGroup("enemies")
bullet_map.AddObjective("enemies", enemy_node, hitbox_radius)
bullet_map.RemoveObjective("enemies", enemy_node)

# Control
bullet_map.Pause()
bullet_map.Unpause()
bullet_map.AllowShooting(false)
bullet_map.ClearMap(0) # instant clear
bullet_map.Reset()     # clear + remove all collision groups
```

Collision targets need a `Hit(bullet_id: Vector2)` method. The bullet ID passed is the same `Vector2i` returned by `Shoot()`.

---

## Requirements

- Godot 4.x
- GDExtension (C++ build toolchain required)
