#include "pong_boss.h"

#include "audio_manager.h"
#include "common.h"
#include "core/logger.h"
#include "entity.h"
#include "fsm.h"
#include "globals.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>

void paddle_pong_entry_enter(void *context);
StateID paddle_pong_entry_update(void *context, float dt);
void paddle_pong_entry_exit(void *context);

void paddle_pong_enter(void *context);
StateID paddle_pong_update(void *context, float dt);
void paddle_pong_exit(void *context);

void paddle_breakout_entry_enter(void *context);
StateID paddle_breakout_entry_update(void *context, float dt);
void paddle_breakout_entry_exit(void *context);

void paddle_breakout_enter(void *context);
StateID paddle_breakout_update(void *context, float dt);
void paddle_breakout_exit(void *context);

void paddle_death_enter(void *context);
StateID paddle_death_update(void *context, float dt);
void paddle_death_exit(void *context);

enum PaddleState {
	PADDLE_STATE_PONG_ENTRY,
	PADDLE_STATE_PONG,
	PADDLE_STATE_BREAKOUT_ENTRY,
	PADDLE_STATE_BREAKOUT,
	PADDLE_STATE_DEATH,

	PADDLE_STATE_COUNT,
};

static const char *stringify_state[PADDLE_STATE_COUNT] = {
	[PADDLE_STATE_PONG_ENTRY] = "STATE_PONG_ENTRY",
	[PADDLE_STATE_PONG] = "STATE_PONG",
	[PADDLE_STATE_BREAKOUT_ENTRY] = "STATE_BREAKOUT_ENTRY",
	[PADDLE_STATE_BREAKOUT] = "STATE_BREAKOUT",
	[PADDLE_STATE_DEATH] = "STATE_DEATH",
};

