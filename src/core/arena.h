#pragma once

#include "common.h"
typedef struct arena {
	usize offset, capacity;
	void *memory;
} Arena;
typedef struct {
	struct arena *arena;
	usize position;
} ArenaTemp;

Arena arena_create(usize size);
Arena arena_create_from_memory(void *buffer, usize size);
void arena_destroy(Arena *arena);

void *arena_push(Arena *arena, usize size, usize alignment, bool32 zero_memory);

void arena_pop(Arena *arena, usize size);
void arena_set(Arena *arena, usize position);

usize arena_size(Arena *arena);
void arena_reset(Arena *arena);

ArenaTemp arena_begin_temp(Arena *);
void arena_end_temp(ArenaTemp temp);

ArenaTemp arena_scratch(Arena *conflict);
#define arena_release_scratch(scratch) arena_end_temp(scratch)

#define arena_push_array(arena, type, count) ((type *)arena_push((arena), sizeof(type) * (count), alignof(type), false))
#define arena_push_array_zero(arena, type, count) ((type *)arena_push((arena), sizeof(type) * (count), alignof(type), true))
#define arena_push_struct(arena, type) ((type *)arena_push((arena), sizeof(type), alignof(type), false))
#define arena_push_struct_zero(arena, type) ((type *)arena_push((arena), sizeof(type), alignof(type), true))
