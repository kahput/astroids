#include "paddle_boss.h"

#include "audio_manager.h"
#include "common.h"
#include "core/logger.h"
#include "entity.h"
#include "fsm.h"
#include <raylib.h>

#define BALL_SPEED_INITIAL 600.0f
#define BALL_SPEED_MAX 1200.0f
#define PADDLE_MOVE_SPEED 450.0f
#define PADDLE_AI_REACTION_DELAY 0.0f

void paddle_enterance_enter(void *context);
StateID paddle_enterance_update(void *context, float dt);
void paddle_enterance_exit(void *context);

void paddle_pong_enter(void *context);
StateID paddle_pong_update(void *context, float dt);
void paddle_pong_exit(void *context);

void paddle_death_enter(void *context);
StateID paddle_death_update(void *context, float dt);
void paddle_death_exit(void *context);

typedef enum {
	PADDLE_STATE_ENTERANCE,
	PADDLE_STATE_PONG,
	PADDLE_STATE_DEATH,

	PADDLE_STATE_COUNT,
} PaddleState;

static const char *stringify_state[PADDLE_STATE_COUNT] = {
	[PADDLE_STATE_ENTERANCE] = "STATE_INTRO",
	[PADDLE_STATE_PONG] = "STATE_PONG",
	[PADDLE_STATE_DEATH] = "STATE_DEATH",
};

bool32 boss_encounter_paddle_initialize(GameContext *context, PaddleEncounter *encounter, Texture *texture) {
	*encounter = (PaddleEncounter){ 0 };

	float max_health = 100.f;

	encounter->game_context = context;

	for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); ++paddle_index) {
		Paddle *paddle = &encounter->paddles[paddle_index];

		paddle->entity.active = true;
		paddle->entity.size = (Vector2){ context->tile_size * 2, context->tile_size * 8 };

		paddle->entity.velocity = (Vector2){ 0, 0 };
		paddle->entity.tint = WHITE;

		paddle->entity.area = (Rectangle){ 0, (context->tile_size * 2) * paddle_index, context->tile_size, context->tile_size * 2 };
		paddle->entity.texture = texture;

		paddle->entity.collision_shape = (Rectangle){ 0, 0, paddle->entity.size.x, paddle->entity.size.y };

		paddle->max_health = max_health * .5f;
		paddle->health = paddle->max_health;
		paddle->flash_timer = 0.0f;
	}

	StateHandler enterance_state = {
		.on_enter = paddle_enterance_enter,
		.on_exit = paddle_enterance_exit,
		.on_update = paddle_enterance_update,
	};

	StateHandler pong_state = {
		.on_enter = paddle_pong_enter,
		.on_exit = paddle_pong_exit,
		.on_update = paddle_pong_update,
	};

	StateHandler death_state = {
		.on_enter = paddle_death_enter,
		.on_exit = paddle_death_exit,
		.on_update = paddle_death_update,
	};

	fsm_state_add(&encounter->state_machine, PADDLE_STATE_ENTERANCE, &enterance_state);
	fsm_state_add(&encounter->state_machine, PADDLE_STATE_PONG, &pong_state);
	fsm_state_add(&encounter->state_machine, PADDLE_STATE_DEATH, &death_state);
	fsm_context_set(&encounter->state_machine, encounter);
	fsm_state_set(&encounter->state_machine, PADDLE_STATE_ENTERANCE);

	return true;
}

void boss_encounter_paddle_update(PaddleEncounter *encounter, Vector2 player_position, float dt) {
	encounter->player_position = player_position;
	fsm_update(&encounter->state_machine, dt);

	for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); ++paddle_index) {
		Paddle *boss = &encounter->paddles[paddle_index];
		entity_sync_collision(&boss->entity);
	}
}

