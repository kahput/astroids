#pragma once

#include "fsm.h"
#include "entity.h"
#include "weapon.h"

typedef struct {
	float fire_rate;
	float fire_timer;

	float speed;
	float base_damage;
} BulletInfo;

typedef struct {
	Entity entity;
	float respawn_timer;

	float rotation_speed;
	float acceleration;
	float drag;

	BulletInfo info;

	uint32_t animation_frame;
	float animation_timer;

	FSM state_machine;
} Player;

bool32 player_init(Player *player, Texture *texture);
void player_update(Player *player, BulletSystem *bullets, float dt);
void player_draw(Player *player);

void player_kill(Player *player);
