#include "weapon.h"
#include "audio_manager.h"
#include "entity.h"
#include "globals.h"
#include <raylib.h>

bool32 weapon_system_init(BulletSystem *system, Texture *texture) {
	*system = (BulletSystem){ 0 };

	system->base_damage = 1.4f;
	system->texture = texture;

	for (int i = 0; i < MAX_BULLETS; i++)
		system->bullets[i].entity.active = false;

	return true;
}

bool32 weapon_bullet_spawn(BulletSystem *sys, Vector2 spawn_position, float rotation, Vector2 direction, float speed, float damage_multiplier) {
	Bullet *bullet = NULL;

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (sys->bullets[i].entity.active == false) {
			bullet = &sys->bullets[i];
			break;
		}
	}

	if (bullet) {
		bullet->entity = (Entity){
			.active = true,
			.position = spawn_position,
			.velocity = Vector2Scale(direction, speed),
			.rotation = rotation,
			.texture = sys->texture,
			.area = { TILE_SIZE * 6, 0, TILE_SIZE, TILE_SIZE },
			.size = { 16.f, 32.f },
			.tint = ORANGE,
		};

		bullet->damage = sys->base_damage * damage_multiplier;
		bullet->life_timer = 1.0f;

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
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (weapon_system->bullets[i].entity.active) {
			entity_draw(&weapon_system->bullets[i].entity);

            if (show_debug) {
                DrawRectangleLinesEx(weapon_system->bullets[i].entity.collision_shape, 1.0f, GREEN);
            }
        }
	}
}
