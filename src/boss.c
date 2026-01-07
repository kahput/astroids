#include "boss.h"
#include "common.h"
#include "game.h"
#include "entity.h"
#include "audio_manager.h" // Assuming this exists
#include <raylib.h>
#include <raymath.h>

// --- CONSTANTS FOR TUNING ---
#define BOSS_INTRO_DURATION_WARNING 2.0f
#define BOSS_INTRO_DURATION_ENTER 1.5f
#define BOSS_MOVE_SPEED_FIGHT 3.5f

float boss_fight_health_ratio(BossEncounter *encounter) {
	float current = 0;
	for (int i = 0; i < 2; i++)
		current += encounter->bosses[i].health;
	return current / encounter->total_max_health;
}

void boss_encounter_paddle_initialize(GameContext *context, BossEncounter *encounter, Texture2D *paddle_texture) {
	encounter->total_max_health = 100.f;
	encounter->phase = BOSS_PHASE_INTRO;
	encounter->state_timer = 0.0f;

	for (int i = 0; i < 2; i++) {
		Boss *boss = &encounter->bosses[i];

		boss->entity.active = true;
		boss->entity.size = (Vector2){ context->tile_size * 2, context->tile_size * 8 };

		// START POS: Off-screen!
		float start_x = (i == 0) ? -200.0f : context->window_width + 200.0f;
		boss->entity.position = (Vector2){ start_x, context->window_height * .5f };

		boss->entity.velocity = (Vector2){ 0, 0 };
		boss->entity.tint = WHITE;

		// Assign texture area (assuming vertical strip in atlas)
		boss->entity.area = (Rectangle){ 0, (context->tile_size * 2) * i, context->tile_size, context->tile_size * 2 };
		boss->entity.texture = paddle_texture;

		boss->entity.collision_shape = (Rectangle){ 0, 0, boss->entity.size.x, boss->entity.size.y };

		boss->max_health = encounter->total_max_health * .5f;
		boss->health = boss->max_health;
		boss->flash_timer = 0.0f;
	}
}

void boss_encounter_paddle_update(GameContext *context, BossEncounter *encounter, Vector2 player_pos) {
	encounter->state_timer += context->dt;

	// --- STATE MACHINE ---
	switch (encounter->phase) {
		case BOSS_PHASE_INTRO: {
			float t = encounter->state_timer;

			// Sub-phase 1: Warning Flashes (0.0s to 2.0s)
			audio_loop_play(LOOP_BOSS_SIREN);
            audio_loop_set_volume(LOOP_BOSS_SIREN, .3);

			// Sub-phase 2: Enter Arena (2.0s to 3.5s)
			if (t > BOSS_INTRO_DURATION_WARNING) {
            audio_loop_stop(LOOP_BOSS_SIREN);
				float enter_t = (t - BOSS_INTRO_DURATION_WARNING) / BOSS_INTRO_DURATION_ENTER;
				if (enter_t > 1.0f)
					enter_t = 1.0f;

				// Smooth ease-out for entrance
				// formula: 1 - (1-t)^3 creates a "Fast then slow" arrival
				float ease = 1.0f - powf(1.0f - enter_t, 3.0f);

				for (int i = 0; i < 2; i++) {
					float start_x = (i == 0) ? -200.0f : context->window_width + 200.0f;
					float end_x = (i == 0) ? context->tile_size * 2 : context->window_width - context->tile_size * 2;

					encounter->bosses[i].entity.position.x = Lerp(start_x, end_x, ease);
					encounter->bosses[i].entity.position.y = context->window_height * 0.5f; // Stay centered vertically
				}

				// Transition to Fight
				if (enter_t >= 1.0f) {
					encounter->phase = BOSS_PHASE_PATTERN_A;
					encounter->state_timer = 0.0f;
				}
			}
		} break;

		case BOSS_PHASE_PATTERN_A: {
			// Basic Attack: "Aim" at player (Track Y position)
			// This is "Pong AI" - move paddle center towards player center

			for (int i = 0; i < 2; i++) {
				Boss *boss = &encounter->bosses[i];
				if (!boss->entity.active)
					continue;

				float target_y = player_pos.y;
				float current_y = boss->entity.position.y;
				float diff = target_y - current_y;

				// Cap movement speed
				float move = 0;
				if (fabsf(diff) > 2.0f) { // Deadzone to prevent jitter
					float speed = BOSS_MOVE_SPEED_FIGHT;
					move = (diff > 0) ? speed : -speed;
				}

				boss->entity.velocity.y = move;
				boss->entity.position.y += boss->entity.velocity.y; // Applying velocity manually since we control AI

				// Screen Bounds Clamp
				float half_h = boss->entity.size.y * 0.5f;
				if (boss->entity.position.y < half_h)
					boss->entity.position.y = half_h;
				if (boss->entity.position.y > context->window_height - half_h)
					boss->entity.position.y = context->window_height - half_h;
			}
		} break;

		default:
			break;
	}

	// --- COMMON UPDATES (Collision Sync, Timers) ---
	for (int i = 0; i < 2; i++) {
		Boss *boss = &encounter->bosses[i];
		if (boss->entity.active) {
			entity_sync_collision(&boss->entity);

			if (boss->flash_timer > 0.0f) {
				boss->flash_timer -= context->dt;
				if (boss->flash_timer <= 0.0f)
					boss->flash_timer = 0.0f;
			}
		}
	}
}

