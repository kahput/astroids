#include "world.h"
#include "asteroid.h"
#include "audio_manager.h"
#include "common.h"
#include "core/astring.h"
#include "core/logger.h"
#include "fsm.h"
#include "globals.h"
#include "player.h"
#include "weapon.h"
#include <raylib.h>
#include <raymath.h>

static void draw_menu_screen(GameWorld *world, Color fade);
static void draw_win_screen(GameWorld *world, Color fade);
static void draw_lose_screen(GameWorld *world, Color fade);

void game_state_menu_enter(void *context);
StateID game_state_menu_update(void *context, float dt);
void game_state_menu_exit(void *context);

void game_state_asteroids_enter(void *context);
StateID game_state_asteroids_update(void *context, float dt);
void game_state_asteroids_exit(void *context);

void game_state_pong_enter(void *context);
StateID game_state_pong_update(void *context, float dt);
void game_state_pong_exit(void *context);

void game_state_win_enter(void *context);
StateID game_state_win_update(void *context, float dt);
void game_state_win_exit(void *context);

void game_state_lose_enter(void *context);
StateID game_state_lose_update(void *context, float dt);
void game_state_lose_exit(void *context);

static void stars_init(Star *stars, int width, int height) {
	for (int i = 0; i < MAX_STARS; i++) {
		stars[i].position = (Vector2){ GetRandomValue(0, width), GetRandomValue(0, height) };
		float depth = (float)GetRandomValue(0, 100) / 100.0f;
		stars[i].speed = clamp(depth, 15.0f, 50.0f);
		stars[i].size = (depth > 0.8f) ? 3.0f : 1.0f;
		unsigned char b = (unsigned char)(100 + (depth * 155));
		stars[i].color = (Color){ b, b, b, 255 };
	}
}

void world_init(GameWorld *world, Texture *atlas, Texture *paddle_tex, Shader *white) {
	*world = (GameWorld){ 0 };
	world->frame = arena_create(MiB(4));
	world->running = true;

	world->atlas = atlas;
	world->paddle_texture = paddle_tex;
	world->white = white;

	stars_init(world->stars, WINDOW_WIDTH, WINDOW_HEIGHT);
	weapon_system_init(&world->weapon_system, paddle_tex);
	player_init(&world->player, atlas);

	world->bar = (Rectangle){ WINDOW_WIDTH * (1 / 6.f), 20.f, (WINDOW_WIDTH * 2) / 3.f, 25.f };

	// Register all states
	StateHandler menu_state = {
		.on_enter = game_state_menu_enter,
		.on_exit = game_state_menu_exit,
		.on_update = game_state_menu_update,
	};

	StateHandler asteroid_state = {
		.on_enter = game_state_asteroids_enter,
		.on_exit = game_state_asteroids_exit,
		.on_update = game_state_asteroids_update,
	};

	StateHandler pong_state = {
		.on_enter = game_state_pong_enter,
		.on_exit = game_state_pong_exit,
		.on_update = game_state_pong_update,
	};

	StateHandler win_state = {
		.on_enter = game_state_win_enter,
		.on_exit = game_state_win_exit,
		.on_update = game_state_win_update,
	};

	StateHandler lose_state = {
		.on_enter = game_state_lose_enter,
		.on_exit = game_state_lose_exit,
		.on_update = game_state_lose_update,
	};

	fsm_state_add(&world->state_machine, GAME_PHASE_MENU, &menu_state);
	fsm_state_add(&world->state_machine, GAME_PHASE_ASTEROIDS, &asteroid_state);
	fsm_state_add(&world->state_machine, GAME_PHASE_BOSS, &pong_state);
	fsm_state_add(&world->state_machine, GAME_PHASE_WIN, &win_state);
	fsm_state_add(&world->state_machine, GAME_PHASE_LOSE, &lose_state);

	fsm_context_set(&world->state_machine, world);
	fsm_state_set(&world->state_machine, GAME_PHASE_BOSS);

	world->score = 0;
	world->high_score = 0; // TODO: Load from save file
}

