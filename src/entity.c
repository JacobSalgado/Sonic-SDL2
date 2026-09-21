#include "simple_logger.h"

#include "camera.h"
#include "world.h"
#include "entity.h"
#include "collision.h"
#include "spring.h"

extern int player_is_jumping(Entity* player);

typedef struct
{
	Entity *entity_list;
	Uint32	entity_max;
}EntityManager;

void entity_system_close(void);

static EntityManager _entity_manager = { 0 }; /**<initialize a LOCAL global entity manager*/
static World* _active_world = NULL;

static Entity* _player = NULL;

void entity_set_player(Entity* player)
{
	_player = player;
}

Entity* entity_get_player(void)
{
	return _player;
}

void entity_system_initialize(Uint32 max)
{
	if (_entity_manager.entity_list)
	{
		slog("cannot have two instances of an entity manager, one is already active");
		return;
	}

	if (!max)
	{
		slog("cannot allocate 0 entities!");
		return;
	}
	_entity_manager.entity_list = gfc_allocate_array(sizeof(Entity), max);
	if (!_entity_manager.entity_list)
	{
		slog("failed to allocate global entity list");
		return;
	}
	_entity_manager.entity_max = max;
	slog("entity system intialized");
	atexit(entity_system_close);
}

void entity_system_close()
{
	entity_clear_all(NULL);
	if (_entity_manager.entity_list)free(_entity_manager.entity_list);
	memset(&_entity_manager, 0, sizeof(EntityManager));
}

void entity_clear_all(Entity* ignore)
{
	int i;
	for (i = 0; i < _entity_manager.entity_max; i++)
	{
		if (&_entity_manager.entity_list[i] == ignore) continue;
		if (!_entity_manager.entity_list[i]._inuse) continue; // skip this iteration of the loop
		entity_free(&_entity_manager.entity_list[i]);
		_entity_manager.entity_list[i]._inuse = 0;
	}
}

Entity* entity_new()
{
	int i;
	if (!_entity_manager.entity_list)
	{
		slog("entity manager not initialized");
		return NULL;
	}

	for (i = 0; i < _entity_manager.entity_max; i++)
	{
		if (_entity_manager.entity_list[i]._inuse) continue; // skip any active entities
		memset(&_entity_manager.entity_list[i], 0, sizeof(Entity));
		_entity_manager.entity_list[i]._inuse = 1;
		_entity_manager.entity_list[i].scale = gfc_vector2d(1, 1); // default scale
		// setting the default color
		//_entity_manager.entity_list[i].scale = gfc_vector2d(1,1);
		return &_entity_manager.entity_list[i];
	}
	slog("no more available entities");
	return NULL;
}

void entity_free(Entity* self)
{
	if (!self) return; // can't do work on a pointer that doesn't exist
	gf2d_sprite_free(self->sprite);
	// anything else you allocate for an entity would get cleaned up here
	if (self->free)self->free(self->data);
}

void entity_destroy(Entity* self)
{
	entity_free(self);
	entity_manager_remove(self);
}

void entity_manager_remove(Entity* self)
{
	int i;
	for (i = 0; i < _entity_manager.entity_max; i++)
	{
		if (&_entity_manager.entity_list[i] == self)
		{
			_entity_manager.entity_list[i]._inuse = 0;
			return;
		}
	}
}

void entity_check_collisions(Entity* player)
{
	int i;
	for (i = 0; i < _entity_manager.entity_max; i++)
	{
		if (!_entity_manager.entity_list[i]._inuse) continue; // if the entity is gone, SKIP
		//if (&_entity_manager.entity_list[i] == player) continue;
		if (_entity_manager.entity_list[i].type != ENTITY_TYPE_ENEMY) continue; 
		//check_collision(player, &_entity_manager.entity_list[i]);
	}
}

/*void entity_surface_collision(World* world, Entity* player)
{
	check_surface_collision(world, player);
}*/

void entity_think(Entity* self)
{
	if (!self) return;
	// any boilerplate think stuff here
	if (self->think)self->think(self);
}

