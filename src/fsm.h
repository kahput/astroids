#pragma once

#include "common.h"

#define STATE_CHANGE_NONE UINT32_MAX
#define MAX_STATES 32

typedef uint32 StateID;

typedef struct {
	void (*on_enter)(void *context);
	void (*on_exit)(void *context);
	uint32_t (*on_update)(void *context, float dt);
} StateHandler;

typedef struct {
	StateID id;
	StateHandler handler;
} State;

typedef struct {
	State states[MAX_STATES];
	State *current;

	void *context;
} FSM;

bool32 fsm_create(FSM *fsm, StateID initial_state, void *context);

bool32 fsm_state_add(FSM *fsm, StateID id, StateHandler *handler);
bool32 fsm_state_remove(FSM *fsm, StateID id);

bool32 fsm_state_set(FSM *fsm, StateID id);
StateID fsm_state_get(FSM *fsm);

void fsm_context_set(FSM *fsm, void *context);

// returns true on state change, else false
bool32 fsm_update(FSM *fsm, float dt);