bool32 boss_encounter_paddle_initialize(PaddleEncounter *encounter, Texture *texture) {
	*encounter = (PaddleEncounter){ 0 };
	encounter->paddle_texture = texture;
	encounter->max_health = 200.f;
	encounter->health = encounter->max_health;

	for (uint32_t paddle_index = 0; paddle_index < 2; ++paddle_index) {
		Paddle *paddle = &encounter->paddles[paddle_index];

		paddle->entity.active = true;
		paddle->entity.size = (Vector2){ TILE_SIZE * 2, TILE_SIZE * 8 };

		paddle->entity.velocity = (Vector2){ 0, 0 };
		paddle->entity.tint = WHITE;

		paddle->entity.area = (Rectangle){ (TILE_SIZE * 3) * paddle_index, 0, TILE_SIZE, TILE_SIZE * 4 };
		paddle->entity.texture = texture;

		paddle->entity.collision_active = true;
		paddle->entity.collision_shape = (Rectangle){ 0, 0, paddle->entity.size.x, paddle->entity.size.y };

		// paddle->max_health = max_health * .5f;
		// paddle->health = paddle->max_health;
		paddle->flash_timer = 0.0f;
	}

	StateHandler pong_entry_state = {
		.on_enter = paddle_pong_entry_enter,
		.on_exit = paddle_pong_entry_exit,
		.on_update = paddle_pong_entry_update,
	};

	StateHandler pong_state = {
		.on_enter = paddle_pong_enter,
		.on_exit = paddle_pong_exit,
		.on_update = paddle_pong_update,
	};

	StateHandler breakout_entry_state = {
		.on_enter = paddle_breakout_entry_enter,
		.on_exit = paddle_breakout_entry_exit,
		.on_update = paddle_breakout_entry_update,
	};

	StateHandler breakout_state = {
		.on_enter = paddle_breakout_enter,
		.on_exit = paddle_breakout_exit,
		.on_update = paddle_breakout_update,
	};

	StateHandler death_state = {
		.on_enter = paddle_death_enter,
		.on_exit = paddle_death_exit,
		.on_update = paddle_death_update,
	};

	fsm_state_add(&encounter->state_machine, PADDLE_STATE_PONG_ENTRY, &pong_entry_state);
	fsm_state_add(&encounter->state_machine, PADDLE_STATE_PONG, &pong_state);
	fsm_state_add(&encounter->state_machine, PADDLE_STATE_BREAKOUT_ENTRY, &breakout_entry_state);
	fsm_state_add(&encounter->state_machine, PADDLE_STATE_BREAKOUT, &breakout_state);
	fsm_state_add(&encounter->state_machine, PADDLE_STATE_DEATH, &death_state);
	fsm_context_set(&encounter->state_machine, encounter);
	fsm_state_set(&encounter->state_machine, PADDLE_STATE_PONG_ENTRY);

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
	for (uint32_t brick_index = 0; brick_index < MAX_BRICKS; ++brick_index) {
		Entity *brick = &encounter->bricks[brick_index];
		if (brick->active == false)
			continue;

		entity_draw(brick);

		if (show_debug) {
			if (brick->collision_active)
				DrawRectangleLinesEx(brick->collision_shape, 1.0f, GREEN);
		}
	}

	for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); ++paddle_index) {
		Paddle *paddle = &encounter->paddles[paddle_index];
		if (paddle->entity.active == false)
			continue;

		entity_draw(&paddle->entity);
		if (show_debug) {
			DrawLineV(encounter->balls[0].position, encounter->player_position, YELLOW);
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

	for (uint32_t projectile_index = 0; projectile_index < BRICK_MAX_PROJECTILES; ++projectile_index) {
		Entity *projectile = &encounter->projectiles[projectile_index];

		if (projectile->active) {
			DrawCircleV(projectile->position, 6.0f, ORANGE);
			DrawCircleV(projectile->position, 3.0f, YELLOW);

			if (show_debug && projectile->collision_active)
				DrawRectangleLinesEx(projectile->collision_shape, 1.0f, GREEN);
		}
	}

	ScenarioConfig *scenario = &encounter->active_scenario;

	if ((scenario->type & SCENARIO_FLAG_WARNING) == SCENARIO_FLAG_WARNING) {
		float flash = (sinf(encounter->active_scenario.timer * 15.f) + 1.0f) * 0.5f;
		Color color = Fade(RED, flash * 0.5f);

		float w_size = TILE_SIZE * 2.0f;

		for (uint32_t side_index = 0; side_index < countof(scenario->warnings); ++side_index) {
			Rectangle rect = scenario->warnings[side_index];
			DrawRectangleRec(rect, color);
		}
	}

	if ((scenario->type & SCENARIO_FLAG_PONG_ENTER) == SCENARIO_FLAG_PONG_ENTER) {
		// NOTE: Is this flag needed?
	}

	for (uint32_t ball_index = 0; ball_index < countof(encounter->balls); ++ball_index) {
		Ball *ball = &encounter->balls[ball_index];
		if (ball->radius == 0.0f)
			continue;
		;

		if (ball->active) {
			Rectangle source = { TILE_SIZE * 8, TILE_SIZE, TILE_SIZE + 16, TILE_SIZE + 16 };



			Rectangle dest = { .x = ball->position.x, .y = ball->position.y, 100.f, 100.f};
			DrawTexturePro(*encounter->paddle_texture, source, dest, (Vector2){50, 50}, 0, WHITE);
			// DrawCircleV(ball->position, ball->radius, RED);
			//
			// DrawCircleV(ball->position, 3.f, GREEN);

            if (show_debug) {
                DrawCircleLinesV(ball->position, ball->radius, GREEN);
            }
		}

		if ((scenario->type & SCENARIO_FLAG_BALL_ENTER) == SCENARIO_FLAG_BALL_ENTER) {
			float radius = ball->radius;
			float t = encounter->active_scenario.timer / encounter->active_scenario.duration;

			DrawCircleV(ball->position, radius, Fade(RED, t * 0.5f));
			DrawCircleLines(ball->position.x, ball->position.y, radius, RED);

			float ring_size = 100.0f - (60.0f * t);
			DrawRing(ball->position, ring_size - 2, ring_size, 0, 360, 0, RED);
		}
	}
}

void boss_paddle_apply_damage(PaddleEncounter *encounter, uint32_t paddle_index, float damage) {
	encounter->health -= damage;
	Paddle *boss = &encounter->paddles[paddle_index];

	boss->flash_timer = 0.1f;
	if (encounter->health <= 0.0f) {
		fsm_state_set(&encounter->state_machine, PADDLE_STATE_DEATH);
	} else {
		if (encounter->health <= encounter->max_health * .5f && boss->entity.area.y == 0) {
			boss->entity.area.y += TILE_SIZE * 4;
		}

		audio_sfx_play(SFX_PADDLE_HURT, 1.0f, true);
	}
}

float boss_encounter_paddle_health_ratio(PaddleEncounter *encounter) {
	float max_health = 0.0f;
	float health = 0.0f;

	// for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); ++paddle_index) {
	// 	Paddle *paddle = &encounter->paddles[paddle_index];
	//
	// 	max_health += paddle->max_health;
	// 	health += paddle->health;
	// }

	return encounter->health / encounter->max_health;
}

bool32 boss_encounter_paddle_check_collision(PaddleEncounter *encounter, Entity *player) {
	for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); paddle_index++) {
		Paddle *paddle = &encounter->paddles[paddle_index];

		if (paddle->entity.active) {
			entity_sync_collision(&paddle->entity);
			if (CheckCollisionRecs(player->collision_shape, paddle->entity.collision_shape))
				return true;
		}
	}

	for (uint32_t brick_index = 0; brick_index < MAX_BRICKS; ++brick_index) {
		Entity *brick = &encounter->bricks[brick_index];
		if (brick->active == false || brick->collision_active == false)
			continue;

		entity_sync_collision(player);
		if (CheckCollisionRecs(player->collision_shape, brick->collision_shape)) {
			float dx = player->position.x - brick->position.x;
			float dy = player->position.y - brick->position.y;

			float width_sum = (player->collision_shape.width * .5f) + (brick->collision_shape.width * .5f);
			float height_sum = (player->collision_shape.height * .5f) + (brick->collision_shape.height * .5f);

			float overlap_x = width_sum - fabsf(dx);
			float overlap_y = height_sum - fabsf(dy);

			if (overlap_x < overlap_y) {
				float sign_x = (dx > 0) ? 1.0f : -1.0f;
				player->position.x += (overlap_x)*sign_x;

				if ((sign_x > 0 && player->velocity.x < 0) || (sign_x < 0 && player->velocity.x > 0))
					player->velocity.x = 0.0f;
			} else {
				float sign_y = (dy > 0) ? 1.0f : -1.0f;
				player->position.y += (overlap_y)*sign_y;

				if ((sign_y > 0 && player->velocity.y < 0) || (sign_y < 0 && player->velocity.y > 0))
					player->velocity.y = 0.0f;
			}

			LOG_INFO("Player should be updated?");
			entity_sync_collision(player);
		}
	}

	for (uint32_t ball_index = 0; ball_index < countof(encounter->balls); ++ball_index) {
		Ball *ball = &encounter->balls[ball_index];

		if (ball->active)
			if (CheckCollisionCircleRec(ball->position, ball->radius, player->collision_shape))
				return true;
	}

	for (int projectile_index = 0; projectile_index < BRICK_MAX_PROJECTILES; projectile_index++) {
		Entity *projectile = &encounter->projectiles[projectile_index];
		if (projectile->active) {
			if (CheckCollisionRecs(player->collision_shape, projectile->collision_shape)) {
				return true;
			}
		}
	}
	return false;
}

