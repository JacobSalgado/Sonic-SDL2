#include "simple_logger.h"
#include "simple_json.h"

#include "gf2d_graphics.h"

#include "camera.h"
#include "world.h"
#include "entity.h"

#include "ring.h"
#include "spring.h"
//#include "turret_enemy.h"
//#include "fly_enemy.h"
#include "goalpost.h"
#include "speedpad.h"
#include "enemy_spawn.h"

extern void spawn_fly_enemy(float x, float y);
extern void spawn_turret_enemy(float x,float y ,Entity* player);

void world_tile_layer_build(World* world)
{
	int i, j;

	GFC_Vector2D position;
	Uint32 frame;

	Uint32 index;

	if (!world) return;

	if (!world->tileSet) return;

	if (world->tileLayer)
	{
		gf2d_sprite_free(world->tileLayer);
	}
	world->tileLayer = gf2d_sprite_new();

	world->tileLayer->surface = gf2d_graphics_create_surface(
		world->tileWidth * world->tileSet->frame_w, 
		world->tileHeight * world->tileSet -> frame_h);

	world->tileLayer->frame_w = world->tileWidth * world->tileSet->frame_w;
	world->tileLayer->frame_h = world->tileHeight * world->tileSet->frame_h;

	if (!world->tileLayer->surface)
	{
		slog("failed to create tilelayer surface");
		return;
	}

	for (j = 0; j < world->tileHeight; j++)
	{
		for (i = 0; i < world->tileWidth; i++)
		{
			index = i + (j * world->tileWidth);
			if (world->tileMap[index] == 0) continue;

			position.x = i * world->tileSet->frame_w;
			position.y = j * world->tileSet->frame_h;

			frame = world->tileMap[index] - 1;

			gf2d_sprite_draw_to_surface(
				world->tileSet,
				position,
				NULL,
				NULL,
				frame,
				world->tileLayer->surface);
		}
	}

	for (i = 0; i < (int)world->platformCount; i++)
	{
		if (world->platforms[i].type == TERRAIN_MOVING) continue;

		int x, y;
		GFC_Rect r = world->platforms[i].bounds;
		int tilesWide = (int)(r.w / world->tileSet->frame_w);
		int tilesTall = (int)(r.h / world->tileSet->frame_h);

		for (y = 0; y < tilesTall; y++)
		{
			for (x = 0; x < tilesWide; x++)
			{
				position.x = r.x + (x * world->tileSet->frame_w);
				position.y = r.y + (y * world->tileSet->frame_h);

				gf2d_sprite_draw_to_surface(
					world->tileSet,
					position,
					NULL,
					NULL,
					world->platforms[i].spriteFrame,
					world->tileLayer->surface
				);
			}
		}
	}

	for (i = 0; i < (int)world->slopeCount; i++)
	{
		float x = world->slopes[i].p2.x - world->slopes[i].p1.x;
		float y = world->slopes[i].p2.y - world->slopes[i].p1.y;
		float length = world->slopes[i].length;
		int steps = (int)(length / world->tileSet->frame_w) + 1;
		int s;

		for (s = 0; s <= steps; s++)
		{
			float t = (steps > 0) ? (float)s / (float)steps : 0.0f;
			position.x = world->slopes[i].p1.x + t * x;
			position.y = world->slopes[i].p1.y + t * y;

			gf2d_sprite_draw_to_surface(
				world->tileSet,
				position,
				NULL,
				NULL,
				world->slopes[i].spriteFrame,
				world->tileLayer->surface);
		}
	}

	world->tileLayer->texture = SDL_CreateTextureFromSurface(gf2d_graphics_get_renderer(), world->tileLayer->surface);
	if (!world->tileLayer->texture)
	{
		slog("failed to convert world tile layer to texture");
		return;
	}
}

