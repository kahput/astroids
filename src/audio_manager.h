#pragma once

#include "common.h"

typedef enum {
	SFX_PLAYER_SHOOT,
	SFX_PLAYER_DEATH,
	SFX_PADDLE_HURT,
	SFX_PADDLE_DEATH,
	SFX_BOSS_SIREN,

	SFX_COUNT
} SoundId;

typedef enum {
	LOOP_PLAYER_ROCKET,
	LOOP_COUNT
} LoopId;

void audio_initialize(void);
void audio_unload(void);

void audio_update(float dt);

void audio_sfx_play(SoundId id, float volume, bool32 varying_pitch);
void audio_loop_play(LoopId id);
void audio_loop_stop(LoopId id);

void audio_loop_set_pitch(LoopId id, float pitch);
void audio_loop_set_volume(LoopId id, float volume);
