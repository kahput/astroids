#pragma once

#include "asteroid.h"
#include "common.h"
#include "core/arena.h"
#include "player.h"
#include "pong_boss.h"
#include "weapon.h"
#include <raylib.h>

#define MAX_STARS 200

typedef struct {
	Vector2 position;
	float speed;
	float size;
	Color color;
} Star;

typedef enum {
	GAME_PHASE_MENU,
	GAME_PHASE_ASTEROIDS,
	GAME_PHASE_BOSS,
	GAME_PHASE_WIN,
	GAME_PHASE_LOSE,
	GAME_PHASE_COUNT,
} GamePhase;

typedef struct {
	Arena frame;

	Player player;
	PaddleEncounter boss;
	BulletSystem weapon_system;
	AsteroidSystem asteroid_system;

	Star stars[MAX_STARS];

	Texture *atlas;
	Texture *paddle_texture;
	Shader *white;

	uint32_t score;
    uint32_t high_score;

	float phase_timer;
	FSM state_machine;

    bool running;

	Rectangle bar;
	Rectangle boss_health_bar;

    float screen_fade;
    bool fading_out;

	bool32 disable_collisions;
	bool32 show_debug, show_ui;
} GameWorld;

void world_init(GameWorld *world, Texture *atlas, Texture *paddle_tex, Shader *white);
void world_update(GameWorld *world, float dt);
void world_draw(GameWorld *world);
