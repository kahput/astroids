#pragma once

#include "common.h"

#include "fsm.h"
#include "entity.h"
#include "globals.h"
#include <raylib.h>

typedef struct {
	Entity entity;
	float flash_timer;

	uint32_t animation_frame;
	float animation_timer;

	// DEBUG
	float target_y;
} Paddle;

typedef struct {
	bool32 active;

	Vector2 position;
	float radius;

	Rectangle area;
	Vector2 velocity;
} Ball;

typedef enum {
	SCENARIO_TYPE_NONE = 0,
	SCENARIO_FLAG_WARNING = 1 << 0,
	SCENARIO_FLAG_PONG_ENTER = 1 << 1,
	SCENARIO_FLAG_BALL_ENTER = 1 << 2,
	SCENARIO_FLAG_SPLIT = 1 << 3,
} ScenarioFlags;

typedef struct {
	ScenarioFlags type;
	Vector2 base_position;

	Rectangle warnings[4];

	float timer;
	float duration;
} ScenarioConfig;

typedef struct {
	float health;
	float max_health;
	Paddle paddles[2];

	Paddle *survivor;

	Entity bricks[MAX_BRICKS];
	Entity projectiles[BRICK_MAX_PROJECTILES];

	FSM state_machine;
	ScenarioConfig active_scenario;
	Vector2 player_position;

	Texture *paddle_texture;

	Ball balls[2];
} PaddleEncounter;

bool32 boss_encounter_paddle_initialize(PaddleEncounter *encounter, Texture *texture);
void boss_encounter_paddle_update(PaddleEncounter *boss, Vector2 player_position, float dt);
void boss_encounter_paddle_draw(PaddleEncounter *encounter, bool32 show_debug);

void boss_paddle_apply_damage(PaddleEncounter *encounter, uint32_t paddle_index, float damage);
void boss_paddle_is_alive(Paddle *boss);

float boss_encounter_paddle_health_ratio(PaddleEncounter *encounter);
bool32 boss_encounter_paddle_check_collision(PaddleEncounter *encounter, Entity *player);