World* world_load(const char* filename)
{
	World* world = NULL;
	SJson* json = NULL;
	SJson* wjson = NULL;
	SJson* vertical, * horizontal;
	SJson* item;

	int w = 0, h = 0;
	int i, j;
	int tile;

	const char* tileSet;
	const char* background;
	int frame_w, frame_h;
	int frames_per_line;

	if (!filename)
	{
		slog("no filename provided for world_load");
		return NULL;
	}

	json = sj_load(filename);
	if (!json)
	{
		slog("failed to load file %s", filename);
		return NULL;
	}
	wjson = sj_object_get_value(json, "world");
	if (!wjson)
	{
		slog("%s missing 'world' object", filename);
		sj_free(json);
		return NULL;
	}
	vertical = sj_object_get_value(wjson, "tileMap");
	if (!vertical)
	{
		slog("%s missing 'tileMap'", filename);
		sj_free(json);
		return NULL;
	}
	h = sj_array_get_count(vertical);
	horizontal = sj_array_get_nth(vertical, 0);
	w = sj_array_get_count(horizontal);
	world = new_world(w, h);
	if (!world)
	{
		slog("failed to create space for a new world for file %s",filename);
		sj_free(json);
		return NULL;
	}
	for (j = 0; j < h; j++)
	{
		horizontal = sj_array_get_nth(vertical, j);
		if (!horizontal) continue;
		for (i = 0; i < w; i++)
		{
			item = sj_array_get_nth(horizontal, i);
			if (!item) continue;
			tile = 0;
			sj_get_integer_value(item, &tile); 
			world->tileMap[i+(j*w)] = tile;
		}
	}

	const char* music = sj_object_get_value_as_string(wjson, "music");
	if (music) snprintf(world->music_path, 256, "%s", music);

	SJson* terrainsArray = sj_object_get_value(wjson, "terrains");
	if (terrainsArray)
	{
		world->platformCount = sj_array_get_count(terrainsArray);
		world->platforms = gfc_allocate_array(sizeof(Platform), world->platformCount);

		for (int i = 0; i < (int)world->platformCount; i++)
		{
			SJson* j = sj_array_get_nth(terrainsArray, i);
			int frame = 0;
			int x = 0, y = 0, w = 0, h = 0;
			sj_object_get_value_as_int(j, "x", &x);
			sj_object_get_value_as_int(j, "y", &y);
			sj_object_get_value_as_int(j, "w", &w);
			sj_object_get_value_as_int(j, "h", &h);
			sj_object_get_value_as_int(j, "frame", &frame);
			world->platforms[i].bounds = gfc_rect(x, y, w, h);
			world->platforms[i].spriteFrame = (Uint32)frame;

			const char* type = sj_object_get_value_as_string(j, "type");
			if (type)
			{
				if (strcmp(type, "oneway") == 0)
					world->platforms[i].type = TERRAIN_ONEWAY;
				else if (strcmp(type, "moving") == 0)
				{
					world->platforms[i].type = TERRAIN_MOVING;

					//int x = 0, y = 0;
					float dx = 0, dy = 0, speed = 2.0f;
					//sj_object_get_value_as_float(j, "x", &x);
					//sj_object_get_value_as_float(j, "y", &y);
					sj_object_get_value_as_float(j, "moveDX", &dx);
					sj_object_get_value_as_float(j, "moveDY", &dy);
					sj_object_get_value_as_float(j, "moveSpeed", &speed);
					world->platforms[i].moveDistance = gfc_vector2d(dx, dy);
					world->platforms[i].moveSpeed = speed;
					world->platforms[i].moveDirection = 1;
					world->platforms[i].origin = gfc_vector2d(x, y);
				}
				else
					world->platforms[i].type = TERRAIN_SOLID;
			}
		}
	}

	SJson* slopesArray = sj_object_get_value(wjson, "slopes");
	if (slopesArray)
	{
		world->slopeCount = sj_array_get_count(slopesArray);
		world->slopes = gfc_allocate_array(sizeof(Slope), world->slopeCount);

		for (int i = 0; i < (int)world->slopeCount; i++)
		{
			SJson* j = sj_array_get_nth(slopesArray, i);
			int frame = 0;
			float x = 0, y = 0, x2 = 0, y2 = 0, thickness = 16;

			sj_object_get_value_as_float(j, "x1", &x);
			sj_object_get_value_as_float(j, "y1", &y);
			sj_object_get_value_as_float(j, "x2", &x2);
			sj_object_get_value_as_float(j, "y2", &y2);
			sj_object_get_value_as_int(j, "frame", &frame);
			sj_object_get_value_as_float(j, "thickness", &thickness);

			world->slopes[i].p1 = gfc_vector2d(x, y);
			world->slopes[i].p2 = gfc_vector2d(x2, y2);
			world->slopes[i].thickness = thickness;

			float dx = x2 - x, dy = y2 - y;
			world->slopes[i].length = sqrt(dx * dx + dy * dy);

			world->slopes[i].normal = gfc_vector2d(dy, -dx);
			gfc_vector2d_normalize(&world->slopes[i].normal);
		}
	}

	SJson* entitiesArray = sj_object_get_value(wjson, "entities");
	if (entitiesArray)
	{
		world->entitiesJson = entitiesArray;
	}

	background = sj_object_get_value_as_string(wjson, "background");
	world->background = gf2d_sprite_load_image(background);

	tileSet = sj_object_get_value_as_string(wjson, "tileSet");
	sj_object_get_value_as_float(wjson, "player_start_x", &world->player_start_x);
	sj_object_get_value_as_float(wjson, "player_start_y", &world->player_start_y);
	sj_object_get_value_as_int(wjson, "frame_w", &frame_w);
	sj_object_get_value_as_int(wjson, "frame_h", &frame_h);
	sj_object_get_value_as_int(wjson, "frames_per_line", &frames_per_line);
	world->tileSet = gf2d_sprite_load_all(
		tileSet,
		frame_w,
		frame_h,
		frames_per_line,
		1);
	world_tile_layer_build(world);

	//sj_free(json);
	world->_json = json;
	return world;
}

