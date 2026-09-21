#include "SDL_events.h"
#include "player.h"


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C"
{
#include "simple_logger.h"
#include "gfc_vector.h"
#include "camera.h"
#include "gamepad.h"
#include "collision.h"
#include "audio.h"
#include "ring.h"
}
#endif

static const AnimationDef anim_defs[ANIM_COUNT] =
{	// filename					w,	h,	fpr,s, end
	{"images/sonic_genesis.png", 97, 81, 9, 0, 3}, // idle
	{"images/sonic_genesis.png", 97, 81, 9, 4, 9}, // run
	{"images/sonic_genesis.png", 97, 81, 9, 28, 31}, // Jump
};

Player* Player::_instance = nullptr;


Player* Player::create_instance(int x, int y/*, const AnimationDef& def*/)
{
	if (!_instance)
	{
		_instance = new Player(x, y/*, def*/);
	}
	return _instance;
}

void Player::destroy_instance()
{
	if (_instance)
	{
		delete _instance;
		_instance = nullptr;
	}
}

Player::Player(int x, int y/*, const AnimationDef& def*/)
{
	entity = entity_new();
	entity_set_player(entity);

	entity->type = ENTITY_TYPE_PLAYER;

	entity->position.x = x;
	entity->position.y = y;

	/* TEMPORARY */
	entity->width = 97;
	entity->height = 81;
	entity->velocity = gfc_vector2d(2, 2);

	entity->scale = gfc_vector2d(1, 1);

	for (int i = 0; i < ANIM_COUNT; i++)
	{
		const AnimationDef& def = anim_defs[i];
		//def = anim_defs[i];
		sprites[i] = gf2d_sprite_load_all(
			def.filename,
			def.frame_w,
			def.frame_h,
			def.frames_per_row,
			0
		);
	}
	current_anim = ANIM_Idle;
	previous_anim = ANIM_Idle;
	entity->sprite = sprites[ANIM_Idle];
	entity->frame = 0;

	/* TEMPORARY */
	/*entity->sprite = gf2d_sprite_load_all(
		"images/sonic_idle.png",
		94,
		118,
		1,
		0
	);
	entity->frame = 0;*/
	entity->data = this; // important to set so it doesn't crash

	entity->think = [](Entity* e)
		{
			Player* self = (Player*)e->data;
			self->think();
		};

	entity->update = [](Entity* e)
		{
			Player* self = (Player*)e->data;
			self->update();
		};

	entity->gravityOn = true;

	static const AnimationDef anim_defs[ANIM_COUNT] =
	{
		{
			// "images/sonic.png"
		}
	};

	jump_sound = audio_load_sound("audio/jump.wav");
}

Player::~Player()
{
	entity_destroy(entity);
}

void Player::think()
{
	//GFC_Vector2D dir = { 0 };

	float jump_force = -10.0f;
	float move_speed = 3.0f;
	float friction = 0.75f;

	if (entity->boost_timer > 0)
	{
		entity->boost_timer -= 0.016f;
		entity->velocity.x = entity->boost_velocity;
		return;
	}

	Uint8 was_moving_last = moving;

	// horizontal movement
	if (input.left)
	{
		entity->scale.x = -fabs(entity->scale.x);
		entity->velocity.x = -move_speed;
		moving = true;
	}
	else if (input.right)
	{
		entity->scale.x = fabs(entity->scale.x);
		entity->velocity.x = move_speed;
		moving = true;
	}
	else
	{
		//entity->sprite->frame_w = 94;
		entity->velocity.x *= friction; // to stop the player
		moving = false;
	}

	// jumping
	if (input.jump && entity->onGround)
	{
		//entity->sprite->frame_w = 94;
		jumping = true;
		audio_play_sound(jump_sound);
		entity->frame = 9; // p
		entity->velocity.y = jump_force;
		entity->onGround = 0;
		input.jump = false;
	}
}

void Player::set_animation(PlayerAnimation anim)
{
	if (anim == current_anim) return;

	previous_anim = current_anim;
	current_anim = anim;
	entity->sprite = sprites[anim];
	entity->frame = anim_defs[anim].start_frame;
}

void Player::update()
{	
	if (!entity->onGround && jumping)
		set_animation(ANIM_Jump);
	else if (moving)
		set_animation(ANIM_Run);
	else
		set_animation(ANIM_Idle);

	entity->frame += 0.1; // how fast the frame plays

	// only within animation range
	const AnimationDef& def = anim_defs[current_anim];
	if (entity->frame >= def.end_frame)
		entity->frame = def.start_frame;
	
	if (entity->onGround)
	{
		jumping = false;
	}
	
	//if (entity->frame >= 26) entity->frame = 0;
	camera_center_on(entity->position);

	//gfc_vector2d_add(entity->position, entity->position, entity->velocity);
}

void Player::handle_input(SDL_Event* event)
{
	int movementVelocity = 5;
	int slowDown = 4;

	gamepad_update(event);

	int gamepadX = get_gamepad_x_direction();
	int gamepadY = get_gamepad_y_direction();

	// analog stick movement - drift occurs
	/*if (gamepadX > 0)
	{
		input.right = true;
		input.left = false;
	}
	else if (gamepadX < 0)
	{
		input.left = true;
		input.right = false;
	}
	else
	{
		input.right = false;
		input.left = false;
	}*/

	if (event->type == SDL_KEYDOWN && event->key.repeat == 0)
	{
		// SWITCH VELOCITY
		switch (event->key.keysym.sym)
		{
		case SDLK_UP: //input.up = true; break; // positive y is downwards
		//case SDLK_DOWN: input.down = true; break;
		case SDLK_SPACE: input.jump = true; break;
		case SDLK_LEFT: input.left = true; break;
		case SDLK_RIGHT: input.right = true; break;
		//case SDLK_r: ring_new(entity); break;
		}
	}
	// slow player down
	else if (event->type == SDL_KEYUP && event->key.repeat == 0)
	{
		switch (event->key.keysym.sym)
		{
		case SDLK_UP:  // positive y is downwards
		case SDLK_SPACE: input.jump = false; break;
		case SDLK_LEFT: input.left = false; break;
		case SDLK_RIGHT: input.right = false; break;
		}
	}
	else if (event->type == SDL_CONTROLLERBUTTONDOWN)
	{
		switch (event->cbutton.button)
		{
		case SDL_CONTROLLER_BUTTON_A: input.jump = true; break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT: input.left = true; break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: input.right = true; break;
		}
	}
	else if (event->type == SDL_CONTROLLERBUTTONUP)
	{
		switch (event->cbutton.button)
		{
		case SDL_CONTROLLER_BUTTON_A: input.jump = false; break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT: input.left = false; break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: input.right = false; break;
		}
	}
}

int player_is_jumping(Entity* player)
{
	if (!player || !player->data) return 0;
	Player* self = (Player*)player->data;
	if (self->jumping)
		return 1;
	else
		return 0;
}
