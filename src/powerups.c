#include <stddef.h>
#include <stdlib.h>
#include "simple_logger.h"

#include "gfc_text.h"
#include "powerups.h"

static SJson* _powerupsJson = NULL;
static SJson* _powerupDefs = NULL;

void powerups_close();

void powerups_initialize(const char* filename)
{
	if (!filename)
	{
		slog("no filename provided for powerup initialization");
		return;
	}
	_powerupsJson = sj_load(filename);
	if (!_powerupsJson)
	{
		slog("failed to load json for powerup definition");
		return;
	}
	_powerupDefs = sj_object_get_value(_powerupsJson, "powerups");
	if (!_powerupDefs)
	{
		slog("powerup definition file %s does not contain powerups list", filename);
		sj_free(_powerupsJson);
		_powerupsJson = NULL;
		return;
	}

	atexit(powerups_close);
}

void powerups_close()
{
	if (_powerupsJson)
	{
		sj_free(_powerupsJson);
	}
	_powerupsJson = NULL;

}

SJson* powerups_get_def_by_name(const char* name)
{
	int i, c;
	SJson* powerup;
	const char* powerupName = NULL;

	if (!name) return NULL;

	if (!_powerupDefs)
	{
		slog("no powerup definitions loaded");
	}
	c = sj_array_get_count(_powerupDefs);
	for (i = 0; i < c; i++)
	{
		powerup = sj_array_get_nth(_powerupDefs, i);
		if (!powerup) continue;
		powerupName = sj_object_get_value_as_string(powerup, "name");
		if (!powerupName) continue;
		if (gfc_strlcmp(name, powerupName) == 0)
		{
			// found the powerup
			return powerup;
		}
	}
	slog("no powerup found by name %s", name);


	return NULL;
}
