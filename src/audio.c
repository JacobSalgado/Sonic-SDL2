#include "SDL_mixer.h"

#include "simple_logger.h"
#include "gfc_list.h"

#include "audio.h"

/*typedef enum
{

};*/

typedef struct
{
	GFC_List* backgroundMusicList;
	GFC_List* soundEffectList;
}AudioManager;

static AudioManager _audio_manager = { 0 };

int volume = 0;

void audio_init()
{
	_audio_manager.backgroundMusicList = gfc_list_new();
	slog("backgroundMusicList pointer after init: %p", _audio_manager.backgroundMusicList);
	_audio_manager.soundEffectList = gfc_list_new();

	Mix_Init(MIX_INIT_MP3);

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		slog("SDL_Mixer couldn't initialize. Error: %s\n", Mix_GetError());
		return;
	}
	set_volume(10);
}

int audio_load_music(const char* filename)
{
	Mix_Music* music = NULL;
	music = Mix_LoadMUS(filename);
	if (music == NULL)
	{
		slog("failed to load music. SDL_Mixer error: %s\n", Mix_GetError());
		return -1;
	}
	slog("music loaded successfully: %s", filename);
	//music.pusb
	//_audio_manager.backgroundMusicList = gfc_list_append(_audio_manager.backgroundMusicList, music);
	gfc_list_append(_audio_manager.backgroundMusicList, music);
	slog("list size after append: %d", _audio_manager.backgroundMusicList->size);
	slog("backgroundMusicList pointer at load time: %p", _audio_manager.backgroundMusicList);
	//return _audio_manager.backgroundMusicList->size - 1;
	return gfc_list_get_count(_audio_manager.backgroundMusicList) - 1;
}

int audio_load_sound(const char* filename)
{
	Mix_Chunk* sound = NULL;
	sound = Mix_LoadWAV(filename);
	if (sound == NULL)
	{
		slog("failed to load sound. SDL_Mixer error: %s\n", Mix_GetError());
		return -1;
	}
	slog("sound loaded successfully: %s", filename);
	//music.pusb
	gfc_list_append(_audio_manager.soundEffectList, sound);
	//return _audio_manager.soundEffectList->size - 1;
	return gfc_list_get_count(_audio_manager.soundEffectList) - 1;
}

int audio_play_music(int music)
{
	slog("audio_play_music called with index: %d", music);
	slog("Mix_PlayingMusic: %d", Mix_PlayingMusic());

	Mix_Music* track = gfc_list_get_nth(_audio_manager.backgroundMusicList, music);
	slog("track pointer: %p", track);

	if (Mix_PlayingMusic() == 0)
	{
		Mix_VolumeMusic(volume);
		slog("volume set to: %d", volume);
		int result = Mix_PlayMusic(track, -1);
		slog("Mix_PlayMusic result: %d (0=success, -1=error)", result);
		if (result < 0)
			slog("Mix_PlayMusic error: %s", Mix_GetError());
		//Mix_PlayMusic(gfc_list_get_nth(_audio_manager.backgroundMusicList, music), -1);
	}
	else
	{
		slog("music already playing, skipping");
	}
	return 0;
}

int audio_play_sound(int sound)
{
	Mix_Chunk* chunk = gfc_list_get_nth(_audio_manager.soundEffectList, sound);
	if (chunk == NULL)
	{
		slog("audio_play_sound: chunk at index %d is NULL", sound);
		return -1;
	}
	Mix_VolumeChunk(chunk, volume);
	int result = Mix_PlayChannel(-1, chunk, 0);
	if (result < 0)
		slog("Mix_PlayChannel error: %s", Mix_GetError());
	return 0;
}

void set_volume(int v)
{
	volume = (MIX_MAX_VOLUME * v) / 100;
}

void audio_play()
{
	if (Mix_PausedMusic() == 1)
	{
		Mix_ResumeMusic();
	}
	else
	{
		Mix_PauseMusic();
	}
}

void audio_close()
{
	for (int i = 0; i < gfc_list_get_count(_audio_manager.backgroundMusicList); i++)
	{
		Mix_FreeMusic(gfc_list_get_nth(_audio_manager.backgroundMusicList, i));
		//gfc_list_set_nth(_audio_manager.backgroundMusicList, i, NULL);
	}
	gfc_list_delete(_audio_manager.backgroundMusicList);
	for (int i = 0; i < gfc_list_get_count(_audio_manager.soundEffectList); i++)
	{
		Mix_FreeChunk(gfc_list_get_nth(_audio_manager.soundEffectList, i));
		//gfc_list_set_nth(_audio_manager.soundEffectList, i, NULL);
	}
	gfc_list_delete(_audio_manager.soundEffectList);
	Mix_Quit();
}

void audio_cleanup()
{
}

void audio_stop_music()
{
	Mix_HaltMusic();
}
