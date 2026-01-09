
#include <raylib.h>
#include <raymath.h>

#include "core/arena.h"
#include "core/debug.h"
#include "core/logger.h"
#include "core/astring.h"

#include "game.h"
#include "globals.h"
#include "entity.h"
#include "audio_manager.h"

#include "paddle_boss.h"

#define MAX_BULLETS 100
#define BULLET_LIFTIME 1.f

#define MAX_STARS 200
#define STAR_SPEED_MIN 15.0f
#define STAR_SPEED_MAX 50.0f

typedef struct {
	Vector2 position;
	float speed;
	float size;
	Color color;
} Star;
static Star stars[MAX_STARS];

const uint32_t WINDOW_WIDTH = 1280;
const uint32_t WINDOW_HEIGHT = 720;

typedef Rectangle TextureArea;

#ifdef PLATFORM_WEB
const char *FLASH_SHADER_CODE =
	"precision mediump float;\n"
	"varying vec2 fragTexCoord;\n"
	"varying vec4 fragColor;\n"
	"uniform sampler2D texture0;\n"
	"void main()\n"
	"{\n"
	"    vec4 texelColor = texture2D(texture0, fragTexCoord);\n"
	"    if (texelColor.a <= 0.1) discard;\n"
	"    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
	"}\n";
#else
const char *FLASH_SHADER_CODE =
	"#version 330\n"
	"in vec2 fragTexCoord;\n"
	"in vec4 fragColor;\n"
	"out vec4 finalColor;\n"
	"uniform sampler2D texture0;\n"
	"void main()\n"
	"{\n"
	"    vec4 texelColor = texture(texture0, fragTexCoord);\n"
	"    if (texelColor.a <= 0.1) discard;\n"
	"    finalColor = vec4(1.0, 1.0, 1.0, 1.0f);\n"
	"}\n";
#endif

float gui_slider(Arena *arena, String label, float value, float min, float max, float x, float y, float width);

void background_initialize(void);
void background_update(float dt);
void background_draw(void);

