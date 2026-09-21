#include "simple_logger.h"

#include "gfc_shape.h"

#include "camera.h"

#include "audio.h"

#include "ring.h"

static RingManager _ring_manager = { 0 }; /**<initialize a LOCAL global ring manager*/

void ring_system_init(Uint32 max)
{
	if (_ring_manager.ring_list)
	{
		slog("ring system already initialized");
		return;
	}
	if (!max)
	{
		slog("cannot allocate 0 rings");
		return;
	}
	_ring_manager.ring_list = gfc_allocate_array(sizeof(Entity), max);
	if (!_ring_manager.ring_list)
	{
		slog("failed to allocate ring list");
		return;
	}
	//audio_init();
	_ring_manager.ring_max = max;
	_ring_manager.ring_sound = audio_load_sound("audio/ring_sound.wav");
	slog("ring_sound index: %d", _ring_manager.ring_sound);
	slog("ring system initialized");
	atexit(ring_system_close);
}

void ring_draw(Entity* self)
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

void ring_system_draw()
{
	/*Entity* ring;
	GFC_Vector2D drawPosition;
	GFC_Vector2D camera = camera_get_position();
	for (int i = 0; i < _ring_manager.ring_max; i++)
	{
		ring = &_ring_manager.ring_list[i];
		if (!ring->_inuse || !ring->sprite) continue;

		drawPosition.x = ring->position.x - camera.x;
		drawPosition.y = ring->position.y - camera.y;

		gf2d_sprite_draw(
			ring->sprite,
			ring->position,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			(int)ring->frame
		);
	}*/
	int i;
	for (i = 0; i < _ring_manager.ring_max; i++)
	{
		if (!_ring_manager.ring_list[i]._inuse) continue; // skip any inactive rings
		ring_draw(&_ring_manager.ring_list[i]);
	}
}

void ring_system_close()
{
	if (_ring_manager.ring_list)
	{
		free(_ring_manager.ring_list);
	}
	memset(&_ring_manager, 0, sizeof(RingManager));
}

Entity* ring_new(float x, float y)
{
	//GFC_Color ringColor = gfc_color8();

	Entity* ring = NULL;

	for (int i = 0; i < _ring_manager.ring_max; i++)
	{
		if (!_ring_manager.ring_list[i]._inuse)
		{
			ring = &_ring_manager.ring_list[i];
			break;
		}
	}
	if (!ring)
	{
		slog("no free ring slots available");
		return NULL;
	}

	memset(ring, 0, sizeof(Entity));
	ring->_inuse = 1;
	ring->scale = gfc_vector2d(1, 1);

	ring->sprite = gf2d_sprite_load_all(
		"images/ring.png",
		64,
		64,
		1,
		0
	);

	ring->type = ENTITY_TYPE_RING;
	ring->frame = 0; /*<the current frame of animation for the sprite>*/
	ring->position = gfc_vector2d(x, y);
	ring->velocity = gfc_vector2d(0, 0);

	ring->update = ring_update;
	ring->free = ring_free;
	ring->gravityOn = false;

	return ring;
}

int ring_collect(Entity* ring, Entity* player)
{
	GFC_Shape ring_circle;
	GFC_Shape player_rect;

	if (!ring || !player)
	{
		slog("no ring or player");
		return 0;
	}

	if (!ring->_inuse) return 0;

	float ring_radius = (ring->sprite->frame_w / 2.0f) * 0.5f;

	ring_circle = gfc_shape_circle(ring->position.x, ring->position.y, ring_radius);
	player_rect = gfc_shape_rect(player->position.x, player->position.y, player->sprite->frame_w, player->sprite->frame_h);

	if (gfc_shape_overlap(ring_circle, player_rect))
	{
		slog("player overlapping with ring");
		ring->_inuse = 0;
		if (_ring_manager.ring_sound >= 0)
			audio_play_sound(_ring_manager.ring_sound);
		else
			slog("ring_sound not loaded, skipping audio");
		entity_destroy(ring);
		return 1;
	}
	return 0;

}

void ring_update(Entity* ring)
{
	if (!ring) return;

	//ring->frame += 0;
	//if (ring->frame >= 8) ring->frame = 1;
	//ring->velocity.x += 0.1;

	gfc_vector2d_add(ring->position, ring->position, ring->velocity);
}

void ring_destroy(Entity* ring)
{
	if (!ring) return;
	gf2d_sprite_delete(ring->sprite);
}

void ring_free(Entity* ring)
{
	if (!ring) return;
}

void ring_system_clear()
{
	int i;
	for (i = 0; i < _ring_manager.ring_max; i++)
	{
		if (!_ring_manager.ring_list[i]._inuse) continue;
		gf2d_sprite_free(_ring_manager.ring_list[i].sprite);
		_ring_manager.ring_list[i]._inuse = 0;
	}
}

Uint32 ring_system_get_max()
{
	return _ring_manager.ring_max;
}

Entity* ring_system_get(int index)
{
	if (index < 0 || index >= _ring_manager.ring_max) return NULL;
	return &_ring_manager.ring_list[index];
}