void paddle_pong_entry_enter(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	encounter->paddles[0].entity.position = (Vector2){ -200, GetScreenHeight() / 2.f };
	encounter->paddles[0].entity.collision_active = false;

	encounter->paddles[1].entity.position = (Vector2){ GetScreenWidth() + 200, GetScreenHeight() / 2.f };
	encounter->paddles[1].entity.collision_active = false;

	float w_size = TILE_SIZE * 2;
	encounter->active_scenario = (ScenarioConfig){
		.type = SCENARIO_FLAG_WARNING | SCENARIO_FLAG_BALL_ENTER,
		.timer = 0.0f,
		.duration = 2.5f,
		.warnings = {
		  [0] = { 0, 0, w_size, GetScreenHeight() },
		  [1] = { GetScreenWidth() - w_size, 0, w_size, GetScreenHeight() } },
	};

	audio_sfx_play(SFX_BOSS_WARNING, 0.8f, false);

	for (uint32_t ball_index = 0; ball_index < countof(encounter->balls); ++ball_index) {
		Ball *ball = &encounter->balls[ball_index];
		encounter->balls[0].active = false;
		encounter->balls[0].position = (Vector2){ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
		encounter->balls[0].radius = 35.f;
	}

	LOG_INFO("Entering state intro");
}

StateID paddle_pong_entry_update(void *context, float dt) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	encounter->active_scenario.timer += dt;
	// LOG_INFO("scenario timer = %.2f", encounter->active_scenario.timer);

	if (encounter->active_scenario.timer >= 1.0f)
		audio_music_play(MUSIC_BOSS_PONG);

	float half_time = encounter->active_scenario.duration * .5f;
	if (encounter->active_scenario.timer > half_time) {
		float t = (encounter->active_scenario.timer - half_time) / half_time;
		t = t > 1.0f ? 1.0f : t;
		float ease = 1.0f - powf(1.0f - t, 3.0f);

		encounter->paddles[0].entity.position.x = -200 + (300 * ease);
		encounter->paddles[1].entity.position.x = (GetScreenWidth() + 200) - (300 * ease);
	}

	if (encounter->active_scenario.timer >= 2.f) {
		encounter->active_scenario.warnings[0] = (Rectangle){ 0 };
		encounter->active_scenario.warnings[1] = (Rectangle){ 0 };
	}

	if (encounter->active_scenario.timer >= encounter->active_scenario.duration)
		return PADDLE_STATE_PONG;

	return STATE_CHANGE_NONE;
}

