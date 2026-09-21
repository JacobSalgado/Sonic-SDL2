#include "simple_logger.h"

#include "projectile.h"

void projectile_think(Entity* self);
void projectile_update(Entity* self);
void projectile_free(void* data);

Entity* projectile_new(GFC_Vector2D position, GFC_Vector2D direction, float speed)
{
	Entity* self = entity_new();
	if (!self)
	{
		slog("failed to spawn projectile");
		return NULL;
	}

	ProjectileData* data = gfc_allocate_array(sizeof(ProjectileData), 1);
	if (!data)
	{
		entity_destroy(self);
		return NULL;
	}

	data->lifetime = 3.0f;
	data->timer = 0.0f;
	data->damage = 1;

	self->data = data;
	self->position = position;
	self->velocity.x = direction.x * speed;
	self->velocity.y = direction.y * speed;
	self->type = ENTITY_TYPE_PROJECTILE;
	self->think = projectile_think;
	self->update = projectile_update;
	self->data = projectile_free;

	self->scale = gfc_vector2d(3.0f, 3.0f);

	self->sprite = gf2d_sprite_load_all(
		"images/projectiles.png",
		20,
		20,
		1,
		0
	);

	self->gravityOn = false;

	return self;
}

void projectile_think(Entity* self)
{
	if (!self) return;
	ProjectileData* data = (ProjectileData*)self->data;

	data->timer += 0.016f;// 0.016 is the delta time - IMPORTANT NOTE
	if (data->timer >= data->lifetime)
	{
		entity_destroy(self);
	}
}
void projectile_update(Entity* self)
{
	if (!self) return;
}
void projectile_free(void* data)
{
	if (data) free(data);
}