void boss_encounter_paddle_draw(PaddleEncounter *encounter) {
	for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); ++paddle_index) {
		Paddle *boss = &encounter->paddles[paddle_index];
		entity_draw(&boss->entity);

		if (encounter->game_context->show_debug) {
			DrawRectangleLinesEx(boss->entity.collision_shape, 1.f, RED);

			StateID current_state = fsm_state_get(&encounter->state_machine);

			const char *state = stringify_state[current_state];
			Vector2 position = {
				.x = boss->entity.position.x - (MeasureText(state, 32) * .5f),
				.y = boss->entity.position.y - (boss->entity.size.y * .5f) - 50.f,
			};
			DrawText(stringify_state[current_state], position.x, position.y, 32, WHITE);
		}
	}

	ScenarioConfig *scenario = &encounter->active_scenario;

	if (scenario->type == SCENARIO_TYPE_ENTERANCE) {
		float flash = (sinf(encounter->active_scenario.timer * 15.f) + 1.0f) * 0.5f;

		Color color = Fade(RED, flash * 0.5f);

		float w_size = encounter->game_context->tile_size * 2.0f;

		DrawRectangle(0, 0, w_size, GetScreenHeight(), color);
		DrawRectangle(GetScreenWidth() - w_size, 0, w_size, GetScreenHeight(), color);

		float radius = encounter->ball.radius;
		float t = encounter->active_scenario.timer / encounter->active_scenario.duration;

		DrawCircleV(encounter->ball.position, radius, Fade(RED, t * 0.5f));
		DrawCircleLines(encounter->ball.position.x, encounter->ball.position.y, radius, RED);

		float ring_size = 100.0f - (60.0f * t);
		DrawRing(encounter->ball.position, ring_size - 2, ring_size, 0, 360, 0, RED);
	}

	if (encounter->ball.active)
		DrawCircleV(encounter->ball.position, encounter->ball.radius, RED);
}

void boss_paddle_apply_damage(Paddle *boss, float damage) {
	boss->flash_timer = 0.1f;
	boss->health -= damage;
	if (boss->health <= 0.0f) {
		boss->entity.active = false;
		audio_sfx_play(SFX_PADDLE_DEATH, 1.0f, true);
	} else {
		audio_sfx_play(SFX_PADDLE_HURT, 1.0f, true);
	}
}

float boss_encounter_paddle_health_ratio(PaddleEncounter *encounter) {
	float max_health = 0.0f;
	float health = 0.0f;

	for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); ++paddle_index) {
		Paddle *paddle = &encounter->paddles[paddle_index];

		max_health += paddle->max_health;
		health += paddle->health;
	}

	return health / max_health;
}

bool32 boss_encounter_paddle_check_collision(PaddleEncounter *encounter, Entity *player) {
	for (int i = 0; i < 2; i++) {
		Paddle *paddle = &encounter->paddles[i];
		if (paddle->entity.active) {
			entity_sync_collision(&paddle->entity);
			if (CheckCollisionRecs(player->collision_shape, paddle->entity.collision_shape))
				return true;
		}
	}

	if (encounter->ball.active) {
		if (CheckCollisionCircleRec(encounter->ball.position, encounter->ball.radius, player->collision_shape)) {
			return true;
		}
	}
	return false;
}

void paddle_enterance_enter(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	encounter->paddles[0].entity.position = (Vector2){ -200, GetScreenHeight() / 2.f };
	encounter->paddles[0].entity.collision_shape.width = 0;
	encounter->paddles[0].entity.collision_shape.height = 0;
	encounter->paddles[1].entity.position = (Vector2){ GetScreenWidth() + 200, GetScreenHeight() / 2.f };
	encounter->paddles[1].entity.collision_shape.width = 0;
	encounter->paddles[1].entity.collision_shape.height = 0;

	encounter->active_scenario = (ScenarioConfig){
		.type = SCENARIO_TYPE_ENTERANCE,
		.timer = 0.0f,
		.duration = 2.5f
	};

	audio_sfx_play(SFX_BOSS_SIREN, 0.8f, false);

	encounter->ball.active = false;
	encounter->ball.position = (Vector2){ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
	encounter->ball.radius = 40.f;
	LOG_INFO("Entering state intro");
}

StateID paddle_enterance_update(void *context, float dt) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	encounter->active_scenario.timer += dt;
	LOG_INFO("scenario timer = %.2f", encounter->active_scenario.timer);

	float half_time = encounter->active_scenario.duration * .5f;
	if (encounter->active_scenario.timer > half_time) {
		float t = (encounter->active_scenario.timer - half_time) / half_time;
		t = t > 1.0f ? 1.0f : t;
		float ease = 1.0f - powf(1.0f - t, 3.0f);

		encounter->paddles[0].entity.position.x = -200 + (300 * ease);
		encounter->paddles[1].entity.position.x = (GetScreenWidth() + 200) - (300 * ease);
	}

	if (encounter->active_scenario.timer >= encounter->active_scenario.duration)
		return PADDLE_STATE_PONG;

	return STATE_CHANGE_NONE;
}

void paddle_enterance_exit(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	encounter->paddles[0].entity.collision_shape.width = encounter->game_context->tile_size * 2;
	encounter->paddles[0].entity.collision_shape.height = encounter->game_context->tile_size * 8;
	encounter->paddles[1].entity.collision_shape.width = encounter->game_context->tile_size * 2;
	encounter->paddles[1].entity.collision_shape.height = encounter->game_context->tile_size * 8;

	encounter->active_scenario = (ScenarioConfig){ 0 };
}

