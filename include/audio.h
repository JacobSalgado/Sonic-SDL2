#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "gfc_audio.h"

/*typedef enum
{
	
}AudioType;*/

extern int volume;

/**
* @brief initializes audio system
*/
void audio_init();

void audio_close();

int audio_load_music(const char* filename);

int audio_play_music(int music);

int audio_load_sound(const char* filename);

int audio_play_sound(int sound);

void set_volume(int volume);

void audio_play();

void audio_stop_music();

/**
* @brief cleans up any internal cache
*/
void audio_cleanup();

#endif
