#include "asteroid.h"
#include "entity.h"
#include "globals.h"
#include <raylib.h>

static Vector2 variant_sizes[ASTEROID_VARIANT_COUNT] = {
	[ASTEROID_VARIANT_LARGE] = { .x = TILE_SIZE * 4, TILE_SIZE * 4 },
	[ASTEROID_VARIANT_MEDIUM] = { .x = TILE_SIZE * 2, TILE_SIZE * 1 },
	[ASTEROID_VARIANT_SMALL] = { .x = TILE_SIZE, TILE_SIZE },
};

void asteroid_system_init(AsteroidSystem *sys, Texture *atlas) {
	*sys = (AsteroidSystem){ 0 };
	sys->texture = atlas;
	sys->spawn_rate = 2.0f; // Spawn one every 2 seconds initially
}

// Helper to find free slot
static Asteroid *get_free_slot(AsteroidSystem *sys) {
	for (int i = 0; i < MAX_ASTEROIDS; i++) {
		if (!sys->asteroids[i].entity.active)
			return &sys->asteroids[i];
	}
	return NULL;
}

void asteroid_spawn_split(AsteroidSystem *sys, Vector2 pos, AsteroidVariant variant) {
	Asteroid *a = get_free_slot(sys);
	if (!a)
		return;

	Vector2 size = variant_sizes[variant];

	// Setup Entity
	a->entity = (Entity){
		.active = true,
		.position = pos,
		.size = size,
		.area = (Rectangle){ 0, 0, size.x, size.y },
		.texture = NULL,
		.tint = GRAY,
		.rotation = GetRandomValue(0, 360),
	};

	Vector2 direction = { (float)GetRandomValue(-100, 100), (float)GetRandomValue(-100, 100) };
	direction = Vector2Normalize(direction);
	float speed = GetRandomValue(ASTEROID_SPEED_MIN, ASTEROID_SPEED_MAX);
	if (variant == ASTEROID_VARIANT_SMALL)
		speed *= 1.5f;

	a->velocity = Vector2Scale(direction, speed);
	a->rotation_speed = GetRandomValue(-90, 90);
	a->variant = variant;

	a->entity.collision_shape = (Rectangle){ 0, 0, size.x * 0.8f, size.y * 0.8f };
}

void asteroid_spawn_random(AsteroidSystem *sys, int screen_w, int screen_h) {
	int side = GetRandomValue(0, 3);
	Vector2 pos = { 0 };

	switch (side) {
		case 0:
			pos = (Vector2){ GetRandomValue(0, screen_w), -50 };
			break; // Top
		case 1:
			pos = (Vector2){ GetRandomValue(0, screen_w), screen_h + 50 };
			break; // Bottom
		case 2:
			pos = (Vector2){ -50, GetRandomValue(0, screen_h) };
			break; // Left
		case 3:
			pos = (Vector2){ screen_w + 50, GetRandomValue(0, screen_h) };
			break; // Right
	}

	asteroid_spawn_split(sys, pos, ASTEROID_VARIANT_LARGE);

	// Nudge velocity towards center so they don't drift away forever
	// (Optional optimization: calculate vector to center screen)
}

void asteroid_system_update(AsteroidSystem *sys, float dt) {
	// 1. Spawning
	if (sys->spawn_rate > 0) { // If 0, spawning is disabled (Boss Mode)
		sys->spawn_timer += dt;
		if (sys->spawn_timer > sys->spawn_rate) {
			sys->spawn_timer = 0;
			asteroid_spawn_random(sys, WINDOW_WIDTH, WINDOW_HEIGHT);
		}
	}

	// 2. Physics & Wrapping
	for (int i = 0; i < MAX_ASTEROIDS; i++) {
		Asteroid *a = &sys->asteroids[i];
		if (!a->entity.active)
			continue;

		// Move
		a->entity.position = Vector2Add(a->entity.position, Vector2Scale(a->velocity, dt));
		a->entity.rotation += a->rotation_speed * dt;

		// Screen Wrap (Toroidal World)
		float pad = a->entity.size.x; // Allow to go fully offscreen before wrapping
		if (a->entity.position.x < -pad)
			a->entity.position.x = WINDOW_WIDTH + pad;
		if (a->entity.position.x > WINDOW_WIDTH + pad)
			a->entity.position.x = -pad;
		if (a->entity.position.y < -pad)
			a->entity.position.y = WINDOW_HEIGHT + pad;
		if (a->entity.position.y > WINDOW_HEIGHT + pad)
			a->entity.position.y = -pad;

		entity_sync_collision(&a->entity);
	}
}

void asteroid_system_draw(AsteroidSystem *system, bool show_debug) {
	for (int asteroid_index = 0; asteroid_index < MAX_ASTEROIDS; asteroid_index++) {
		if (system->asteroids[asteroid_index].entity.active) {
			entity_draw(&system->asteroids[asteroid_index].entity);

			if (show_debug) {
				DrawRectangleLinesEx(system->asteroids[asteroid_index].entity.collision_shape, 1.0f, GREEN);
			}
		}
	}
}
