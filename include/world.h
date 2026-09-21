#ifndef __WORLD_H__
#define __WORLD_H__

#include "gfc_shape.h"
#include "gf2d_sprite.h"
#include "simple_json.h"
//#include "entity.h"

struct Entity_S;
typedef struct Entity_S Entity;

extern void spawn_fly_enemy(float x, float y);
extern void spawn_turret_enemy(float x,float y ,Entity* player);

typedef enum {
	TERRAIN_SOLID = 0,
	TERRAIN_ONEWAY = 1,
	TERRAIN_MOVING = 2
} TerrainType;

typedef struct 
{
	GFC_Rect bounds;
	TerrainType type;

	/* Moving Platform data */
	GFC_Vector2D moveDistance;
	GFC_Vector2D origin;
	GFC_Vector2D currentVelocity;
	float moveSpeed;
	int moveDirection;
	Uint32 spriteFrame;
} Platform;

typedef struct
{
	GFC_Vector2D p1;
	GFC_Vector2D p2;
	float thickness;
	GFC_Vector2D normal;
	float length;
	Uint32 spriteFrame;
}Slope;

typedef struct World_S
{
	Sprite* background;		/**<background image for the world>*/
	Sprite* tileLayer;		/**<prerendered sprite layer>*/
	Sprite* tileSet;		/**<sprite containing the tiles for the world>*/
	Uint8* tileMap;			/**<the tiles that make up the world>*/
	Uint32 tileHeight;		/**<how many tiles tall the map is>*/
	Uint32 tileWidth;		/**<how many tiles wide the map is>*/

	Platform* platforms;
	Uint32 platformCount;
	Slope* slopes;
	Uint32 slopeCount;

	SJson* entitiesJson;
	SJson* _json;

	float player_start_x;
	float player_start_y;

	char music_path[256];
}
World;

void world_tile_layer_build(World* world);

/**level
* @brief load a world from a config file 
* @param filename the name of the world file to load
* @return NULL on error, or a usable world otherwise 
*/
World* world_load(const char* filename);

/**
* @brief set the camera bounds to the world size
*/
void world_setup_camera(World* world);

/**
* @brief create a brand new world
* @param width the width of the tiles 
* @param height the height of the tiles
* @return NULL for errors, or an empty world
*/
World* new_world(Uint32 width, Uint32 height);

/**
* @brief free a previously assigned world
* @param world the world to free
*/
void free_world(World* world);

/**
* @brief draw the world
* @return world the world to draw
*/
void draw_world(World* world);

/**
* @brief updates the position of any moving platforms in the scene
* @param world the world to update
*/
void world_update_moving_platforms(World* world);

void world_spawn_entities(World* world, Entity* player);

#endif
