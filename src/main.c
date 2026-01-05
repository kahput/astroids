#include "common.h"
#include "core/logger.h"

#include <raylib.h>

const uint32_t WINDOW_WIDTH = 1280;
const uint32_t WINDOW_HEIGHT = 720;

int main(void) {
	InitWindow(1280, 720, "Astroids");

    SetTargetFPS(60);

	while (WindowShouldClose() == false) {
		ClearBackground(RAYWHITE);
		BeginDrawing();

		uint32_t string_length = MeasureText("Hello, world!", 64);
		LOG_INFO("StringLength = %d", string_length);
		DrawText("Hello, world!", ((WINDOW_WIDTH - string_length) / 2) , WINDOW_HEIGHT / 2, 64, BLACK);

		EndDrawing();
	}
}