void boss_encounter_paddle_draw(GameContext *context, BossEncounter *encounter) {
	// --- DRAW INTRO EFFECTS ---
	if (encounter->phase == BOSS_PHASE_INTRO) {
		float t = encounter->state_timer;

		// Draw Flashing Danger Zones (0s - 2.5s)
		if (t < BOSS_INTRO_DURATION_WARNING + 0.5f) {
			// Flash frequency: 8 times per second
			float alpha = (sinf(t * 15.0f) + 1.0f) * 0.5f;
			Color warning_col = Fade(RED, alpha * 0.5f); // Max 50% opacity

			// Left Bar
			DrawRectangle(0, 0, context->tile_size, context->window_height, warning_col);
			// Right Bar
			DrawRectangle(context->window_width - context->tile_size, 0, context->tile_size, context->window_height, warning_col);

			// Optional Text
			if (alpha > 0.5f) {
				const char *text = "WARNING";
				int text_w = MeasureText(text, 20);
				DrawText(text, 10, context->window_height / 2, 20, RED);
				DrawText(text, context->window_width - 10 - text_w, context->window_height / 2, 20, RED);
			}
		}
	}

	// --- DRAW BOSSES ---
	for (uint32_t i = 0; i < 2; ++i) {
		Boss *boss = &encounter->bosses[i];
		if (boss->entity.active) {
			if (boss->flash_timer > 0.0f) {
				BeginShaderMode(context->flash_shader);
				entity_draw(&boss->entity);
				EndShaderMode();
			} else {
				entity_draw(&boss->entity);
			}
		}
	}
}

bool boss_encounter_paddle_check_collision(BossEncounter *encounter, Entity *player) {
	for (int i = 0; i < 2; i++) {
		if (encounter->bosses[i].entity.active) {
			entity_sync_collision(&encounter->bosses[i].entity);
			if (CheckCollisionRecs(player->collision_shape, encounter->bosses[i].entity.collision_shape))
				return true;
		}
	}
	return false;
}

void boss_apply_damage(Boss *boss, float damage) {
	boss->flash_timer = 0.1f;
	boss->health -= damage;
	if (boss->health <= 0.0f) {
		boss->entity.active = false;
		audio_sfx_play(SFX_PADDLE_DEATH, 1.0f);
	} else {
		audio_sfx_play(SFX_PADDLE_HURT, 1.0f);
	}
}

bool boss_is_alive(Boss *boss) {
	return boss->entity.active;
}