int main(void) {
	GameContext context = {
		.frame_arena = arena_create(MiB(4)),
		.window_width = 1280,
		.window_height = 720,
		.show_debug = false,
		.show_ui = true,
	};

	InitWindow(context.window_width, context.window_height, "Astroids");
	SetTargetFPS(60);

	background_initialize();
	audio_initialize();

	Texture atlas = LoadTexture("assets/sprites/asteroid_sprite.png");
	Texture paddle_texture = LoadTexture("assets/sprites/boss_paddle.png");

	Sound sfx_player_shoot = LoadSound("assets/sfx/shoot.wav");
	Sound sfx_player_death = LoadSound("assets/sfx/player_death.wav");
	Sound sfx_player_rocket = LoadSound("assets/sfx/player_rocket.wav");
	Sound sfx_paddle_hurt = LoadSound("assets/sfx/paddle_hurt.wav");
	Sound sfx_paddle_death = LoadSound("assets/sfx/paddle_death.wav");

	Music boss_music_paddle = LoadMusicStream("assets/music/boss_music.wav");

	context.flash_shader = LoadShaderFromMemory(NULL, FLASH_SHADER_CODE);

	Entity player = {
		.active = true,
		.position = { .x = WINDOW_WIDTH * .5f, .y = WINDOW_HEIGHT * .5f },
		.velocity = { 0, 0 },
		.size = { .x = PLAYER_SIZE, .y = PLAYER_SIZE },
		.rotation = 0,
		.area = { 0, 0, TILE_SIZE, TILE_SIZE },
		.tint = WHITE,
		.texture = &atlas
	};
	uint32_t player_frame = 0;
	float animation_timer = 0.0f;
	player.collision_shape = (Rectangle){ 0, 0, .width = PLAYER_SIZE * .6f, .height = PLAYER_SIZE * .7f };

	// BossEncounter paddle_encounter = { 0 };
	// boss_encounter_paddle_initialize(&context, &paddle_encounter, &paddle_texture);

	PaddleEncounter encounter = { 0 };
	boss_encounter_paddle_initialize(&context, &encounter, &paddle_texture, &boss_music_paddle);

	Entity bullets[MAX_BULLETS] = { 0 };
	uint32_t bullet_count = 0;

	float rotation_speed = 4.5f;
	float acceleration = 0.4f;
	float drag = 0.95;

	float bullet_speed = 15.f;
	float bullet_base_damage = 1.4f;

	float fire_rate = .2f;
	float fire_timer = 0.0f;

	Color DARK = { 20, 20, 20, 255 };
	while (WindowShouldClose() == false) {
		context.dt = GetFrameTime();
		background_update(context.dt);
		audio_update(context.dt);
		boss_encounter_paddle_update(&encounter, player.position, context.dt);

		UpdateMusicStream(boss_music_paddle);

		BeginDrawing();
		background_draw();

		fire_timer += context.dt;
		for (uint32_t bullet_index = 0; bullet_index < MAX_BULLETS; ++bullet_index) {
			Entity *bullet = &bullets[bullet_index];
			if (bullet->active == false)
				continue;

			bullet->position = Vector2Add(bullet->position, bullet->velocity);
			entity_sync_collision(bullet);

			for (uint32_t paddle_index = 0; paddle_index < countof(encounter.paddles); ++paddle_index) {
				Paddle *paddle = &encounter.paddles[paddle_index];
				Entity *paddle_entity = &paddle->entity;

				if (paddle_entity->active) {
					if (CheckCollisionRecs(bullet->collision_shape, paddle_entity->collision_shape))
						if (CheckCollisionRecs(bullet->collision_shape, paddle_entity->collision_shape)) {
							boss_paddle_apply_damage(paddle, bullet->bullet_damage);

							bullet->active = false;
							break;
						}
				}
			}
			if (bullet->active == false)
				continue;

			bullet->bullet_life_timer -= context.dt;
			if (bullet->bullet_life_timer <= 0.0f)
				bullet->active = false;

			float half_w = bullet->size.x * .5f;
			float half_h = bullet->size.y * .5f;

			if (bullet->position.x < -half_w)
				bullet->position.x = WINDOW_WIDTH + half_w;
			else if (bullet->position.x > WINDOW_WIDTH + half_w)
				bullet->position.x = -half_w;

			if (bullet->position.y < -half_h)
				bullet->position.y = WINDOW_HEIGHT + half_h;
			else if (bullet->position.y > WINDOW_HEIGHT + half_h)
				bullet->position.y = -half_h;
		}

		static bool32 disable_player_collision = false;
		if (player.active) {
			if (IsKeyDown(KEY_D))
				player.rotation += rotation_speed;
			if (IsKeyDown(KEY_A))
				player.rotation -= rotation_speed;

			if (IsKeyPressed(KEY_SPACE) && fire_timer >= fire_rate) {
				Entity *bullet = NULL;
				for (int bullet_index = 0; bullet_index < MAX_BULLETS; bullet_index++) {
					if (bullets[bullet_index].active == false) {
						bullet = &bullets[bullet_index];
						break;
					}
				}

				if (bullet) {
					Vector2 aim_direction = Vector2Rotate((Vector2){ 0, -1 }, player.rotation * DEG2RAD);
					Vector2 spawn_position = Vector2Add(player.position, Vector2Scale(aim_direction, player.size.y * 0.5f));

					// LOG_INFO("Player movespeed = %.2f", Vector2Length(player.velocity));
					float inverse_drag = 1 / (1 - drag);

					float player_max_speed = (acceleration * drag) / (1 - drag);
					float t = Vector2Length(player.velocity) / player_max_speed;

					float damage_multiplier = 1.0f + t * 2.0f;
					// LOG_INFO("Damage multiplier = %.2f", damage_multiplier);
					*bullet = (Entity){
						.active = true,
						.bullet_life_timer = BULLET_LIFTIME,
						.bullet_damage = bullet_base_damage * damage_multiplier,
						.position = { spawn_position.x, spawn_position.y },
						.velocity = { aim_direction.x * bullet_speed, aim_direction.y * bullet_speed },
						.area = { TILE_SIZE* 6, 0, TILE_SIZE,  TILE_SIZE},
						.texture = &paddle_texture,
						.size = { 32.f, 32.f },
						.rotation = player.rotation,
						.tint = WHITE,
					};

					bullet->collision_shape = (Rectangle){ 0, 0, bullet->size.x, bullet->size.y };

					SetSoundPitch(sfx_player_shoot, GetRandomValue(80, 100) / 100.f);
					PlaySound(sfx_player_shoot);
					fire_timer = 0.0f;
				}
			}

			if (IsKeyDown(KEY_W)) {
				Vector2 thrust_direction = Vector2Rotate((Vector2){ 0, -1 }, player.rotation * DEG2RAD);
				player.velocity = Vector2Add(player.velocity, Vector2Scale(thrust_direction, acceleration));

				if (player_frame == 0) {
					player_frame = 2;
					animation_timer = 0;
				}

				animation_timer += context.dt;
				if (animation_timer >= ANIMATION_SPEED) {
					player_frame = (player_frame + 1) % 4;
					animation_timer = 0.0f;
				}

				player.area.x = TILE_SIZE * player_frame;

				// TODO: Proper rocket sound
				audio_loop_play(LOOP_PLAYER_ROCKET);
				audio_loop_set_pitch(LOOP_PLAYER_ROCKET, 1.0f + (Vector2Length(player.velocity) * 0.05f));
				audio_loop_set_volume(LOOP_PLAYER_ROCKET, .3f);

			} else {
				player_frame = 0;
				player.area.x = 0;

				audio_loop_stop(LOOP_PLAYER_ROCKET);
			}

			entity_update_physics(&player, drag, context.dt);
			entity_sync_collision(&player);

			if (disable_player_collision == false)
				if (boss_encounter_paddle_check_collision(&encounter, &player)) {
					StopMusicStream(boss_music_paddle);
					player.respawn_timer = 1.f;
					player.active = false;
					audio_loop_stop(LOOP_PLAYER_ROCKET);

					// encounter = (PaddleEncounter){ 0 };

					SetSoundPitch(sfx_player_death, GetRandomValue(80, 100) / 100.f);
					PlaySound(sfx_player_death);
				}

			float half_w = player.size.x * .5f;
			float half_h = player.size.y * .5f;

			if (player.position.x < -half_w)
				player.position.x = WINDOW_WIDTH + half_w;
			else if (player.position.x > WINDOW_WIDTH + half_w)
				player.position.x = -half_w;

			if (player.position.y < -half_h)
				player.position.y = WINDOW_HEIGHT + half_h;
			else if (player.position.y > WINDOW_HEIGHT + half_h)
				player.position.y = -half_h;

			entity_draw(&player);

		} else {
			player.respawn_timer -= context.dt;
			if (player.respawn_timer <= 0.0f) {
				player.respawn_timer = 0.0f;
				player.position = (Vector2){ .x = WINDOW_WIDTH * .5f, .y = WINDOW_HEIGHT * .5f };
				player.velocity = (Vector2){ 0 };
				player.rotation = 0;
				player.active = true;

				boss_encounter_paddle_initialize(&context, &encounter, &paddle_texture, &boss_music_paddle);
			}
		}

		// boss_encounter_paddle_draw(&context, &paddle_encounter);
		boss_encounter_paddle_draw(&encounter);

		for (uint32_t index = 0; index < MAX_BULLETS; ++index) {
			if (bullets[index].active == true)
				entity_draw(&bullets[index]);
		}

		Rectangle health_bar = { WINDOW_HEIGHT * .25f, 20.f, WINDOW_WIDTH * 2.f / 3.f, 25.f };
		Rectangle boss_health_bar = { health_bar.x, health_bar.y, health_bar.width * boss_encounter_paddle_health_ratio(&encounter), health_bar.height };

		DrawRectangleRec(health_bar, RAYWHITE);
		DrawRectangleRec(boss_health_bar, RED);

		if (context.show_debug)
			DrawRectangleLinesEx(player.collision_shape, 1.f, disable_player_collision ? RED : GREEN);

		if (context.show_ui) {
			rotation_speed = gui_slider(&context.frame_arena, S("Turn Speed"), rotation_speed, 1.0f, 10.0f, 20, 50, 200);
			acceleration = gui_slider(&context.frame_arena, S("Engine Power"), acceleration, 0.01f, 1.0f, 20, 80, 200);
			drag = gui_slider(&context.frame_arena, S("Friction (Drag)"), drag, 0.90f, 1.0f, 20, 110, 200);
		}

		if (IsKeyPressed(KEY_TAB))
			context.show_ui = !context.show_ui;
		if (IsKeyPressed(KEY_C))
			context.show_debug = !context.show_debug;
		if (IsKeyPressed(KEY_N))
			disable_player_collision = !disable_player_collision;

		EndDrawing();

		arena_reset(&context.frame_arena);
	}

	UnloadMusicStream(boss_music_paddle);
	audio_unload();
	UnloadShader(context.flash_shader);
	CloseWindow();

	arena_destroy(&context.frame_arena);
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

void background_initialize(void) {
	for (int i = 0; i < MAX_STARS; i++) {
		stars[i].position = (Vector2){
			.x = GetRandomValue(0, WINDOW_WIDTH),
			.y = GetRandomValue(0, WINDOW_HEIGHT)
		};

		float depth = (float)GetRandomValue(0, 100) / 100.0f;
		stars[i].speed = clamp(depth, STAR_SPEED_MIN, STAR_SPEED_MAX);

		stars[i].size = (depth > 0.8f) ? 3.0f : 1.0f;

		unsigned char brightness = (unsigned char)(100 + (depth * 155));
		stars[i].color = (Color){ brightness, brightness, brightness, 255 };
	}
}

void background_update(float dt) {
	for (int i = 0; i < MAX_STARS; i++) {
		stars[i].position.y += stars[i].speed * dt;

		if (stars[i].position.y > WINDOW_HEIGHT) {
			stars[i].position.y = -5;
			stars[i].position.x = GetRandomValue(0, WINDOW_WIDTH);
		}
	}
}

void background_draw(void) {
	DrawRectangleGradientV(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (Color){ 5, 5, 20, 255 }, BLACK);

	for (int star_index = 0; star_index < MAX_STARS; star_index++) {
		if (stars[star_index].size > 1.5f) {
			DrawRectangle(stars[star_index].position.x, stars[star_index].position.y, 2, 2, stars[star_index].color);
		} else {
			DrawPixel(stars[star_index].position.x, stars[star_index].position.y, stars[star_index].color);
		}
	}
}
