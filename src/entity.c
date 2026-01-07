#include "entity.h"

void entity_update_physics(Entity *entity, float drag, float dt) {
	entity->position = Vector2Add(entity->position, entity->velocity);
	entity->velocity = Vector2Scale(entity->velocity, drag);
}

void entity_sync_collision(Entity *entity) {
	entity->collision_shape.x = entity->position.x - entity->collision_shape.width * .5f;
	entity->collision_shape.y = entity->position.y - entity->collision_shape.height * .5f;
}

void entity_draw(Entity *entity) {
	Rectangle dest = {
		.x = entity->position.x,
		.y = entity->position.y,
		.width = entity->size.x,
		.height = entity->size.y
	};

	if (entity->texture) {
		Vector2 origin = { entity->size.x * .5f, entity->size.y * .5f };
		DrawTexturePro(
			*entity->texture,
			entity->area,
			dest,
			origin,
			entity->rotation,
			entity->tint);

	} else
		DrawRectanglePro(dest, (Vector2){ entity->size.x * .5f, entity->size.y * .5f }, entity->rotation, entity->tint);

	// DrawCircleV(sprite.position, 5.f, RED);
}
