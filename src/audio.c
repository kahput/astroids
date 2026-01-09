#include "audio_manager.h"
#include "core/debug.h"
#include "core/logger.h"
#include <raylib.h>

typedef struct {
	Sound sound;
	float max_volume;
	float volume; // Current volume (0.0 to 1.0)
	bool32 active; // Target state (true = fade in, false = fade out)
	float fade_speed; // How fast this specific sound changes
} LoopState;

typedef struct {
	Sound clips[SFX_COUNT];
	LoopState loops[LOOP_COUNT];
	Music music[MUSIC_COUNT];
} AudioSystem;

static AudioSystem audio = { 0 };

static Sound load_sound(const char *path) {
	if (!FileExists(path))
		return (Sound){ 0 };
	return LoadSound(path);
}

void audio_initialize(void) {
	InitAudioDevice();

	audio.clips[SFX_PLAYER_SHOOT] = load_sound("assets/sfx/shoot.wav");
	audio.clips[SFX_PLAYER_DEATH] = load_sound("assets/sfx/player_death.wav");
	audio.clips[SFX_PADDLE_HURT] = load_sound("assets/sfx/paddle_hurt.wav");
	audio.clips[SFX_PADDLE_DEATH] = load_sound("assets/sfx/paddle_death.wav");
	audio.clips[SFX_BOSS_WARNING] = load_sound("assets/sfx/boss_siren.wav");

	audio.loops[LOOP_PLAYER_ROCKET].sound = load_sound("assets/sfx/rocket_loop.wav");
	audio.loops[LOOP_PLAYER_ROCKET].fade_speed = 5.0f;

	audio.music[MUSIC_BOSS_PADDLE] = LoadMusicStream("assets/music/boss_music.wav");

	for (int i = 0; i < LOOP_COUNT; i++) {
		audio.loops[i].volume = 0.0f;
		audio.loops[i].max_volume = 1.0f;
		audio.loops[i].active = false;
	}
}

void audio_unload(void) {
	for (int i = 0; i < SFX_COUNT; i++)
		UnloadSound(audio.clips[i]);
	for (int i = 0; i < LOOP_COUNT; i++)
		UnloadSound(audio.loops[i].sound);

	CloseAudioDevice();
}

void audio_update(float dt) {
	for (int i = 0; i < LOOP_COUNT; i++) {
		LoopState *loop = &audio.loops[i];

		float target = loop->active ? 1.0f : 0.0f;

		if (loop->volume < target) {
			loop->volume += loop->fade_speed * dt;
			if (loop->volume > target)
				loop->volume = target;
		} else if (loop->volume > target) {
			loop->volume -= loop->fade_speed * dt;
			if (loop->volume < target)
				loop->volume = target;
		}

		if (loop->volume > 0.01f) {
			if (!IsSoundPlaying(loop->sound))
				PlaySound(loop->sound);
			SetSoundVolume(loop->sound, loop->volume * loop->max_volume);
		} else {
			if (IsSoundPlaying(loop->sound))
				StopSound(loop->sound);
			loop->volume = 0.0f;
		}
	}

	for (uint32_t index = 0; index < MUSIC_COUNT; ++index) {
		Music *music = &audio.music[index];

		UpdateMusicStream(*music);
	}
}

void audio_sfx_play(SoundID id, float volume, bool32 varying_pitch) {
	if (id < SFX_COUNT && IsSoundPlaying(audio.clips[id]) == false) {
		if (varying_pitch)
			SetSoundPitch(audio.clips[id], GetRandomValue(90, 100) / 100.0f);
		SetSoundVolume(audio.clips[id], volume);
		PlaySound(audio.clips[id]);
	}
}

void audio_music_play(MusicID id) {
	if (id < MUSIC_COUNT && IsMusicStreamPlaying(audio.music[id]) == false)
		PlayMusicStream(audio.music[id]);
}

void audio_music_stop(MusicID id) {
	if (id < MUSIC_COUNT && IsMusicStreamPlaying(audio.music[id]) == true)
		StopMusicStream(audio.music[id]);
}

void audio_music_stop_all(void) {
	for (uint32_t music_index = 0; music_index < MUSIC_COUNT; ++music_index)
		audio_music_stop(music_index);
}

void audio_loop_play(LoopID id) {
	if (id < LOOP_COUNT)
		audio.loops[id].active = true;
}

void audio_loop_stop(LoopID id) {
	if (id < LOOP_COUNT)
		audio.loops[id].active = false;
}

void audio_loop_set_pitch(LoopID id, float pitch) {
	if (id < LOOP_COUNT)
		SetSoundPitch(audio.loops[id].sound, pitch);
}

void audio_loop_set_volume(LoopID id, float volume) {
	if (id < LOOP_COUNT)
		audio.loops[id].max_volume = volume;
}
