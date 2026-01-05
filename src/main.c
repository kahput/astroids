#include "common.h"
#include "core/logger.h"
#include "core/astring.h"

#include <math.h>

#include <raylib.h>
#include <raymath.h>

const uint32_t WINDOW_WIDTH = 1280;
const uint32_t WINDOW_HEIGHT = 720;
const uint32_t TILE_SIZE = 32 * 20;
const uint32_t PLAYER_SIZE = 128;

typedef struct {
	Vector2 v1;
	Vector2 v2;
	Vector2 v3;
} Triangle;

Triangle triangle_from_direction(Vector2 position, uint32_t base_length, float rad);

int main(void) {
	InitWindow(1280, 720, "Astroids");

	SetTargetFPS(60);

	Vector2 position = { .x = (WINDOW_WIDTH * .5f), .y = (WINDOW_HEIGHT * .5f) };
	uint32_t base_length = 30;
	float rad = 3 * PI / 2.f;

	LOG_INFO("cos(rad) = %.2f, sin(rad) = %.2f", cos(rad), sin(rad));

	Texture texture = LoadTexture("assets/sprites/asteroid_sprite.png");

	Rectangle source = {
		.x = 0,
		.y = 0,
		.width = TILE_SIZE,
		.height = TILE_SIZE
	};

	while (WindowShouldClose() == false) {
		ClearBackground(RAYWHITE);
		BeginDrawing();

		// DrawTexture(texture, 0, 0, WHITE);

		Rectangle dest = {
			.x = (WINDOW_WIDTH - PLAYER_SIZE) * .5f,
			.y = (WINDOW_HEIGHT - PLAYER_SIZE) * .5f,
			.width = PLAYER_SIZE,
			.height = PLAYER_SIZE
		};
		DrawTexturePro(texture, source, dest, (Vector2){ PLAYER_SIZE * .5f, PLAYER_SIZE * .5f }, GetTime() * 100.f, WHITE);

		EndDrawing();
	}
}

Triangle triangle_from_direction(Vector2 position, uint32_t base_length, float rad) {
}
