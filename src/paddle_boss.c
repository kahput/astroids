#include "paddle_boss.h"

#include "audio_manager.h"
#include "common.h"
#include "core/logger.h"
#include "entity.h"
#include "fsm.h"
#include "globals.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#define BALL_SPEED_INITIAL 500.0f
#define BALL_SPEED_MAX 700.0f
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

enum PaddleState {
	PADDLE_STATE_ENTERANCE,
	PADDLE_STATE_PONG,
	PADDLE_STATE_DEATH,

	PADDLE_STATE_COUNT,
};

static const char *stringify_state[PADDLE_STATE_COUNT] = {
	[PADDLE_STATE_ENTERANCE] = "STATE_INTRO",
	[PADDLE_STATE_PONG] = "STATE_PONG",
	[PADDLE_STATE_DEATH] = "STATE_DEATH",
};

bool32 boss_encounter_paddle_initialize(PaddleEncounter *encounter, Texture *texture) {
	*encounter = (PaddleEncounter){ 0 };

	float max_health = 100.f;

	for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); ++paddle_index) {
		Paddle *paddle = &encounter->paddles[paddle_index];

		paddle->entity.active = true;
		paddle->entity.size = (Vector2){ TILE_SIZE * 2, TILE_SIZE * 8 };

		paddle->entity.velocity = (Vector2){ 0, 0 };
		paddle->entity.tint = WHITE;

		paddle->entity.area = (Rectangle){ (TILE_SIZE * 3) * paddle_index, 0, TILE_SIZE, TILE_SIZE * 4 };
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
		Paddle *paddle = &encounter->paddles[paddle_index];

		paddle->animation_timer += dt;
		uint32_t total_frames = paddle->entity.area.y == 0 ? 3 : 5;
		if (paddle->animation_timer >= ANIMATION_SPEED) {
			paddle->animation_frame = (paddle->animation_frame + 1) % total_frames;
			paddle->animation_timer = 0.0f;
		}

		uint32_t offset = 0;
		if (paddle->entity.area.y == 0)
			offset = paddle_index ? TILE_SIZE * 3 : 0;

		paddle->entity.area.x = offset + (paddle->animation_frame * TILE_SIZE);

		entity_sync_collision(&paddle->entity);
	}
}

void boss_encounter_paddle_draw(PaddleEncounter *encounter, bool32 show_debug) {
	for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); ++paddle_index) {
		Paddle *paddle = &encounter->paddles[paddle_index];
		if (paddle->entity.active == false)
			continue;

		entity_draw(&paddle->entity);
		if (show_debug) {
			DrawLineV(encounter->ball.position, encounter->player_position, YELLOW);
			DrawCircle(paddle->entity.position.x, paddle->target_y, 5, GREEN);

			DrawCircleV(paddle->entity.position, 3.f, GREEN);
			DrawRectangleLinesEx(paddle->entity.collision_shape, 1.f, RED);

			StateID current_state = fsm_state_get(&encounter->state_machine);

			const char *state = stringify_state[current_state];
			Vector2 position = {
				.x = paddle->entity.position.x - (MeasureText(state, 32) * .5f),
				.y = paddle->entity.position.y - (paddle->entity.size.y * .5f) - 50.f,
			};
			DrawText(stringify_state[current_state], position.x, position.y, 32, WHITE);
		}
	}

	ScenarioConfig *scenario = &encounter->active_scenario;

	if (scenario->type == SCENARIO_TYPE_ENTERANCE) {
		float flash = (sinf(encounter->active_scenario.timer * 15.f) + 1.0f) * 0.5f;

		Color color = Fade(RED, flash * 0.5f);

		float w_size = TILE_SIZE * 2.0f;

		DrawRectangle(0, 0, w_size, GetScreenHeight(), color);
		DrawRectangle(GetScreenWidth() - w_size, 0, w_size, GetScreenHeight(), color);

		float radius = encounter->ball.radius;
		float t = encounter->active_scenario.timer / encounter->active_scenario.duration;

		DrawCircleV(encounter->ball.position, radius, Fade(RED, t * 0.5f));
		DrawCircleLines(encounter->ball.position.x, encounter->ball.position.y, radius, RED);

		float ring_size = 100.0f - (60.0f * t);
		DrawRing(encounter->ball.position, ring_size - 2, ring_size, 0, 360, 0, RED);
	}

	if (encounter->ball.active) {
		DrawCircleV(encounter->ball.position, encounter->ball.radius, RED);

		DrawCircleV(encounter->ball.position, 3.f, GREEN);
	}
}

