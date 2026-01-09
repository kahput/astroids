#pragma once

#include "common.h"

#include <raylib.h>
#include <raymath.h>

typedef struct {
	bool32 active;

	Vector2 position;
	Vector2 velocity;
	Vector2 size;
	float rotation;

	Rectangle collision_shape;

    float bullet_damage;
    float bullet_life_timer;

	Rectangle area;
	Color tint;
	Texture2D *texture;
} Entity;

void entity_update_physics(Entity *entity, float drag, float dt);
void entity_sync_collision(Entity *entity);
void entity_draw(Entity *entity);
