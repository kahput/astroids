#pragma once

#include <raylib.h>

#include "entity.h"
#include "game.h"

typedef enum {
	BOSS_PHASE_INTRO,
	BOSS_PHASE_PATTERN_A,
	BOSS_PHASE_ENRAGED,
	BOSS_PHASE_DEAD,
} BossPhase;

typedef struct {
	Entity entity;

	float flash_timer;

	float max_health;
	float health;
} Boss;

typedef struct {
	Boss bosses[2];
	float total_max_health;

	float state_timer;
	BossPhase phase;
} BossEncounter;

void boss_encounter_paddle_initialize(GameContext *context, BossEncounter *encounter, Texture2D *paddle_texture);
void boss_encounter_paddle_update(GameContext *context, BossEncounter *encounter, Vector2 player_pos);
bool boss_encounter_paddle_check_collision(BossEncounter *encounter, Entity *player);
void boss_encounter_paddle_draw(GameContext *context, BossEncounter *encounter);

float boss_fight_health_ratio(BossEncounter *encounter);
void boss_apply_damage(Boss *boss, float damage);
bool boss_is_alive(Boss *boss);