void world_update(GameWorld *world, float dt) {
	if (IsKeyPressed(KEY_TAB))
		world->show_ui = !world->show_ui;
	if (IsKeyPressed(KEY_C))
		world->show_debug = !world->show_debug;
	if (IsKeyPressed(KEY_N))
		world->disable_collisions = !world->disable_collisions;

	for (int i = 0; i < MAX_STARS; i++) {
		world->stars[i].position.y += world->stars[i].speed * dt;
		if (world->stars[i].position.y > WINDOW_HEIGHT) {
			world->stars[i].position.y = -5;
			world->stars[i].position.x = GetRandomValue(0, WINDOW_WIDTH);
		}
	}

	StateID current_state = fsm_state_get(&world->state_machine);
	fsm_update(&world->state_machine, dt);
	if (current_state == GAME_PHASE_ASTEROIDS || current_state == GAME_PHASE_BOSS) {
		if (world->player.entity.active) {
			player_update(&world->player, &world->weapon_system, dt);
			weapon_bullets_update(&world->weapon_system, dt);

		} else {
			world->player.respawn_timer -= dt;
			if (world->player.respawn_timer <= 0.0f) {
				world_init(world, world->atlas, world->paddle_texture, world->white);
			}
		}
	}
}

float gui_slider(Arena *arena, String label, float value, float min, float max, float x, float y, float width);

void world_draw(GameWorld *world) {
	DrawRectangleGradientV(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (Color){ 5, 5, 20, 255 }, BLACK);
	for (int i = 0; i < MAX_STARS; i++) {
		if (world->stars[i].size > 1.5f)
			DrawRectangleV(world->stars[i].position, (Vector2){ 2, 2 }, world->stars[i].color);
		else
			DrawPixelV(world->stars[i].position, world->stars[i].color);
	}

	StateID current_state = fsm_state_get(&world->state_machine);
	if (current_state == GAME_PHASE_ASTEROIDS || current_state == GAME_PHASE_BOSS) {
		player_draw(&world->player);
		weapon_bullets_draw(&world->weapon_system, world->show_debug);
		asteroid_system_draw(&world->asteroid_system, world->show_debug);
		boss_encounter_paddle_draw(&world->boss, world->show_debug);

		if (world->boss_health_bar.width != 0) {
			DrawRectangleRec(world->bar, RAYWHITE);
			DrawRectangleRec(world->boss_health_bar, RED);
		}

		if (world->show_debug && world->player.entity.collision_active) {
			DrawRectangleLinesEx(world->player.entity.collision_shape, 1.f, GREEN);
		}

		if (world->show_ui) {
			world->player.rotation_speed = gui_slider(&world->frame, S("Turn Speed"), world->player.rotation_speed, 1.0f, 10.0f, 20, 50, 200);
			world->player.acceleration = gui_slider(&world->frame, S("Engine Power"), world->player.acceleration, 0.01f, 1.0f, 20, 80, 200);
			world->player.drag = gui_slider(&world->frame, S("Friction"), world->player.drag, 0.90f, 1.0f, 20, 110, 200);
		}
	}

	Color fade_color = Fade(WHITE, world->screen_fade);
	if (current_state == GAME_PHASE_MENU) {
		draw_menu_screen(world, fade_color);
	} else if (current_state == GAME_PHASE_WIN) {
		draw_win_screen(world, fade_color);
	} else if (current_state == GAME_PHASE_LOSE) {
		draw_lose_screen(world, fade_color);
	}
}

float gui_slider(Arena *arena, String label, float value, float min, float max, float x, float y, float width) {
	float height = 20;
	float knob_width = 10;

	float t = (value - min) / (max - min);

	Rectangle bar_area = { x + 80, y, width, height };
	Rectangle knob_area = { x + 80 + (t * (width - knob_width)), y, knob_width, height };

	// Interaction
	Vector2 mouse = GetMousePosition();
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, bar_area)) {
		float mouse_x_relative = mouse.x - (x + 80);
		t = mouse_x_relative / (width - knob_width);
		if (t < 0)
			t = 0;
		if (t > 1)
			t = 1;
		value = min + (t * (max - min));
	}

	// Drawing
	DrawText(label.data, x, y + 5, 10, WHITE); // Label

	DrawRectangleRec(bar_area, LIGHTGRAY); // Background Bar
	DrawRectangleRec(knob_area, RED); // Handle
	DrawRectangleLinesEx(bar_area, 1, WHITE); // Border

	String value_string = string_format(arena, S("%.2f"), value);
	DrawText(value_string.data, x + 80 + width + 10, y + 5, 10, WHITE);

	return value;
}

// ========================================
// MENU STATE
// ========================================
void game_state_menu_enter(void *context) {
	GameWorld *world = (GameWorld *)context;
	world->screen_fade = 0.0f;
	world->fading_out = false;

	world->asteroid_system = (AsteroidSystem){ 0 };
	world->boss = (PaddleEncounter){ 0 };
	world->score = 0;
}