void paddle_pong_entry_exit(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	encounter->paddles[0].entity.collision_active = true;
	encounter->paddles[1].entity.collision_active = true;

	encounter->active_scenario = (ScenarioConfig){ 0 };
}

void paddle_pong_enter(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	float x_sign = encounter->player_position.x > encounter->balls[0].position.x ? .5f : -.5f;
	float y_sign = encounter->player_position.y > encounter->balls[0].position.y ? .5f : -.5f;
	encounter->balls[0].velocity = (Vector2){ BALL_SPEED_INITIAL * x_sign, BALL_SPEED_INITIAL * y_sign };
	encounter->balls[0].active = true;
}

StateID paddle_pong_update(void *context, float dt) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	if (encounter->balls[0].active) {
		encounter->balls[0].position.x += encounter->balls[0].velocity.x * dt;
		encounter->balls[0].position.y += encounter->balls[0].velocity.y * dt;

		if (encounter->balls[0].position.y - encounter->balls[0].radius < 0) {
			encounter->balls[0].position.y = encounter->balls[0].radius;
			encounter->balls[0].velocity.y *= -1;
		} else if (encounter->balls[0].position.y + encounter->balls[0].radius > GetScreenHeight()) {
			encounter->balls[0].position.y = GetScreenHeight() - encounter->balls[0].radius;
			encounter->balls[0].velocity.y *= -1;
		}

		if (encounter->balls[0].position.x < -100 || encounter->balls[0].position.x > GetScreenWidth() + 100) {
			encounter->balls[0].position = (Vector2){ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
			float dir = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;
			encounter->balls[0].velocity = (Vector2){ BALL_SPEED_INITIAL * dir, 0 };
		}
	}

	for (int paddle_index = 0; paddle_index < 2; paddle_index++) {
		Paddle *paddle = &encounter->paddles[paddle_index];

		if (!paddle->entity.active)
			return PADDLE_STATE_BREAKOUT_ENTRY;

		float direction_x = (paddle_index == 0) ? 1.0f : -1.0f;
		float half_h = paddle->entity.size.y * 0.5f;

		Vector2 desired_bounce_direction = Vector2Normalize(Vector2Subtract(encounter->player_position, encounter->balls[0].position));
		bool32 player_on_correct_side =
			(paddle_index == 0 && desired_bounce_direction.x > 0) ||
			(paddle_index == 1 && desired_bounce_direction.x < 0);
		bool32 direction_valid = fabsf(desired_bounce_direction.x) > 0.01f;

		float target_paddle_y;
		if (player_on_correct_side && direction_valid) {
			float needed_offset_y = (direction_x * desired_bounce_direction.y) / desired_bounce_direction.x;

			needed_offset_y = Clamp(needed_offset_y, -1.0f, 1.0f);

			target_paddle_y = encounter->balls[0].position.y - (needed_offset_y * half_h);
		} else
			target_paddle_y = encounter->balls[0].position.y;

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

		if (CheckCollisionCircleRec(encounter->balls[0].position, encounter->balls[0].radius, paddle->entity.collision_shape)) {
			bool moving_towards = (paddle_index == 0 && encounter->balls[0].velocity.x < 0) || (paddle_index == 1 && encounter->balls[0].velocity.x > 0);
			// audio_sfx_play(SFX_PADDLE_HIT, 1.0f, true);

			if (moving_towards) {
				float offset_y = (encounter->balls[0].position.y - paddle->entity.position.y) / half_h;
				const char *side[2] = { "left", "right" };
				LOG_INFO("Paddle_%s.hit_offset_y = %.2f", side[paddle_index], offset_y);
				offset_y = clamp(offset_y, -1.0f, 1.0f);

				float current_speed = Vector2Length(encounter->balls[0].velocity);

				current_speed = fminf(current_speed + 100.f, BALL_SPEED_MAX);

				encounter->balls[0].velocity.x = direction_x;
				encounter->balls[0].velocity.y = offset_y;

				encounter->balls[0].velocity = Vector2Scale(Vector2Normalize(encounter->balls[0].velocity), current_speed);

				// audio_sfx_play(SFX_PADDLE_ENTER, 1.0f, true);
				// Optional: Flash the paddle white briefly?
			}
		}
	}

	return STATE_CHANGE_NONE;
}

