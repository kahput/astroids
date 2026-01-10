#include "player.h"
#include "audio_manager.h"
#include "globals.h"
#include "weapon.h"
#include <raymath.h>

bool32 player_init(Player *player, Texture *texture) {
	*player = (Player){ 0 };

	*player = (Player){
		.entity = {
		  .active = true,
		  .position = { .x = WINDOW_WIDTH * .5f, .y = WINDOW_HEIGHT * .5f },
		  .velocity = { 0, 0 },
		  .size = { .x = PLAYER_SIZE, .y = PLAYER_SIZE },
		  .rotation = 0,
		  .area = { 0, 0, TILE_SIZE, TILE_SIZE },
		  .tint = WHITE,
		  .texture = texture }
	};

	player->animation_frame = 0;
	player->animation_timer = 0.0f;

    player->entity.collision_active = true;
	player->entity.collision_shape = (Rectangle){ 0, 0, .width = PLAYER_SIZE * .6f, .height = PLAYER_SIZE * .7f };

	player->rotation_speed = 4.5f;
	player->acceleration = 0.4f;
	player->drag = 0.95;

	player->info.speed = 15.f;
	player->info.base_damage = 1.4f;
	player->info.fire_rate = .2f;
	player->info.fire_timer = 0.0f;

	return true;
}

void player_update(Player *player, BulletSystem *weapon_system, float dt) {
	if (!player->entity.active)
		return;

	// --- 1. COOLDOWNS ---
	player->info.fire_timer += dt;

	// --- 2. INPUT & PHYSICS ---
	if (IsKeyDown(KEY_D))
		player->entity.rotation += player->rotation_speed;
	if (IsKeyDown(KEY_A))
		player->entity.rotation -= player->rotation_speed;

	if (IsKeyDown(KEY_W)) {
		Vector2 thrust_direction = Vector2Rotate((Vector2){ 0, -1 }, player->entity.rotation * DEG2RAD);
		player->entity.velocity = Vector2Add(player->entity.velocity, Vector2Scale(thrust_direction, player->acceleration));

		// Animation
		if (player->animation_frame == 0) {
			player->animation_frame = 2;
			player->animation_timer = 0;
		}

		player->animation_timer += dt;
		if (player->animation_timer >= ANIMATION_SPEED) {
			player->animation_frame = (player->animation_frame + 1) % 4;
			// Keep strictly to frames 1, 2, 3 for thrusting if that's your sprite layout
			if (player->animation_frame == 0)
				player->animation_frame = 1;
			player->animation_timer = 0.0f;
		}

		player->entity.area.x = TILE_SIZE * player->animation_frame;

		// Audio
		audio_loop_play(LOOP_PLAYER_ROCKET);
		audio_loop_set_pitch(LOOP_PLAYER_ROCKET, 1.0f + (Vector2Length(player->entity.velocity) * 0.05f));
		audio_loop_set_volume(LOOP_PLAYER_ROCKET, .3f);

	} else {
		// Idle
		player->animation_frame = 0;
		player->entity.area.x = 0;
		audio_loop_stop(LOOP_PLAYER_ROCKET);
	}

	entity_update_physics(&player->entity, player->drag, dt);
	float half_w = player->entity.size.x * .5f;
	float half_h = player->entity.size.y * .5f;

	if (player->entity.position.x < -half_w)
		player->entity.position.x = WINDOW_WIDTH + half_w;
	else if (player->entity.position.x > WINDOW_WIDTH + half_w)
		player->entity.position.x = -half_w;

	if (player->entity.position.y < -half_h)
		player->entity.position.y = WINDOW_HEIGHT + half_h;
	else if (player->entity.position.y > WINDOW_HEIGHT + half_h)
		player->entity.position.y = -half_h;
	entity_sync_collision(&player->entity);

	// --- 3. FIRING ---
	if (IsKeyPressed(KEY_SPACE) && player->info.fire_timer >= player->info.fire_rate) {
		Vector2 aim_direction = Vector2Rotate((Vector2){ 0, -1 }, player->entity.rotation * DEG2RAD);
		Vector2 spawn_position = Vector2Add(player->entity.position, Vector2Scale(aim_direction, player->entity.size.y * 0.5f));

		// Speed scaling damage logic
		float player_max_speed = (player->acceleration * player->drag) / (1 - player->drag);
		float t = Vector2Length(player->entity.velocity) / player_max_speed;
		float damage_multiplier = 1.0f + t * 2.0f;

		// Delegate to Weapon System
		weapon_bullet_spawn(weapon_system,
			spawn_position,
			player->entity.rotation,
			aim_direction,
			player->info.speed,
			damage_multiplier);

		player->info.fire_timer = 0.0f;
	}

	// --- 4. INTEGRATION ---
}

void player_draw(Player *player) {
	if (player->entity.active)
		entity_draw(&player->entity);
}

void player_kill(Player *player) {
	player->respawn_timer = 1.f;
	player->entity.active = false;
	audio_loop_stop(LOOP_PLAYER_ROCKET);

	audio_sfx_play(SFX_PLAYER_DEATH, 1.0f, true);
}
