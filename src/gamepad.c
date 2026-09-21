#include "simple_logger.h"

#include "gamepad.h"

int xDirection = 0;
int yDirection = 0;

void gamepad_init(SDL_GameController** controller)
{
	SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

	if (SDL_NumJoysticks() == 0)
	{
		slog("no controllers found");
		return;
	}
	*controller = SDL_GameControllerOpen(0);

	if (!*controller)
	{
		slog("no controller found");
		return;
	}

	slog("%d controller[s] connected", SDL_NumJoysticks());

	if (SDL_GameControllerGetType(*controller) == SDL_CONTROLLER_TYPE_PS4)
		slog("PS4 controller connected");
}

void gamepad_close(SDL_GameController** controller)
{
	if (*controller)
	{
		SDL_GameControllerClose(*controller);
		*controller = NULL;
	}
}

void gamepad_cleanup(void)
{
}

void gamepad_update(SDL_Event* event)
{
	if (event->type == SDL_CONTROLLERAXISMOTION)
	{
		// x-axis
		if (event->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
		{
			if (event->caxis.value > DEAD_ZONE)
				xDirection = 1;
			else if (event->caxis.value < -DEAD_ZONE)
				xDirection = -1;
			else
				xDirection = 0;
		}
		// y-axis
		if (event->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
		{
			if (event->caxis.value > DEAD_ZONE)
				yDirection = 1;
			else if (event->caxis.value < -DEAD_ZONE)
				yDirection = -1;
			else
				yDirection = 0;
		}
	}
}

int get_gamepad_x_direction(void)
{
	return xDirection;
}

int get_gamepad_y_direction(void)
{
	return yDirection;
}