void entity_system_think(void)
{
	int i;
	for (i = 0; i < _entity_manager.entity_max; i++)
	{
		if (!_entity_manager.entity_list[i]._inuse) continue; // skip any inactive entities
		entity_think(&_entity_manager.entity_list[i]);
	}
}

void entity_update(Entity* self)
{
	if (!self) return;
	// any boilerplate update stuff here
	apply_gravity(self, 0.016f);

	if (self->update)self->update(self);

	if (_active_world)
	{
		self->position.x += self->velocity.x;
		resolveTileCollisionX(self, _active_world);
		resolveTerrainCollisionX(self, _active_world);

		self->position.y += self->velocity.y;
		resolveTileCollisionY(self, _active_world);
		resolveTerrainCollisionY(self, _active_world);
		resolveSlopeCollision(self, _active_world);
	}
}

void entity_system_update(void)
{
	int i;
	for (i = 0; i < _entity_manager.entity_max; i++)
	{
		if (!_entity_manager.entity_list[i]._inuse) continue; // skip any inactive entities
		entity_update(&_entity_manager.entity_list[i]);
	}
}

void entity_draw(Entity* self)
{
	GFC_Vector2D offset, position;

	if (!self) return;
	offset = camera_get_offset();
	gfc_vector2d_add(position, self->position, offset);

	if (self->sprite)
	{
		gf2d_sprite_draw(
			self->sprite,
			position,
			&self->scale,
			NULL,
			NULL,
			NULL,
			NULL,
			(Uint32)self->frame);
	}
}

void entity_system_draw(void)
{
	int i;
	for (i = 0; i < _entity_manager.entity_max; i++)
	{
		if (!_entity_manager.entity_list[i]._inuse) continue; // skip any inactive entities
		entity_draw(&_entity_manager.entity_list[i]);
	}
}

void apply_gravity(Entity* self, float deltaTime)
{
	float gravity = 9.81f; // gravity value 

	if (!self->gravityOn) // for flying entites
		gravity = 0;

	if (!self->onGround)
		self->velocity.y += gravity * deltaTime;
}

void entity_system_set_world(World* world)
{
	_active_world = world;
}

void check_spring_collision(Entity* player)
{
	int i;
	Entity* ent;

	for (i = 0; i < _entity_manager.entity_max; i++)
	{
		ent = &_entity_manager.entity_list[i];
		if (!ent->_inuse) continue;
		if (ent->type != ENTITY_TYPE_SPRING) continue;
		if (!check_entity_overlap(player, ent)) continue;

		SpringData* data = (SpringData*)ent->data;
		player->velocity.y = data->launch_velocity;
		player->onGround = 0;
	}
}

int enemy_collide_check(Entity* player)
{
	int i;
	GFC_Shape player_rect, enemy_rect;
	Entity* enemy;

	if (!player) return 0;
	if (!player_is_jumping(player)) return 0; // collision will only work if player is jumping

	for (i = 0; i < _entity_manager.entity_max; i++)
	{
		enemy = &_entity_manager.entity_list[i];

		if (!enemy->_inuse) continue;
		if (enemy->type != ENTITY_TYPE_ENEMY) continue;

		player_rect = gfc_shape_rect(player->position.x, player->position.y, player->sprite->frame_w, player->sprite->frame_h);
		enemy_rect = gfc_shape_rect(enemy->position.x, enemy->position.y, enemy->sprite->frame_w, enemy->sprite->frame_h);

		if (!gfc_shape_overlap(player_rect, enemy_rect)) continue;

		// bounce mechanic lol
		player->velocity.y = -7.0f;
		player->onGround = 0;

		entity_destroy(enemy);
		return 1;
	}
	return 0;
}

Uint32 entity_system_get_max(void)
{
	return _entity_manager.entity_max;
}

Entity* entity_system_get(int index)
{
	if (index < 0 || index >= _entity_manager.entity_max) return NULL;
	return &_entity_manager.entity_list[index];
}
