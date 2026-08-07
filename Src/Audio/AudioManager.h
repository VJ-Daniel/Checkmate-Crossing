#pragma once
#include <string>
#include <SDL_mixer.h>

class AudioManager {
public:
    static bool Initialize();
    static void Shutdown();
    static void PlaySound(const std::string& filePath);
    static void PlayMusic(const std::string& filePath);
    static void StopMusic();
    static void PlayFootstep(const std::string& filePath);
private:
    static Mix_Chunk* soundChunk;
    static Mix_Music* music;
    static int footstepChannel;
};