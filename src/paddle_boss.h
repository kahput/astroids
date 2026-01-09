#pragma once

#include "common.h"

#include "fsm.h"
#include "entity.h"
#include "game.h"

typedef struct {
	Entity entity;

	float health;
	float max_health;
	float flash_timer;

    uint32_t animation_frame; float animation_timer;

	float target_y;
} Paddle;

typedef struct {
	bool32 active;

	Vector2 position;
	float radius;

	Vector2 velocity;
} Ball;

typedef enum {
	SCENARIO_TYPE_NONE,
	SCENARIO_TYPE_ENTERANCE,
} ScenarioType;

typedef struct {
	ScenarioType type;
	float timer;
	float duration;
} ScenarioConfig;

typedef struct {
	Paddle paddles[2];

	FSM state_machine;
	ScenarioConfig active_scenario;
	Vector2 player_position;
	Ball ball;

    Music *music;

	GameContext *game_context;
} PaddleEncounter;

bool32 boss_encounter_paddle_initialize(GameContext *context, PaddleEncounter *encounter, Texture *texture, Music *music);
void boss_encounter_paddle_update(PaddleEncounter *boss, Vector2 player_position, float dt);
void boss_encounter_paddle_draw(PaddleEncounter *boss);

void boss_paddle_apply_damage(Paddle *boss, float damage);
void boss_paddle_is_alive(Paddle *boss);

float boss_encounter_paddle_health_ratio(PaddleEncounter *encounter);
bool32 boss_encounter_paddle_check_collision(PaddleEncounter *encounter, Entity *player);