StateID game_state_menu_update(void *context, float dt) {
	GameWorld *world = (GameWorld *)context;

	if (!world->fading_out && world->screen_fade < 1.0f) {
		world->screen_fade += dt * 2.0f;
		if (world->screen_fade > 1.0f)
			world->screen_fade = 1.0f;
	}

	if (IsKeyPressed(KEY_ESCAPE)) {
		world->running = false;
	}
	if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
		world->fading_out = true;
	}

	if (world->fading_out) {
		world->screen_fade -= dt * 2.0f;
		if (world->screen_fade <= 0.0f) {
			return GAME_PHASE_BOSS;
		}
	}

	return STATE_CHANGE_NONE;
}

void game_state_menu_exit(void *context) {
	GameWorld *world = (GameWorld *)context;
	world->screen_fade = 1.0f;
}

void game_state_asteroids_enter(void *context) {
	GameWorld *world = (GameWorld *)context;

	asteroid_system_init(&world->asteroid_system, world->atlas);
}
StateID game_state_asteroids_update(void *context, float dt) {
	GameWorld *world = (GameWorld *)context;

	AsteroidSystem *asteroid_system = &world->asteroid_system;
	asteroid_system_update(&world->asteroid_system, dt);

	if (world->player.entity.active) {
		for (int asteroid_index = 0; asteroid_index < MAX_ASTEROIDS; asteroid_index++) {
			Asteroid *asteroid = &asteroid_system->asteroids[asteroid_index];

			if (!asteroid->entity.active || asteroid->entity.collision_active == false || world->player.entity.collision_active == false)
				continue;

			if (CheckCollisionRecs(world->player.entity.collision_shape, asteroid->entity.collision_shape)) {
				player_kill(&world->player);
				return GAME_PHASE_LOSE;
			}
		}
	}

	for (int bullet_index = 0; bullet_index < MAX_BULLETS; bullet_index++) {
		Bullet *bullet = &world->weapon_system.bullets[bullet_index];
		if (!bullet->entity.active)
			continue;

		for (int a_idx = 0; a_idx < MAX_ASTEROIDS; a_idx++) {
			Asteroid *asteroid = &asteroid_system->asteroids[a_idx];
			if (!asteroid->entity.active || asteroid->entity.collision_active == false || bullet->entity.collision_active == false)
				continue;

			entity_sync_collision(&bullet->entity);
			if (CheckCollisionRecs(bullet->entity.collision_shape, asteroid->entity.collision_shape)) {
				bullet->entity.active = false;
				asteroid->entity.active = false;

				// Audio
				// audio_sfx_play(SFX_EXPLOSION);
				world->score += 100;

				if (asteroid->variant == ASTEROID_VARIANT_LARGE) {
					asteroid_spawn_split(asteroid_system, asteroid->entity.position, ASTEROID_VARIANT_MEDIUM);
					asteroid_spawn_split(asteroid_system, asteroid->entity.position, ASTEROID_VARIANT_MEDIUM);
				} else if (asteroid->variant == ASTEROID_VARIANT_MEDIUM) {
					asteroid_spawn_split(asteroid_system, asteroid->entity.position, ASTEROID_VARIANT_SMALL);
					asteroid_spawn_split(asteroid_system, asteroid->entity.position, ASTEROID_VARIANT_SMALL);
				}

				break;
			}
		}
	}

	if (world->score >= BOSS_SCORE_THRESHOLD_PONG) {
		world->asteroid_system.spawn_rate = 0;

		bool any_active = false;
		for (int i = 0; i < MAX_ASTEROIDS; i++) {
			if (world->asteroid_system.asteroids[i].entity.active) {
				any_active = true;
				break;
			}
		}

		if (!any_active)
			return GAME_PHASE_BOSS;
	}

	return STATE_CHANGE_NONE;
}

void game_state_asteroids_exit(void *context) {
	GameWorld *world = (GameWorld *)context;

	world->asteroid_system = (AsteroidSystem){ 0 };
}

