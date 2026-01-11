#include "weapon.h"
#include "audio_manager.h"
#include "entity.h"
#include "globals.h"
#include <raylib.h>

bool32 weapon_system_init(BulletSystem *system, Texture *texture) {
	*system = (BulletSystem){ 0 };

	system->base_damage = 1.4f;
	system->texture = texture;

	for (int bullet_index = 0; bullet_index < MAX_BULLETS; bullet_index++)
		system->bullets[bullet_index].entity.active = false;

	return true;
}

bool32 weapon_bullet_spawn(BulletSystem *system, Vector2 spawn_position, float rotation, Vector2 direction, float speed, float damage_multiplier) {
	Bullet *bullet = NULL;

	for (int bullet_index = 0; bullet_index < MAX_BULLETS; bullet_index++) {
		if (system->bullets[bullet_index].entity.active == false) {
			bullet = &system->bullets[bullet_index];
			break;
		}
	}

	if (bullet) {
		bullet->entity = (Entity){
			.active = true,
			.position = spawn_position,
			.velocity = Vector2Scale(direction, speed),
			.rotation = rotation,
			.texture = system->texture,
			.area = { TILE_SIZE * 5, TILE_SIZE * 4, TILE_SIZE, TILE_SIZE },
			.size = { 16.f, 32.f },
			.tint = ORANGE,
		};

		bullet->damage = system->base_damage * damage_multiplier;
		bullet->life_timer = 1.0f;

		bullet->entity.collision_active = true;
		bullet->entity.collision_shape = (Rectangle){ 0, 0, bullet->entity.size.x, bullet->entity.size.y };

		audio_sfx_play(SFX_PLAYER_SHOOT, 1.0f, true);
		return true;
	}

	return false;
}

void weapon_bullets_update(BulletSystem *sys, float dt) {
	for (int i = 0; i < MAX_BULLETS; i++) {
		Bullet *b = &sys->bullets[i];
		if (!b->entity.active)
			continue;

		entity_update_physics(&b->entity, 1.0f, dt);

		b->life_timer -= dt;
		if (b->life_timer <= 0.0f) {
			b->entity.active = false;
			continue;
		}

		float half_w = b->entity.size.x * .5f;
		float half_h = b->entity.size.y * .5f;

		if (b->entity.position.x < -half_w)
			b->entity.position.x = WINDOW_WIDTH + half_w;
		else if (b->entity.position.x > WINDOW_WIDTH + half_w)
			b->entity.position.x = -half_w;

		if (b->entity.position.y < -half_h)
			b->entity.position.y = WINDOW_HEIGHT + half_h;
		else if (b->entity.position.y > WINDOW_HEIGHT + half_h)
			b->entity.position.y = -half_h;

		entity_sync_collision(&b->entity);
	}
}

void weapon_bullets_draw(BulletSystem *weapon_system, bool show_debug) {
	for (int bullet_index = 0; bullet_index < MAX_BULLETS; bullet_index++) {
		Bullet *bullet = &weapon_system->bullets[bullet_index];
		if (bullet->entity.active) {
			entity_draw(&bullet->entity);

			if (show_debug && bullet->entity.collision_active) {
				DrawRectangleLinesEx(bullet->entity.collision_shape, 1.0f, GREEN);
			}
		}
	}
}