void paddle_pong_exit(void *context) {}

void paddle_breakout_entry_enter(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	audio_music_stop(MUSIC_BOSS_PONG);
	encounter->survivor = encounter->paddles[0].entity.active ? &encounter->paddles[0] : &encounter->paddles[1];
	encounter->survivor->entity.collision_active = false;
	encounter->active_scenario = (ScenarioConfig){
		.type = SCENARIO_FLAG_WARNING,
		.base_position = encounter->survivor->entity.position,
		.timer = 0.0f,
		.duration = SPLIT_DURATION_SHAKE + SPLIT_DURATION_ROTATE + SPLIT_DURATION_EXIT + SPLIT_DURATION_WARN + SPLIT_DURATION_ENTRY
	};

	encounter->balls[0].active = false;
	encounter->balls[0].position = (Vector2){ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
}

StateID paddle_breakout_entry_update(void *context, float dt) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	encounter->active_scenario.timer += dt;
	float timer = encounter->active_scenario.timer;

	if (timer >= encounter->active_scenario.duration) {
		for (int brick_index = 0; brick_index < MAX_BRICKS; brick_index++) {
			Entity *brick = &encounter->bricks[brick_index];

			encounter->active_scenario.warnings[0] = (Rectangle){ 0 };

			brick->collision_active = true;
			brick->collision_shape.width = BRICK_WIDTH;
			brick->collision_shape.height = BRICK_HEIGHT;
			entity_sync_collision(brick);
		}

		return PADDLE_STATE_BREAKOUT;
	}

	// Find the survivor
	if (encounter->survivor == NULL)
		return PADDLE_STATE_DEATH;

	Vector2 original_position = encounter->active_scenario.base_position;
	if (timer < SPLIT_DURATION_SHAKE) {
		float shake_t = timer / SPLIT_DURATION_SHAKE;

		float intensity = shake_t * 10.0f;
		float shake_x = (sinf(timer * 40.0f) * intensity);
		float shake_y = (cosf(timer * 35.0f) * intensity);

		encounter->survivor->entity.position = (Vector2){
			original_position.x + shake_x,
			original_position.y + shake_y
		};

		encounter->survivor->entity.tint = ColorLerp(WHITE, RED, shake_t);

		// TODO: Play sound
	}

	else if (timer < SPLIT_DURATION_SHAKE + SPLIT_DURATION_ROTATE) {
		float rotate_t = (timer - SPLIT_DURATION_SHAKE) / SPLIT_DURATION_ROTATE;
		float ease = 1.0f - powf(1.0f - rotate_t, 3.0f); // Ease out

		// Reset to original position
		encounter->survivor->entity.position = original_position;
		encounter->survivor->entity.rotation = ease * 90.0f;

		encounter->survivor->entity.tint = RED;
	}

	else if (timer < SPLIT_DURATION_SHAKE + SPLIT_DURATION_ROTATE + SPLIT_DURATION_EXIT) {
		float exit_t = (timer - SPLIT_DURATION_SHAKE - SPLIT_DURATION_ROTATE) / SPLIT_DURATION_EXIT;
		float ease = powf(exit_t, 2.0f);

		float exit_distance = GetScreenHeight() + encounter->survivor->entity.size.y;
		encounter->survivor->entity.position = (Vector2){
			original_position.x,
			original_position.y - (exit_distance * ease)
		};

		encounter->survivor->entity.rotation = 90.0f;
		encounter->survivor->entity.tint = RED;

	}

	else if (timer < SPLIT_DURATION_SHAKE + SPLIT_DURATION_ROTATE + SPLIT_DURATION_EXIT + SPLIT_DURATION_WARN) {
		audio_sfx_play(SFX_BOSS_WARNING, 1.0f, false);
		float warn_t = (timer - SPLIT_DURATION_SHAKE - SPLIT_DURATION_ROTATE - SPLIT_DURATION_EXIT) / SPLIT_DURATION_WARN;

		if (warn_t >= .5f)
			audio_music_play(MUSIC_BOSS_BREAKOUT);

		float ball_radius = 30.f;
		float ball_spacing = 100.f;

		float total_width = (2 * ball_radius) + ((2 - 1) * ball_spacing);

		float screen_center = GetScreenWidth() / 2.0f;
		float row_start_x = screen_center - (total_width / 2.0f);
		for (uint32_t ball_index = 0; ball_index < countof(encounter->balls); ++ball_index) {
			Ball *ball = &encounter->balls[ball_index];
			ball->radius = ball_radius;

			float target_x = row_start_x + (ball_index * (ball_radius + ball_spacing)) + (ball_radius / 2.0f);
			ball->position.x = target_x;
			ball->position.y = GetScreenHeight() / 2.f;
		}

		encounter->active_scenario.type |= SCENARIO_FLAG_BALL_ENTER;
		float size_h = TILE_SIZE * 2.f;
		encounter->active_scenario.warnings[0] = (Rectangle){ 0, 0, GetScreenWidth(), size_h };
	}

	else if (timer < encounter->active_scenario.duration) {
		float entry_t = (timer - SPLIT_DURATION_SHAKE - SPLIT_DURATION_ROTATE - SPLIT_DURATION_EXIT - SPLIT_DURATION_WARN) / SPLIT_DURATION_ENTRY;
		if (entry_t >= .5f) {
			encounter->active_scenario.warnings[0] = (Rectangle){ 0 };
		}
		float ease = 1.0f - powf(1.0f - entry_t, 3.0f);

		float boss_start_y = -encounter->survivor->entity.size.y * 2.f;
		float boss_target_y = TILE_SIZE * 3.0f;

		encounter->survivor->entity.position.x = GetScreenWidth() * .5f;
		encounter->survivor->entity.position.y = boss_start_y + (boss_target_y - boss_start_y) * ease;

		float gap_size = 125.f;

		float screen_center_y = GetScreenHeight() * .5f;
		float screen_center_x = GetScreenWidth() * .5f;

		float start_y_offset = 40.f;

		for (int brick_index = 0; brick_index < MAX_BRICKS; brick_index++) {
			Entity *brick = &encounter->bricks[brick_index];

			float total_height = 3 * (BRICK_HEIGHT + BRICK_SPACING_Y);
			float total_health = 75.f;

			int32_t row = brick_index / 2;
			uint32_t col = brick_index % 2;
			uint32_t items_in_row = 2;

			float spacing_x = (gap_size * (row * 4));
			spacing_x = spacing_x <= 0.0f ? (gap_size + (BRICK_WIDTH * .5f)) : spacing_x;

			float row_width = (items_in_row * BRICK_WIDTH) + ((items_in_row - 1) * spacing_x);
			float row_start_x = screen_center_x - (row_width / 2.f);

			float target_x = row_start_x + (col * (BRICK_WIDTH + spacing_x)) + (BRICK_WIDTH / 2.f);
			float target_y = screen_center_y + start_y_offset + ((row ? 0 : 1) * (BRICK_HEIGHT + gap_size));

			float start_x = (target_x < screen_center_x) ? -BRICK_WIDTH * 2.f : GetScreenWidth() + (BRICK_WIDTH * 2.f);

			if (brick->active == false) {
				*brick = (Entity){ 0 };
				brick->active = true;

				brick->position = (Vector2){ start_x, target_y };

				brick->size = (Vector2){ BRICK_WIDTH, BRICK_HEIGHT };
				brick->rotation = 0;

				brick->tint = WHITE;
				// brick->area = (Rectangle){ .x = 0, .y = TILE_SIZE * 4, .width = TILE_SIZE, .height = TILE_SIZE * 4 };
				// brick->texture = encounter->paddle_texture;

				brick->collision_active = true;
				brick->collision_shape = (Rectangle){ 0, 0, brick->size.x, brick->size.y };
			}

			brick->position.x = start_x + (target_x - start_x) * ease;
			entity_sync_collision(brick);
		}
	}

	return STATE_CHANGE_NONE;
}
void paddle_breakout_entry_exit(void *context) {}

