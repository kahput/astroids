#pragma once

#include "entity.h"
#include "globals.h"

typedef struct {
	Entity entity;

	float speed;
	float damage;
	float life_timer;
} Bullet;

typedef struct {
	Bullet bullets[MAX_BULLETS];

	float base_damage;
	Texture *texture;

	int next_slot;
} BulletSystem;

bool32 weapon_system_init(BulletSystem *system, Texture *texture);
bool32 weapon_bullet_spawn(BulletSystem *sys, Vector2 spawn_position, float rotation, Vector2 direction, float speed, float damage_multiplier);
void weapon_bullets_update(BulletSystem *sys, float dt);
void weapon_bullets_draw(BulletSystem *weapon_system, bool show_debug);
