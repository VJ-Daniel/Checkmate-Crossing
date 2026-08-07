#include "AudioManager.h"
#include <iostream>

Mix_Chunk* AudioManager::soundChunk = nullptr;
int AudioManager::footstepChannel = -1;
Mix_Music* AudioManager::music = nullptr;

bool AudioManager::Initialize() {
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
		std::cerr << "Audio Error: " << Mix_GetError() << std::endl;
		return false;
	}
	Mix_AllocateChannels(8);
	return true;
}

void AudioManager::Shutdown() {
	Mix_HaltMusic();
	Mix_FreeMusic(music);
	music = nullptr;
	Mix_FreeChunk(soundChunk);
	soundChunk = nullptr;
	Mix_CloseAudio();
}

void AudioManager::PlaySound(const std::string& filePath) {
	soundChunk = Mix_LoadWAV(filePath.c_str());
	if (!soundChunk) {
		std::cerr << "Failed to load sound: " << filePath << std::endl;
		return;

	}
	Mix_PlayChannel(-1, soundChunk, 0);
}

void AudioManager::PlayMusic(const std::string& filePath) {
	music = Mix_LoadMUS(filePath.c_str());
	if (!music)
	{
		std::cerr << "Failed to load music: " << filePath << std::endl;
		return;
	}
	Mix_PlayMusic(music, -1);
}

void AudioManager::StopMusic()
{
	Mix_HaltMusic();
}

void AudioManager::PlayFootstep(const std::string& filePath)
{
	// If the previous footstep is still playing, stop it immediately!
	if (footstepChannel != -1 && Mix_Playing(footstepChannel))
	{
		Mix_HaltChannel(footstepChannel);
	}

	soundChunk = Mix_LoadWAV(filePath.c_str());
	if (!soundChunk)
	{
		std::cerr << "Failed to load sound: " << filePath << std::endl;
		return;
	}

	// Play it and store the channel number
	footstepChannel = Mix_PlayChannel(-1, soundChunk, 0);
}