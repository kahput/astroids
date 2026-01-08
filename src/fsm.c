#include "fsm.h"

#include "core/debug.h"
#include <string.h>

bool32 fsm_create(FSM *fsm, StateID initial_state, void *context) {
	ASSERT(context != NULL);
	ASSERT(initial_state < MAX_STATES);

	fsm->context = context;
	fsm->current = &fsm->states[initial_state];

	return true;
}

bool32 fsm_state_add(FSM *fsm, StateID id, StateHandler *handler) {
	ASSERT(id < MAX_STATES);
	ASSERT(handler != NULL);

	fsm->states[id].id = id;
	memcpy(&fsm->states[id].handler, handler, sizeof(StateHandler));

	return true;
}

bool32 fsm_state_remove(FSM *fsm, StateID id) {
	ASSERT(id < MAX_STATES);
	ASSERT(id != fsm->current->id);

	fsm->states[id]
		.id = INVALID_INDEX;
	fsm->states[id].handler = (StateHandler){ 0 };

	return true;
}

bool32 fsm_state_set(FSM *fsm, StateID id) {
	ASSERT(id < MAX_STATES);
	fsm->current = &fsm->states[id];

	if (fsm->current->handler.on_enter)
		fsm->current->handler.on_enter(fsm->context);

	return true;
}

StateID fsm_state_get(FSM *fsm) {
    ASSERT(fsm->current);
	return fsm->current->id;
}

void fsm_context_set(FSM *fsm, void *context) {
	fsm->context = context;
}

bool32 fsm_update(FSM *fsm, float dt) {
	ASSERT(fsm->current);
	StateHandler *current = &fsm->current->handler;
	ASSERT(current->on_update);

	StateID next_id = current->on_update(fsm->context, dt);

	if (next_id < MAX_STATES && next_id != fsm->current->id) {
		StateHandler *next = &fsm->states[next_id].handler;

		if (current->on_exit)
			current->on_exit(fsm->context);
		if (next->on_enter)
			next->on_enter(fsm->context);

		fsm->current = &fsm->states[next_id];

		return true;
	}

	return false;
}
