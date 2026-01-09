#pragma once
#include "entity.h"

typedef enum {
	ASTEROID_VARIANT_LARGE,
	ASTEROID_VARIANT_MEDIUM,
	ASTEROID_VARIANT_SMALL,

    ASTEROID_VARIANT_COUNT
} AsteroidVariant;

typedef struct {
	Entity entity;
	AsteroidVariant variant;
	Vector2 velocity;
	float rotation_speed;
} Asteroid;

#define MAX_ASTEROIDS 64

typedef struct {
	Asteroid asteroids[MAX_ASTEROIDS];
	Texture *texture;

	float spawn_timer;
	float spawn_rate;
} AsteroidSystem;

void asteroid_system_init(AsteroidSystem *system, Texture *atlas);
void asteroid_system_update(AsteroidSystem *system, float dt);
void asteroid_system_draw(AsteroidSystem *system, bool show_debug);

void asteroid_spawn_random(AsteroidSystem *system, int screen_w, int screen_h);
void asteroid_spawn_split(AsteroidSystem *system, Vector2 pos, AsteroidVariant tier);