World* new_world(Uint32 width, Uint32 height)
{
	World* world;

	if ((!width) || (!height))
	{
		slog("cannot create world with 0 height and width");
		return NULL;
	}

	world = gfc_allocate_array(sizeof(World), 1);
	if (!world)
	{
		slog("failed to allocate a new world");
		return NULL;
	}

	world->tileMap = gfc_allocate_array(sizeof(Uint8), height * width);
	world->tileHeight = height;
	world->tileWidth = width;
	return world;
}

void free_world(World* world)
{
	if (!world) return;

	// free the pointers
	gf2d_sprite_free(world->background);
	gf2d_sprite_free(world->tileSet);
	gf2d_sprite_free(world->tileLayer);
	free(world->tileMap);
	free(world->platforms);
	free(world->slopes);

	sj_free(world->_json);
	free(world);
}

void draw_world(World* world)
{
	GFC_Vector2D position;
	GFC_Vector2D offset;

	if (!world) return;

	offset = camera_get_offset();

	gf2d_sprite_draw_image(world->background, gfc_vector2d(0, 0));
	gf2d_sprite_draw_image(world->tileLayer, offset);

	// moving platforms
	for (int i = 0; i < (int)world->platformCount; i++)
	{
		Platform* p = &world->platforms[i];
		if (p->type != TERRAIN_MOVING) continue;

		GFC_Vector2D pos;
		pos.x = p->bounds.x + offset.x;
		pos.y = p->bounds.y + offset.y;

		gf2d_sprite_draw(
			world->tileSet,
			pos,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			p->spriteFrame
		);
	}
}

void world_setup_camera(World* world)
{
	if (!world) return;
	if ((!world->tileLayer) || (!world->tileLayer->surface))
	{
		slog("no tile layer set for world");
		return;
	}

	camera_set_bounds(gfc_rect(0,0, world->tileLayer->surface->w, world->tileLayer->surface->h));
	camera_enable_binding(1);
	camera_apply_bounds();
}

void world_update_moving_platforms(World* world)
{
	if (!world->platforms) return;

	for (int i = 0; i < (int)world->platformCount; i++)
	{
		Platform* p = &world->platforms[i];
		if (p->type != TERRAIN_MOVING) continue;

		float length = sqrtf(p->moveDistance.x * p->moveDistance.x + p->moveDistance.y * p->moveDistance.y);
		if (length == 0) continue;

		float dirX = (p->moveDistance.x / length) * p->moveSpeed * p->moveDirection;
		float dirY = (p->moveDistance.y / length) * p->moveSpeed * p->moveDirection;

		p->bounds.x += dirX;
		p->bounds.y += dirY;
		p->currentVelocity = gfc_vector2d(dirX, dirY);

		float travelX = p->bounds.x - p->origin.x;
		float travelY = p->bounds.y - p->origin.y;
		float traveled = sqrtf(travelX * travelX + travelY * travelY);

		// reverse movement of the platform
		if (traveled >= length)
		{
			if (p->moveDirection == 1)
			{
				p->bounds.x = p->origin.x + p->moveDistance.x;
				p->bounds.y = p->origin.y + p->moveDistance.y;
			}
			else
			{
				p->bounds.x = p->origin.x;
				p->bounds.y = p->origin.y;
			}
			p->moveDirection *= -1;
			/* not working
			if (p->moveDirection == 1)
			{
				p->bounds.x = p->origin.x;
				p->bounds.y = p->origin.y;
			}
			else
			{
				p->bounds.x = p->origin.x + p->moveDistance.x;
				p->bounds.y = p->origin.y + p->moveDistance.y;
			}*/
		}
	}
}

void world_spawn_entities(World* world, Entity* player)
{
	SJson* j;
	SJson* w;
	const char* type;
	float x, y;
	int count, i;

	if (!world || !world->entitiesJson) return;
	j = world->entitiesJson;
	count = sj_array_get_count(j);

	for (i = 0; i < count; i++)
	{
		w = sj_array_get_nth(j, i);
		if (!w) continue;

		x = 0; y = 0;
		sj_object_get_value_as_float(w, "x", &x);
		sj_object_get_value_as_float(w, "y", &y);
		type = sj_object_get_value_as_string(w, "type");
		if (!type) continue;

		if (strcmp(type, "ring") == 0)
			ring_new(x, y);
		else if (strcmp(type, "spring") == 0)
			spring_new(x, y);
		else if (strcmp(type, "speedpad") == 0)
			speedpad_new(x, y);
		else if (strcmp(type, "fly_enemy") == 0)
			spawn_fly_enemy(x, y);
		else if (strcmp(type, "turret") == 0)
			spawn_turret_enemy(x, y, player);
		else if (strcmp(type, "goal_post") == 0)
		{
			const char* next = sj_object_get_value_as_string(w, "next_level");
			if (next) goalpost_new(x, y, next);
		}
		else
			slog("unknown entity type attempting to be spawned");
	}
}
