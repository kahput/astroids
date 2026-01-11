#include "asteroid.h"
#include "entity.h"
#include "globals.h"
#include <raylib.h>
#include <raymath.h>

#define ASTEROID_SPRITE_OFFSET_X TILE_SIZE * 6
#define ASTEROID_SPRITE_OFFSET_Y TILE_SIZE

static Vector2 variant_sizes[ASTEROID_VARIANT_COUNT] = {
	[ASTEROID_VARIANT_LARGE] = { .x = TILE_SIZE * 4, TILE_SIZE * 4 },
	[ASTEROID_VARIANT_MEDIUM] = { .x = TILE_SIZE * 2, TILE_SIZE * 1 },
	[ASTEROID_VARIANT_SMALL] = { .x = TILE_SIZE, TILE_SIZE },
};

void asteroid_system_init(AsteroidSystem *system, Texture *atlas) {
	*system = (AsteroidSystem){ 0 };
	system->texture = atlas;
	system->spawn_rate = ASTEROID_SPAWN_RATE;
}

static Asteroid *get_free_slot(AsteroidSystem *sys) {
	for (int i = 0; i < MAX_ASTEROIDS; i++) {
		if (!sys->asteroids[i].entity.active)
			return &sys->asteroids[i];
	}
	return NULL;
}

void asteroid_spawn_split(AsteroidSystem *system, Vector2 pos, AsteroidVariant variant) {
	Asteroid *asteroid = get_free_slot(system);
	if (!asteroid)
		return;

	Vector2 size = variant_sizes[variant];

	// Setup Entity
	uint32_t offset_x = ASTEROID_SPRITE_OFFSET_X + (GetRandomValue(0, 1) * TILE_SIZE);
	uint32_t offset_y = ASTEROID_SPRITE_OFFSET_Y + (GetRandomValue(0, 1) * TILE_SIZE);

	asteroid->entity = (Entity){
		.active = true,
		.position = pos,
		.size = size,
		.texture = system->texture,
		.area = (Rectangle){ offset_x, offset_y, TILE_SIZE, TILE_SIZE },
		.tint = GRAY,
		.rotation = GetRandomValue(0, 360),
	};

	Vector2 center = { WINDOW_WIDTH * .5f, WINDOW_HEIGHT * .5f };

	float min = -200;
	float max = 200;
	asteroid->inital_target = Vector2Add(center, (Vector2){ (float)GetRandomValue(min, max), (float)GetRandomValue(min, max) });

	Vector2 direction = Vector2Subtract(asteroid->inital_target, asteroid->entity.position);
	direction = Vector2Normalize(direction);
	float speed = GetRandomValue(ASTEROID_SPEED_MIN, ASTEROID_SPEED_MAX);
	if (variant == ASTEROID_VARIANT_SMALL)
		speed *= 1.5f;

	asteroid->velocity = Vector2Scale(direction, speed);
	asteroid->rotation_speed = GetRandomValue(-90, 90);
	asteroid->variant = variant;

	asteroid->entity.collision_active = true;
	asteroid->entity.collision_shape = (Rectangle){ 0, 0, size.x * 0.8f, size.y * 0.8f };
}

void asteroid_spawn_random(AsteroidSystem *system, int screen_w, int screen_h) {
	int side = GetRandomValue(0, 3);
	Vector2 position = { 0 };

	switch (side) {
		case 0:
			position = (Vector2){ GetRandomValue(0, screen_w), -50 };
			break; // Top
		case 1:
			position = (Vector2){ GetRandomValue(0, screen_w), screen_h + 50 };
			break; // Bottom
		case 2:
			position = (Vector2){ -50, GetRandomValue(0, screen_h) };
			break; // Left
		case 3:
			position = (Vector2){ screen_w + 50, GetRandomValue(0, screen_h) };
			break; // Right
	}

	asteroid_spawn_split(system, position, ASTEROID_VARIANT_LARGE);
	system->large_count++;
}

void asteroid_system_update(AsteroidSystem *system, float dt) {
	// 1. Spawning
	if (system->spawn_rate > 0) {
		system->spawn_timer += dt;
		if (system->spawn_timer > system->spawn_rate && system->large_count < ASTEROID_SPAWN_LIMIT) {
			system->spawn_timer = 0;
			asteroid_spawn_random(system, WINDOW_WIDTH, WINDOW_HEIGHT);
		}
	}

	for (int asteroid_index = 0; asteroid_index < MAX_ASTEROIDS; asteroid_index++) {
		Asteroid *asteroid = &system->asteroids[asteroid_index];
		if (!asteroid->entity.active)
			continue;

		// Move
		asteroid->entity.position = Vector2Add(asteroid->entity.position, Vector2Scale(asteroid->velocity, dt));
		asteroid->entity.rotation += asteroid->rotation_speed * dt;

		// Screen Wrap (Toroidal World)
		float pad = asteroid->entity.size.x; // Allow to go fully offscreen before wrapping
		if (asteroid->entity.position.x < -pad)
			asteroid->entity.position.x = WINDOW_WIDTH + pad;
		if (asteroid->entity.position.x > WINDOW_WIDTH + pad)
			asteroid->entity.position.x = -pad;
		if (asteroid->entity.position.y < -pad)
			asteroid->entity.position.y = WINDOW_HEIGHT + pad;
		if (asteroid->entity.position.y > WINDOW_HEIGHT + pad)
			asteroid->entity.position.y = -pad;

		entity_sync_collision(&asteroid->entity);
	}
}

void asteroid_system_draw(AsteroidSystem *system, bool show_debug) {
	for (int asteroid_index = 0; asteroid_index < MAX_ASTEROIDS; asteroid_index++) {
		if (system->asteroids[asteroid_index].entity.active) {
			Asteroid *asteroid = &system->asteroids[asteroid_index];

			entity_draw(&asteroid->entity);

			if (show_debug && asteroid->entity.collision_active) {
				DrawRectangleLinesEx(asteroid->entity.collision_shape, 1.0f, GREEN);
				DrawLineV(asteroid->entity.position, asteroid->inital_target, RED);
			}
		}
	}
}