void paddle_breakout_enter(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;
	encounter->active_scenario = (ScenarioConfig){ 0 };

	float ball_radius = 30.f;
	float ball_spacing = 20.f;

	float total_width = (2 * ball_radius) + ((2 - 1) * ball_spacing);

	float screen_center = GetScreenWidth() / 2.0f;
	float row_start_x = screen_center - (total_width / 2.0f);

	for (uint32_t ball_index = 0; ball_index < countof(encounter->balls); ++ball_index) {
		Ball *ball = &encounter->balls[ball_index];

		float target_x = row_start_x + (ball_index * (ball_radius + ball_spacing)) + (ball_radius / 2.0f);
		ball->position.x = target_x;
		ball->position.y = GetScreenHeight() / 2.f;

		float x_sign = ball_index - .5f;
		float y_sign = -.5f;

		ball->radius = ball_radius;
		ball->velocity = (Vector2){ BALL_SPEED_INITIAL * x_sign, BALL_SPEED_INITIAL * y_sign };
		ball->active = true;
	}

	float width = encounter->survivor->entity.collision_shape.height;
	float height = encounter->survivor->entity.collision_shape.width;
	encounter->survivor->entity.collision_shape = (Rectangle){ 0, 0, width, height };
	entity_sync_collision(&encounter->survivor->entity);
}

