#include "common.h"
#include "core/arena.h"
#include "core/logger.h"
#include "core/astring.h"

#include <math.h>

#include <raylib.h>
#include <raymath.h>

const uint32_t WINDOW_WIDTH = 1280;
const uint32_t WINDOW_HEIGHT = 720;
const uint32_t TILE_SIZE = 32 * 20;
const uint32_t PLAYER_SIZE = 96;

typedef Rectangle TextureArea;

typedef struct {
	Vector2 position;
	Vector2 velocity;
	Vector2 size;
	float rotation;

	TextureArea area;
	Texture2D *texture;
} Sprite;

void sprite_draw(Sprite sprite);
float gui_slider(Arena *arena, String label, float value, float min, float max, float x, float y, float width);

int main(void) {
	Arena frame_arena = arena_create(MiB(4));

	InitWindow(1280, 720, "Astroids");
	SetTargetFPS(60);

	Texture atlas = LoadTexture("assets/sprites/asteroid_sprite.png");

	Sprite player = {
		.position = { .x = WINDOW_WIDTH * .5f, .y = WINDOW_HEIGHT * .5f },
		.velocity = { 0, 0 },
		.size = { .x = PLAYER_SIZE, .y = PLAYER_SIZE },
		.rotation = 0,
		.area = { 0, 0, TILE_SIZE, TILE_SIZE },
		.texture = &atlas
	};

	float rotation_speed = 4.5f;
	float acceleration = 0.2f;
	float drag = 0.99f;

	while (WindowShouldClose() == false) {
		ClearBackground(RAYWHITE);
		BeginDrawing();

		if (IsKeyDown(KEY_D))
			player.rotation += rotation_speed;
		if (IsKeyDown(KEY_A))
			player.rotation -= rotation_speed;

		if (IsKeyDown(KEY_W)) {
			float rad = DEG2RAD * player.rotation;

			Vector2 direction = { sinf(rad), -cosf(rad) };

			player.velocity = Vector2Add(player.velocity, Vector2Scale(direction, acceleration));
		}

		player.position = Vector2Add(player.position, player.velocity);
		player.velocity = Vector2Scale(player.velocity, drag);

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

		sprite_draw(player);

		rotation_speed = gui_slider(&frame_arena, S("Turn Speed"), rotation_speed, 1.0f, 10.0f, 20, 50, 200);
		acceleration = gui_slider(&frame_arena, S("Engine Power"), acceleration, 0.01f, 1.0f, 20, 80, 200);
		drag = gui_slider(&frame_arena, S("Friction (Drag)"), drag, 0.90f, 1.0f, 20, 110, 200);

		EndDrawing();

		arena_reset(&frame_arena);
	}
}

void sprite_draw(Sprite sprite) {
	Rectangle dest = {
		.x = sprite.position.x,
		.y = sprite.position.y,
		.width = sprite.size.x,
		.height = sprite.size.y
	};

	DrawTexturePro(
		*sprite.texture,
		sprite.area,
		dest,
		(Vector2){ sprite.size.x * .5f, sprite.size.y * .5f },
		sprite.rotation,
		WHITE);

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
	DrawText(label.data, x, y + 5, 10, BLACK); // Label

	DrawRectangleRec(bar_area, LIGHTGRAY); // Background Bar
	DrawRectangleRec(knob_area, RED); // Handle
	DrawRectangleLinesEx(bar_area, 1, BLACK); // Border

	String value_string = string_format(arena, S("%.2f"), value);
	DrawText(value_string.data, x + 80 + width + 10, y + 5, 10, BLACK);

	return value;
}
