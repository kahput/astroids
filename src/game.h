#pragma once

#include <raylib.h>

#include "common.h"
#include "core/arena.h"

typedef struct {
    Arena frame_arena;

	float dt;
	uint32_t window_width, window_height;

	float tile_size;

	Shader flash_shader;
} GameContext;