StateID paddle_breakout_update(void *context, float dt) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	for (uint32_t ball_index = 0; ball_index < countof(encounter->balls); ++ball_index) {
		Ball *ball = &encounter->balls[ball_index];

		if (ball->active) {
			ball->position.x += ball->velocity.x * dt;
			ball->position.y += ball->velocity.y * dt;

			if (ball->position.x - ball->radius < 0) {
				ball->position.x = ball->radius;
				ball->velocity.x *= -1;
			} else if (ball->position.x + ball->radius > GetScreenWidth()) {
				ball->position.x = GetScreenWidth() - ball->radius;
				ball->velocity.x *= -1;
			}

			if (ball->position.y - ball->radius < 0) {
				ball->position.y = ball->radius;
				ball->velocity.y *= -1;
			} else if (ball->position.y + ball->radius > GetScreenHeight()) {
				ball->position.y = GetScreenHeight() - ball->radius;
				ball->velocity.y *= -1;
			}

			bool hit_this_frame = false;

			for (int brick_index = 0; brick_index < MAX_BRICKS; brick_index++) {
				Entity *brick = &encounter->bricks[brick_index];
				if (!brick->active)
					continue;

				if (CheckCollisionCircleRec(ball->position, ball->radius, brick->collision_shape)) {
					float dx = ball->position.x - brick->position.x;
					float dy = ball->position.y - brick->position.y;

					float abs_dx = fabsf(dx);
					float abs_dy = fabsf(dy);

					float half_w = brick->collision_shape.width * 0.5f;
					float half_h = brick->collision_shape.height * 0.5f;

					if (abs_dx > abs_dy) {
						ball->velocity.x *= -1;
						ball->position.x = brick->position.x +
							(dx > 0 ? half_w + ball->radius : -half_w - ball->radius);
					} else {
						ball->velocity.y *= -1;
						ball->position.y = brick->position.y +
							(dy > 0 ? half_h + ball->radius : -half_h - ball->radius);
					}
				}
			}
		}
	}

	Paddle *survivor = encounter->survivor;

	if (survivor->entity.active) {
		float half_w = survivor->entity.size.x * 0.5f;

		Ball *target = &encounter->balls[0];
		for (uint32_t index = 1; index < countof(encounter->balls); ++index) {
			Ball *ball = &encounter->balls[index];
			if (ball->position.y < target->position.y)
				target = ball;
		}

		Vector2 to_player = Vector2Subtract(encounter->player_position, target->position);
		to_player = Vector2Normalize(to_player);

		float needed_offset_x = to_player.x / to_player.y;
		needed_offset_x = Clamp(needed_offset_x, -1.0f, 1.0f);
		float target_paddle_x = target->position.x - (needed_offset_x * half_w);

		float dist = target_paddle_x - survivor->entity.position.x;
		float move = 0.0f;

		if (fabsf(dist) > 5.0f) {
			float dir = (dist > 0) ? 1.0f : -1.0f;
			move = dir * (PADDLE_MOVE_SPEED * 2.f) * dt;

			if (fabsf(move) > fabsf(dist))
				move = dist;
		}

		survivor->entity.position.x += move;

		if (survivor->entity.position.x < half_w)
			survivor->entity.position.x = half_w;
		if (survivor->entity.position.x > GetScreenWidth() - half_w)
			survivor->entity.position.x = GetScreenWidth() - half_w;

		entity_sync_collision(&survivor->entity);

		if (CheckCollisionCircleRec(target->position, target->radius, survivor->entity.collision_shape)) {
			float offset_x = (target->position.x - survivor->entity.position.x) / half_w;

			LOG_INFO("Survivor.hit_offset_x = %.2f", offset_x);
			offset_x = clamp(offset_x, -1.0f, 1.0f);

			float current_speed = Vector2Length(target->velocity);
			current_speed = fminf(current_speed + 50.f, BALL_SPEED_MAX); // Add speed on hit

			// Set new direction
			// X = Offset (Steering)
			// Y = 1.0f   (Always bounce DOWN)
			target->velocity.x = offset_x;
			target->velocity.y = 1.0f;

			target->velocity = Vector2Scale(Vector2Normalize(target->velocity), current_speed);

			// audio_sfx_play(SFX_PADDLE_HURT, 1.0f, true); // Or hit sound
			// target->position.y = survivor->entity.position.y + survivor->entity.size.y * 0.5f + target->radius + 1.0f;
		}
	}

	for (uint32_t projectile_index = 0; projectile_index < BRICK_MAX_PROJECTILES; ++projectile_index) {
		Entity *projectile = &encounter->projectiles[projectile_index];

		entity_update_physics(projectile, 1.0f, dt);
		entity_sync_collision(projectile);

		if ((projectile->position.x < -50 || projectile->position.x > GetScreenWidth() + 50) ||
			projectile->position.y < -50 || projectile->position.y > GetScreenHeight() + 50)
			projectile->active = false;
	}

	return STATE_CHANGE_NONE;
}

void paddle_breakout_exit(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;
}

void paddle_death_enter(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;

	for (uint32_t paddle_index = 0; paddle_index < countof(encounter->paddles); ++paddle_index) {
		encounter->paddles[paddle_index].entity.active = false;
		audio_sfx_play(SFX_PADDLE_DEATH, 1.0f, true);
	}
}
StateID paddle_death_update(void *context, float dt) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;
	return STATE_CHANGE_NONE;
}
void paddle_death_exit(void *context) {
	PaddleEncounter *encounter = (PaddleEncounter *)context;
}