void boss_paddle_apply_damage(Paddle *boss, float damage) {
	boss->flash_timer = 0.1f;
	boss->health -= damage;

	if (boss->health <= 0.0f) {
		boss->entity.active = false;
		audio_sfx_play(SFX_PADDLE_DEATH, 1.0f, true);
	} else {
		if (boss->health <= boss->max_health * .5f && boss->entity.area.y == 0) {
			boss->entity.area.y += TILE_SIZE * 4;
		}

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

	audio_sfx_play(SFX_BOSS_WARNING, 0.8f, false);

	encounter->ball.active = false;
	encounter->ball.position = (Vector2){ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
	encounter->ball.radius = 40.f;
	LOG_INFO("Entering state intro");
}

StateID paddle_enterance_update(void *context, float dt) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	encounter->active_scenario.timer += dt;
	// LOG_INFO("scenario timer = %.2f", encounter->active_scenario.timer);

	if (encounter->active_scenario.timer >= 1.0f)
		audio_music_play(MUSIC_BOSS_PADDLE);

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

	encounter->paddles[0].entity.collision_shape.width = TILE_SIZE * 2;
	encounter->paddles[0].entity.collision_shape.height = TILE_SIZE * 8;
	encounter->paddles[1].entity.collision_shape.width = TILE_SIZE * 2;
	encounter->paddles[1].entity.collision_shape.height = TILE_SIZE * 8;

	encounter->active_scenario = (ScenarioConfig){ 0 };
}

void paddle_pong_enter(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	float x_sign = encounter->player_position.x > encounter->ball.position.x ? .5f : -.5f;
	float y_sign = encounter->player_position.y > encounter->ball.position.y ? .5f : -.5f;
	encounter->ball.velocity = (Vector2){ BALL_SPEED_INITIAL * x_sign, BALL_SPEED_INITIAL * y_sign };
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

	for (int paddle_index = 0; paddle_index < 2; paddle_index++) {
		Paddle *paddle = &encounter->paddles[paddle_index];

		if (!paddle->entity.active)
			continue;

		float direction_x = (paddle_index == 0) ? 1.0f : -1.0f;
		float half_h = paddle->entity.size.y * 0.5f;

		Vector2 desired_bounce_direction = Vector2Normalize(Vector2Subtract(encounter->player_position, encounter->ball.position));
		bool32 player_on_correct_side =
			(paddle_index == 0 && desired_bounce_direction.x > 0) ||
			(paddle_index == 1 && desired_bounce_direction.x < 0);
		bool32 direction_valid = fabsf(desired_bounce_direction.x) > 0.01f;

		float target_paddle_y;
		if (player_on_correct_side && direction_valid) {
			float needed_offset_y = (direction_x * desired_bounce_direction.y) / desired_bounce_direction.x;

			needed_offset_y = Clamp(needed_offset_y, -1.0f, 1.0f);

			target_paddle_y = encounter->ball.position.y - (needed_offset_y * half_h);
		} else
			target_paddle_y = encounter->ball.position.y;

		paddle->target_y = target_paddle_y;
		float dist = target_paddle_y - paddle->entity.position.y;
		float move = 0.0f;

		if (fabsf(dist) > 10.0f) {
			float dir = (dist > 0) ? 1.0f : -1.0f;
			move = dir * PADDLE_MOVE_SPEED * dt;

			if (fabsf(move) > fabsf(dist))
				move = dist;
		}

		paddle->entity.position.y += move;

		if (paddle->entity.position.y < half_h)
			paddle->entity.position.y = half_h;
		if (paddle->entity.position.y > GetScreenHeight() - half_h)
			paddle->entity.position.y = GetScreenHeight() - half_h;

		entity_sync_collision(&paddle->entity);

		if (CheckCollisionCircleRec(encounter->ball.position, encounter->ball.radius, paddle->entity.collision_shape)) {
			bool moving_towards = (paddle_index == 0 && encounter->ball.velocity.x < 0) || (paddle_index == 1 && encounter->ball.velocity.x > 0);

			if (moving_towards) {
				float offset_y = (encounter->ball.position.y - paddle->entity.position.y) / half_h;
				const char *side[2] = { "left", "right" };
				LOG_INFO("Paddle_%s.hit_offset_y = %.2f", side[paddle_index], offset_y);
				offset_y = clamp(offset_y, -1.0f, 1.0f);

				float current_speed = Vector2Length(encounter->ball.velocity);

				current_speed = fminf(current_speed + 100.f, BALL_SPEED_MAX);

				encounter->ball.velocity.x = direction_x;
				encounter->ball.velocity.y = offset_y;

				encounter->ball.velocity = Vector2Scale(Vector2Normalize(encounter->ball.velocity), current_speed);

				// audio_sfx_play(SFX_PADDLE_ENTER, 1.0f, true);
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
