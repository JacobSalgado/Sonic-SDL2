#include "simple_logger.h"
#include "gfc_shape.h"
#include "entity.h"
#include "goalpost.h"
//#include "player.h"

static Entity* _player = NULL;

extern void request_transition(const char* next_level);

void goalpost_think(Entity* self)
{
	GoalPostData* data;
	GFC_Shape player_rect, post_rect;

	if (!self || !self->data) return;
	data = (GoalPostData*)self->data;

	if (data->triggered) return;

	_player = entity_get_player();
	if (!_player)
	{
		slog("goalpost_think: player is NULL");
		return;
	}

	post_rect = gfc_shape_rect(
		self->position.x,
		self->position.y,
		self->sprite->frame_w,
		self->sprite->frame_h
	);
	player_rect = gfc_shape_rect(
		_player->position.x,
		_player->position.y,
		_player->sprite->frame_w,
		_player->sprite->frame_h
	);

	if (gfc_shape_overlap(post_rect, player_rect))
	{
		slog("goalpost triggered, transitioning to %s", data->next_level);
		data->triggered = 1;
		self->frame = 1;
		request_transition(data->next_level);
	}
}

void goalpost_free(void* data)
{
	if (data) free(data);
}

Entity* goalpost_new(float x, float y, const char* next_level)
{
	Entity* self;
	GoalPostData* data;

	self = entity_new();
	if (!self)
	{
		slog("goalpost_new: failed to allocate entity");
		return NULL;
	}

	data = (GoalPostData*)malloc(sizeof(GoalPostData));
	if (!data)
	{
		slog("goalpost_new: failed to allocate data");
		return NULL;
	}
	memset(data, 0, sizeof(GoalPostData));
	snprintf(data->next_level, 256, "%s", next_level);

	self->sprite = gf2d_sprite_load_all(
		"images/goalpost.png",
		216,
		194,
		1,
		0
	);
	self->position = gfc_vector2d(x, y);
	self->scale = gfc_vector2d(1, 1);
	self->frame = 0;
	self->gravityOn = 0;
	self->type = ENTITY_TYPE_GOALPOST;
	self->data = data;
	self->think = goalpost_think;
	self->data = goalpost_free;

	return self;
}
