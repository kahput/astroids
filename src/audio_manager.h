#pragma once

#include "common.h"

typedef enum {
	SFX_PLAYER_SHOOT,
	SFX_PLAYER_DEATH,

	SFX_PADDLE_HURT,
	SFX_PADDLE_DEATH,

	SFX_BOSS_WARNING,

	SFX_COUNT
} SoundID;

typedef enum {
	LOOP_PLAYER_ROCKET,
	LOOP_COUNT
} LoopID;

typedef enum {
	MUSIC_BOSS_PADDLE,

	MUSIC_COUNT,
} MusicID;

void audio_initialize(void);
void audio_unload(void);

void audio_update(float dt);

void audio_sfx_play(SoundID id, float volume, bool32 varying_pitch);
void audio_loop_play(LoopID id);
void audio_loop_stop(LoopID id);

void audio_music_play(MusicID id);
void audio_music_stop(MusicID id);
void audio_music_stop_all(void);

void audio_loop_set_pitch(LoopID id, float pitch);
void audio_loop_set_volume(LoopID id, float volume);