void paddle_pong_enter(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	encounter->ball.velocity = (Vector2){ -BALL_SPEED_INITIAL * .5f, BALL_SPEED_INITIAL * .5f };
	encounter->ball.active = true;
}

StateID paddle_pong_update(void *context, float dt) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	if (encounter->ball.active) {
		encounter->ball.position.x += encounter->ball.velocity.x * dt;
		encounter->ball.position.y += encounter->ball.velocity.y * dt;

		if (encounter->ball.position.y - encounter->ball.radius < 0) {
			encounter->ball.position.y = encounter->ball.radius;
			encounter->ball.velocity.y *= -1;
		} else if (encounter->ball.position.y + encounter->ball.radius > GetScreenHeight()) {
			encounter->ball.position.y = GetScreenHeight() - encounter->ball.radius;
			encounter->ball.velocity.y *= -1;
		}

		if (encounter->ball.position.x < -100 || encounter->ball.position.x > GetScreenWidth() + 100) {
			encounter->ball.position = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
			float dir = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;
			encounter->ball.velocity = (Vector2){ BALL_SPEED_INITIAL * dir, 0 };
		}
	}

	for (int i = 0; i < 2; i++) {
		Paddle *p = &encounter->paddles[i];

		if (!p->entity.active)
			continue;

		float aim_offset = encounter->player_position.y > p->entity.position.y ? -p->entity.size.y * .33f : p->entity.size.y * .33f;
		float target_y = encounter->ball.position.y + aim_offset;
		float dist = target_y - p->entity.position.y;

		// Move towards ball, capped by max speed
		float move = 0.0f;
		if (fabsf(dist) > 10.0f) { // Deadzone to prevent jitter
			float dir = (dist > 0) ? 1.0f : -1.0f;
			move = dir * PADDLE_MOVE_SPEED * dt;

			// Don't overshoot
			if (fabsf(move) > fabsf(dist))
				move = dist;
		}

		p->entity.position.y += move;

		// Clamp Paddle to Screen
		float half_h = p->entity.size.y * 0.5f;
		if (p->entity.position.y < half_h)
			p->entity.position.y = half_h;
		if (p->entity.position.y > GetScreenHeight() - half_h)
			p->entity.position.y = GetScreenHeight() - half_h;

		// B. BALL COLLISION
		// Sync collision shape explicitly here so we can check immediately
		entity_sync_collision(&p->entity);

		if (CheckCollisionCircleRec(encounter->ball.position, encounter->ball.radius, p->entity.collision_shape)) {
			// 1. Determine direction (Left paddle hits right, Right paddle hits left)
			float dir_x = (i == 0) ? 1.0f : -1.0f;

			// Only resolve if ball is actually moving towards the paddle (prevents sticking)
			bool moving_towards = (i == 0 && encounter->ball.velocity.x < 0) || (i == 1 && encounter->ball.velocity.x > 0);

			if (moving_towards) {
				// 2. Calculate "English" (Reflection Angle)
				// Hit Center = 0, Hit Top = -1, Hit Bottom = 1
				float offset_y = (encounter->ball.position.y - p->entity.position.y) / half_h;
				offset_y = clamp(offset_y, -1.0f, 1.0f);

				// 3. Set new Velocity
				float current_speed = Vector2Length(encounter->ball.velocity);

				// Speed up slightly on every hit (Intensity!)
				current_speed = fminf(current_speed + 50.0f, BALL_SPEED_MAX);

				// Basic vector math:
				// X is mostly constant speed, Y is determined by where we hit the paddle
				encounter->ball.velocity.x = dir_x * current_speed * 0.8f; // Most energy in X
				encounter->ball.velocity.y = offset_y * current_speed * 0.6f; // Some energy in Y

				// Re-normalize to ensure consistent speed
				encounter->ball.velocity = Vector2Scale(Vector2Normalize(encounter->ball.velocity), current_speed);

				// 4. Effects
				// audio_sfx_play(SFX_PADDLE_ENTER, 1.0f, true); // Or a specific bounce sound
				// Optional: Flash the paddle white briefly?
			}
		}
	}

	return STATE_CHANGE_NONE;
}

void paddle_pong_exit(void *context) {}

void paddle_death_enter(void *context) {}
StateID paddle_death_update(void *context, float dt) {
	return STATE_CHANGE_NONE;
}
void paddle_death_exit(void *context) {}
