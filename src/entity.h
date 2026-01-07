#pragma once

#include "common.h"

#include <raylib.h>
#include <raymath.h>

typedef struct {
	bool active;

	float bullet_life_timer;
	float bullet_damage;
	float respawn_timer;

	Vector2 position;
	Vector2 velocity;
	Vector2 size;
	float rotation;

	Rectangle collision_shape;
	Rectangle hitbox, hurtbox;

	float health;
	float movement_speed;

	Rectangle area;
	Color tint;
	Texture2D *texture;
} Entity;

void entity_update_physics(Entity *entity, float drag, float dt);
void entity_sync_collision(Entity *entity);
void entity_draw(Entity *entity);
