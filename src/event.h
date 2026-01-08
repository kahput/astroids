#pragma once

#include "common.h"

#define MAX_EVENT_SIZE 128
#define MAX_EVENT_TYPES 1024

typedef struct {
	uint32_t type, size;
} EventCommon;

typedef struct {
	EventCommon header;

	uint8_t padding[MAX_EVENT_SIZE - sizeof(EventCommon)];
} Event;
#define EVENT_DEFINE(name) STATIC_ASSERT(sizeof(name) <= sizeof(Event))

typedef bool32 (*PFN_on_event)(Event *event);

bool32 event_system_startup(void);
bool32 event_system_shutdown(void);

// TODO: Implement event queue
// bool32 event_system_update(void);

bool32 event_subscribe(uint16_t event_type, PFN_on_event on_event);
bool32 event_unsubscribe(uint16_t event_type, PFN_on_event on_event);

#define event_create(T, type_id) ((T){ .header = { .type = type_id, .size = sizeof(T) } })
bool32 event_emit(Event *event);

typedef enum {
	CORE_EVENT_NULL = 0,

	CORE_EVENT_QUIT,
	CORE_EVENT_WINDOW_RESIZED,

	CORE_EVENT_KEY_PRESSED,
	CORE_EVENT_KEY_RELEASED,

	CORE_EVENT_MOUSE_MOTION,
	CORE_EVENT_MOUSE_BUTTON_PRESSED,
	CORE_EVENT_MOUSE_BUTTON_RELEASED,

	CORE_EVENT_COUNT = 0xFF
} CoreEvent;
