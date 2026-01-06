#include "common.h"

#include <raylib.h>
#include <raymath.h>

#include "core/arena.h"
#include "core/debug.h"
#include "core/logger.h"
#include "core/astring.h"

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
const uint32_t TILE_SIZE = 32;
const uint32_t PLAYER_SIZE = 64;
#define ANIMATION_SPEED .1f

typedef Rectangle TextureArea;

typedef struct {
	bool active;
	float life_timer;
	float flash_timer;
	float respawn_timer;

	Vector2 position;
	Vector2 velocity;
	Vector2 size;
	float rotation;

	Rectangle collision_shape;
	Rectangle hitbox, hurtbox;

	float health;
	float movement_speed;

	TextureArea area;
	Color tint;
	Texture2D *texture;
} Entity;

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
	"    finalColor = vec4(1.0, 1.0, 1.0, texelColor.a);\n"
	"}\n";

void entity_draw(Entity sprite);
void collision_shape_sync(Entity *sprite);
float gui_slider(Arena *arena, String label, float value, float min, float max, float x, float y, float width);

void background_initialize(void);
void background_update(float dt);
void background_draw(void);

int main(void) {
	Arena frame_arena = arena_create(MiB(4));

	InitWindow(1280, 720, "Astroids");
	InitAudioDevice();
	SetTargetFPS(60);

	background_initialize();

	Texture atlas = LoadTexture("assets/sprites/asteroid_sprite.png");
	Texture paddle = LoadTexture("assets/sprites/paddle.png");

	Sound sfx_player_shoot = LoadSound("assets/sfx/shoot.wav");
	Sound sfx_player_death = LoadSound("assets/sfx/player_death.wav");
	Sound sfx_player_rocket = LoadSound("assets/sfx/player_rocket.wav");
	Sound sfx_paddle_hurt = LoadSound("assets/sfx/paddle_hurt.wav");
	Sound sfx_paddle_death = LoadSound("assets/sfx/paddle_death.wav");

	Shader flash_shader = LoadShaderFromMemory(NULL, FLASH_SHADER_CODE);

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

	Entity paddle_bosses[2] = { 0 };
	float paddle_boss_max_health = 100.f;

	for (int i = 0; i < 2; i++) {
		paddle_bosses[i].active = true;
		paddle_bosses[i].size = (Vector2){ PLAYER_SIZE, 4 * PLAYER_SIZE };
		paddle_bosses[i].position = (Vector2){ (i == 0 ? PLAYER_SIZE : WINDOW_WIDTH - PLAYER_SIZE), WINDOW_HEIGHT * .5f };
		paddle_bosses[i].velocity = (Vector2){ 0, i ? 1 : -1 };
		paddle_bosses[i].tint = RAYWHITE;
		paddle_bosses[i].area = (Rectangle){ 0, (TILE_SIZE * 2) * i, TILE_SIZE, TILE_SIZE * 2 };
		paddle_bosses[i].texture = &paddle;

		paddle_bosses[i].collision_shape = (Rectangle){ 0, 0, paddle_bosses[i].size.x, paddle_bosses[i].size.y };
		paddle_bosses[i].health = paddle_boss_max_health * .5f;
	}

	Entity bullets[MAX_BULLETS] = { 0 };
	uint32_t bullet_count = 0;

	float rotation_speed = 4.5f;
	float acceleration = 0.2f;
	float drag = 0.99f;

	float bullet_speed = 15.f;
	float bullet_damage = 1.4f;

	float fire_rate = .2f;
	float fire_timer = 0.0f;

	bool ui_toggle = false;
	bool collision_shape_toggle = false;

	Color DARK = { 20, 20, 20, 255 };
	while (WindowShouldClose() == false) {
		float dt = GetFrameTime();
		background_update(dt);

		BeginDrawing();
		background_draw();

		collision_shape_sync(&paddle_bosses[0]);
		collision_shape_sync(&paddle_bosses[1]);

		fire_timer += dt;
		for (uint32_t bullet_index = 0; bullet_index < MAX_BULLETS; ++bullet_index) {
			Entity *bullet = &bullets[bullet_index];
			if (bullet->active == false)
				continue;

			bullet->position = Vector2Add(bullet->position, bullet->velocity);
			collision_shape_sync(bullet);

			for (uint32_t boss_index = 0; boss_index < countof(paddle_bosses); ++boss_index) {
				Entity *boss = &paddle_bosses[boss_index];

				if (boss->active) {
					if (CheckCollisionRecs(bullet->collision_shape, paddle_bosses[boss_index].collision_shape)) {
						paddle_bosses[boss_index].flash_timer = 0.1f;
						paddle_bosses[boss_index].tint = RED;

						paddle_bosses[boss_index].health -= bullet_damage;

						if (paddle_bosses[boss_index].health <= 0.0f) {
							paddle_bosses[boss_index].active = false;

							SetSoundPitch(sfx_paddle_death, GetRandomValue(80, 100) / 100.f);
							PlaySound(sfx_paddle_death);
							continue;
						}

						SetSoundPitch(sfx_paddle_hurt, GetRandomValue(80, 100) / 100.f);
						PlaySound(sfx_paddle_hurt);

						bullet->active = false;
						break;
					}
				}
			}
			if (bullet->active == false)
				continue;

			bullet->life_timer -= dt;
			if (bullet->life_timer <= 0.0f)
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

					*bullet = (Entity){
						.active = true,
						.life_timer = BULLET_LIFTIME,
						.position = { spawn_position.x, spawn_position.y },
						.velocity = { aim_direction.x * bullet_speed, aim_direction.y * bullet_speed },
						.size = { 5.f, 10.f },
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

				animation_timer += dt;
				if (animation_timer >= ANIMATION_SPEED) {
					player_frame = (player_frame + 1) % 4;
					animation_timer = 0.0f;
				}

				player.area.x = TILE_SIZE * player_frame;

                // TODO: Proper rocket sound
				// if (IsSoundPlaying(sfx_player_rocket) == false) {
				// 	SetSoundPitch(sfx_player_shoot, GetRandomValue(80, 100) / 100.f);
				// 	PlaySound(sfx_player_rocket);
				// }

			} else {
				player_frame = 0;
				player.area.x = 0;

                // StopSound(sfx_player_rocket);
			}

			player.position = Vector2Add(player.position, player.velocity);
			player.velocity = Vector2Scale(player.velocity, drag);

			collision_shape_sync(&player);

			for (uint32_t boss_index = 0; boss_index < countof(paddle_bosses); ++boss_index) {
				Entity *boss = &paddle_bosses[boss_index];

				if (boss->active) {
					boss->position = Vector2Add(boss->position, Vector2Scale(boss->velocity, 3.f));

					float half_w = boss->size.x * .5f;
					float half_h = boss->size.y * .5f;

					if (boss->position.y < half_h)
						boss->velocity.y = -boss->velocity.y;
					else if (boss->position.y > WINDOW_HEIGHT - half_h)
						boss->velocity.y = -boss->velocity.y;

					if (CheckCollisionRecs(player.collision_shape, paddle_bosses[boss_index].collision_shape)) {
						player.respawn_timer = 1.f;
						player.active = false;

						SetSoundPitch(sfx_player_death, GetRandomValue(80, 100) / 100.f);
						PlaySound(sfx_player_death);
					}
				}
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

			entity_draw(player);

		} else {
			player.respawn_timer -= dt;
			if (player.respawn_timer <= 0.0f) {
				player.respawn_timer = 0.0f;
				player.position = (Vector2){ .x = WINDOW_WIDTH * .5f, .y = WINDOW_HEIGHT * .5f };
				player.velocity = (Vector2){ 0 };
				player.rotation = 0;
				player.active = true;
			}
		}

		for (uint32_t boss_index = 0; boss_index < countof(paddle_bosses); ++boss_index) {
			Entity *boss = &paddle_bosses[boss_index];

			if (boss->active) {
				if (boss->flash_timer > 0.0f) {
					boss->flash_timer -= dt;

					if (boss->flash_timer <= 0.0f) {
						boss->flash_timer = 0.0f;
						boss->tint = WHITE;
					}
				}

				if (boss->flash_timer) {
					BeginShaderMode(flash_shader);
					entity_draw(paddle_bosses[boss_index]);
					EndShaderMode();

				} else
					entity_draw(paddle_bosses[boss_index]);
			}
		}

		for (uint32_t index = 0; index < MAX_BULLETS; ++index) {
			if (bullets[index].active == true)
				entity_draw(bullets[index]);
		}

		Rectangle health_bar = { WINDOW_HEIGHT * .25f, 20.f, WINDOW_WIDTH * 2.f / 3.f, 25.f };
		float paddle_boss_health = paddle_bosses[0].health + paddle_bosses[1].health;
		Rectangle boss_health_bar = { health_bar.x, health_bar.y, health_bar.width * paddle_boss_health / paddle_boss_max_health, health_bar.height };

		DrawRectangleRec(health_bar, RAYWHITE);
		DrawRectangleRec(boss_health_bar, RED);

		if (collision_shape_toggle) {
			DrawRectangleLinesEx(player.collision_shape, 1.f, RED);
			DrawRectangleLinesEx(paddle_bosses[0].collision_shape, 1.f, RED);
			DrawRectangleLinesEx(paddle_bosses[1].collision_shape, 1.f, RED);
		}

		if (ui_toggle) {
			rotation_speed = gui_slider(&frame_arena, S("Turn Speed"), rotation_speed, 1.0f, 10.0f, 20, 50, 200);
			acceleration = gui_slider(&frame_arena, S("Engine Power"), acceleration, 0.01f, 1.0f, 20, 80, 200);
			drag = gui_slider(&frame_arena, S("Friction (Drag)"), drag, 0.90f, 1.0f, 20, 110, 200);
		}

		if (IsKeyPressed(KEY_TAB))
			ui_toggle = !ui_toggle;
		if (IsKeyPressed(KEY_C))
			collision_shape_toggle = !collision_shape_toggle;

		EndDrawing();

		arena_reset(&frame_arena);
	}

	CloseWindow();
}

void entity_draw(Entity sprite) {
	Rectangle dest = {
		.x = sprite.position.x,
		.y = sprite.position.y,
		.width = sprite.size.x,
		.height = sprite.size.y
	};

	if (sprite.texture) {
		Vector2 origin = { sprite.size.x * .5f, sprite.size.y * .5f };
		DrawTexturePro(
			*sprite.texture,
			sprite.area,
			dest,
			origin,
			sprite.rotation,
			sprite.tint);

	} else
		DrawRectanglePro(dest, (Vector2){ sprite.size.x * .5f, sprite.size.y * .5f }, sprite.rotation, sprite.tint);

	// DrawCircleV(sprite.position, 5.f, RED);
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

void collision_shape_sync(Entity *sprite) {
	sprite->collision_shape.x = sprite->position.x - sprite->collision_shape.width * .5f;
	sprite->collision_shape.y = sprite->position.y - sprite->collision_shape.height * .5f;
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