void game_state_pong_enter(void *context) {
	GameWorld *world = (GameWorld *)context;

	boss_encounter_paddle_initialize(&world->boss, world->paddle_texture);
}
StateID game_state_pong_update(void *context, float dt) {
	GameWorld *world = (GameWorld *)context;

	if (boss_encounter_paddle_check_collision(&world->boss, &world->player.entity)) {
		audio_music_stop_all();
		player_kill(&world->player);
		return GAME_PHASE_LOSE;
	}

	for (uint32_t bullet_index = 0; bullet_index < MAX_BULLETS; bullet_index++) {
		Bullet *bullet = &world->weapon_system.bullets[bullet_index];
		if (!bullet->entity.active)
			continue;

		for (uint32_t paddle_index = 0; paddle_index < countof(world->boss.paddles); paddle_index++) {
			Paddle *paddle = &world->boss.paddles[paddle_index];
			if (paddle->entity.active == false || paddle->entity.collision_active == false || world->player.entity.collision_active == false)
				continue;

			entity_sync_collision(&bullet->entity);
			entity_sync_collision(&paddle->entity);
			if (CheckCollisionRecs(bullet->entity.collision_shape, paddle->entity.collision_shape) == false)
				continue;

			boss_paddle_apply_damage(paddle, bullet->damage);
			bullet->entity.active = false;
			break;
		}
	}

	boss_encounter_paddle_update(&world->boss, world->player.entity.position, dt);
	world->boss_health_bar = (Rectangle){
		world->bar.x, world->bar.y,
		world->bar.width * boss_encounter_paddle_health_ratio(&world->boss),
		world->bar.height
	};

	bool boss_dead = true;
	for (int paddle_index = 0; paddle_index < MAX_PADDLES; paddle_index++) {
		if (world->boss.paddles[paddle_index].entity.active) {
			boss_dead = false;
			break;
		}
	}

	if (boss_dead)
		return GAME_PHASE_WIN;

	return STATE_CHANGE_NONE;
}

void game_state_pong_exit(void *context) {
	GameWorld *world = (GameWorld *)context;

	world->boss = (PaddleEncounter){ 0 };
	world->boss_health_bar = (Rectangle){ 0 };
}

// ========================================
// WIN STATE
// ========================================
void game_state_win_enter(void *context) {
	GameWorld *world = (GameWorld *)context;
	world->screen_fade = 0.0f;
	world->fading_out = false;

	// Update high score
	if (world->score > world->high_score) {
		world->high_score = world->score;
		// TODO: Save to file
	}

	audio_music_stop(MUSIC_BOSS_PONG);
	// TODO: Play victory sound/music
}

StateID game_state_win_update(void *context, float dt) {
	GameWorld *world = (GameWorld *)context;

	// Fade in
	if (!world->fading_out && world->screen_fade < 1.0f) {
		world->screen_fade += dt * 1.5f;
		if (world->screen_fade > 1.0f)
			world->screen_fade = 1.0f;
	}

	// Return to menu
	if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
		world->fading_out = true;
	}

	if (world->fading_out) {
		world->screen_fade -= dt * 2.0f;
		if (world->screen_fade <= 0.0f) {
			return GAME_PHASE_MENU;
		}
	}

	return STATE_CHANGE_NONE;
}

void game_state_win_exit(void *context) {
	// Nothing to clean up
}

// ========================================
// LOSE STATE
// ========================================
void game_state_lose_enter(void *context) {
	GameWorld *world = (GameWorld *)context;
	world->screen_fade = 0.0f;
	world->fading_out = false;

	audio_music_stop(MUSIC_BOSS_PONG);
	audio_loop_stop(LOOP_PLAYER_ROCKET);
}

StateID game_state_lose_update(void *context, float dt) {
	GameWorld *world = (GameWorld *)context;

	// Fade in
	if (!world->fading_out && world->screen_fade < 1.0f) {
		world->screen_fade += dt * 1.5f;
		if (world->screen_fade > 1.0f)
			world->screen_fade = 1.0f;
	}

	// Retry or menu
	static uint32_t key_pressed = 0;
	if (IsKeyPressed(KEY_SPACE) && key_pressed == 0) {
		world->fading_out = true;
		world->screen_fade = 1.0f;
		key_pressed = KEY_SPACE;
	}

	if (IsKeyPressed(KEY_ESCAPE) && key_pressed == 0) {
		world->fading_out = true;
		world->screen_fade = 2.0f;
		key_pressed = KEY_ESCAPE;
	}

	if (world->fading_out) {
		world->screen_fade -= dt;
		if (world->screen_fade <= 0.0f) {
			if (key_pressed == KEY_SPACE) {
				key_pressed = 0;
				return GAME_PHASE_BOSS;
			} else {
				key_pressed = 0;
				return GAME_PHASE_BOSS;
			}
		}
	}

	return STATE_CHANGE_NONE;
}

void game_state_lose_exit(void *context) {
	GameWorld *world = (GameWorld *)context;

	// Reset player for retry
	player_init(&world->player, world->atlas);
	world->score = 0;
}

