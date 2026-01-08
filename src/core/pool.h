#ifndef POOL_H
#define POOL_H

#include "common.h"
#include "core/arena.h"

struct arena;

typedef struct pool_slot {
	struct pool_slot *next;
} PoolSlot;

typedef struct pool {
	PoolSlot *slots, *free_slots;
	usize slot_size;
} Pool;

Pool *allocator_pool(usize slot_size, uint32_t capacity);
Pool *allocator_pool_from_arena(Arena *arena, uint32_t capacity, usize slot_size, usize alignment);
void pool_destroy(Pool *pool);

void *pool_alloc(Pool *pool);
void *pool_alloc_zeroed(Pool *pool);

void pool_free(Pool *pool, void *element);

#endif
