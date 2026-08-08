#include "AudioManager.h"
#include <iostream>

std::unordered_map<std::string, Mix_Chunk*> AudioManager::soundCache;
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

	// All channels are done mixing once the audio device is closed below,
	// so it's safe to free every cached chunk here in one pass.
	for (auto& entry : soundCache) {
		if (entry.second)
			Mix_FreeChunk(entry.second);
	}
	soundCache.clear();

	Mix_CloseAudio();
}

Mix_Chunk* AudioManager::GetOrLoadChunk(const std::string& filePath) {
	auto it = soundCache.find(filePath);
	if (it != soundCache.end())
		return it->second;

	Mix_Chunk* chunk = Mix_LoadWAV(filePath.c_str());
	if (!chunk) {
		std::cerr << "Failed to load sound: " << filePath << std::endl;
		return nullptr;
	}

	soundCache[filePath] = chunk;
	return chunk;
}

void AudioManager::PlaySound(const std::string& filePath) {
	Mix_Chunk* chunk = GetOrLoadChunk(filePath);
	if (!chunk)
		return;

	Mix_PlayChannel(-1, chunk, 0);
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

	Mix_Chunk* chunk = GetOrLoadChunk(filePath);
	if (!chunk)
		return;

	// Play it and store the channel number
	footstepChannel = Mix_PlayChannel(-1, chunk, 0);
}