void draw_menu_screen(GameWorld *world, Color fade) {
	int center_x = WINDOW_WIDTH / 2;
	int center_y = WINDOW_HEIGHT / 2;

	const char *title = "ASTEROIDS";
	int title_size = 80;
	int title_width = MeasureText(title, title_size);
	DrawText(title, center_x - title_width / 2, center_y - 150, title_size, fade);

	// Subtitle
	const char *subtitle = "I guess";
	int subtitle_size = 30;
	int subtitle_width = MeasureText(subtitle, subtitle_size);
	DrawText(subtitle, center_x - subtitle_width / 2, center_y - 80, subtitle_size,
		Fade(GRAY, world->screen_fade));

	// Instructions
	const char *start = "PRESS SPACE TO START";
	int start_size = 25;
	int start_width = MeasureText(start, start_size);

	// Pulsing effect
	float pulse = (sinf(GetTime() * 3.0f) + 1.0f) * 0.5f;
	Color pulse_color = Fade(WHITE, world->screen_fade * (0.5f + pulse * 0.5f));
	DrawText(start, center_x - start_width / 2, center_y + 50, start_size, pulse_color);

	// Controls
	const char *controls[] = {
		"W - THRUST",
		"A/D - ROTATE",
		"SPACE - SHOOT",
	};

	int y_offset = center_y + 120;
	for (int i = 0; i < 3; i++) {
		int width = MeasureText(controls[i], 20);
		DrawText(controls[i], center_x - width / 2, y_offset + (i * 30), 20,
			Fade(LIGHTGRAY, world->screen_fade));
	}

	// High score
	if (world->high_score > 0) {
		const char *high_score = TextFormat("HIGH SCORE: %d", world->high_score);
		int hs_width = MeasureText(high_score, 20);
		DrawText(high_score, center_x - hs_width / 2, WINDOW_HEIGHT - 50, 20,
			Fade(YELLOW, world->screen_fade));
	}
}

void draw_win_screen(GameWorld *world, Color fade) {
	int center_x = WINDOW_WIDTH / 2;
	int center_y = WINDOW_HEIGHT / 2;

	// Victory text
	const char *victory = "VICTORY!";
	int victory_size = 80;
	int victory_width = MeasureText(victory, victory_size);
	DrawText(victory, center_x - victory_width / 2, center_y - 100, victory_size,
		Fade(GREEN, world->screen_fade));

	// Score
	const char *score_text = TextFormat("FINAL SCORE: %d", world->score);
	int score_size = 40;
	int score_width = MeasureText(score_text, score_size);
	DrawText(score_text, center_x - score_width / 2, center_y, score_size, fade);

	// High score indicator
	if (world->score >= world->high_score) {
		const char *new_high = "NEW HIGH SCORE!";
		int nh_size = 30;
		int nh_width = MeasureText(new_high, nh_size);

		float pulse = (sinf(GetTime() * 4.0f) + 1.0f) * 0.5f;
		Color pulse_color = Fade(YELLOW, world->screen_fade * (0.5f + pulse * 0.5f));
		DrawText(new_high, center_x - nh_width / 2, center_y + 50, nh_size, pulse_color);
	}

	// Continue prompt
	const char *prompt = "PRESS SPACE TO CONTINUE";
	int prompt_width = MeasureText(prompt, 20);
	DrawText(prompt, center_x - prompt_width / 2, center_y + 120, 20,
		Fade(WHITE, world->screen_fade * 0.7f));
}

void draw_lose_screen(GameWorld *world, Color fade) {
	int center_x = WINDOW_WIDTH / 2;
	int center_y = WINDOW_HEIGHT / 2;

	// Game over text
	const char *game_over = "GAME OVER";
	int go_size = 80;
	int go_width = MeasureText(game_over, go_size);
	DrawText(game_over, center_x - go_width / 2, center_y - 100, go_size,
		Fade(RED, world->screen_fade));

	// Score
	const char *score_text = TextFormat("SCORE: %d", world->score);
	int score_size = 40;
	int score_width = MeasureText(score_text, score_size);
	DrawText(score_text, center_x - score_width / 2, center_y, score_size, fade);

	// Options
	const char *retry = "SPACE - RETRY";
	const char *menu = "ESC - MENU";

	int retry_width = MeasureText(retry, 25);
	int menu_width = MeasureText(menu, 25);

	DrawText(retry, center_x - retry_width / 2, center_y + 80, 25,
		Fade(WHITE, world->screen_fade));
	DrawText(menu, center_x - menu_width / 2, center_y + 120, 25,
		Fade(LIGHTGRAY, world->screen_fade));
}
