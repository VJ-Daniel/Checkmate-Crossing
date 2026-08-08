#pragma once
#include <string>
#include <unordered_map>
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
    // Loads filePath once and caches it; every later call for the same path
    // reuses the cached chunk instead of re-reading the file from disk each
    // time (previously every footstep re-loaded walk.wav from disk, up to
    // 4 times a second while moving). Multiple channels can safely play the
    // same cached Mix_Chunk at once, which also avoids freeing audio data
    // that might still be playing on another channel.
    static Mix_Chunk* GetOrLoadChunk(const std::string& filePath);

    static std::unordered_map<std::string, Mix_Chunk*> soundCache;
    static Mix_Music* music;
    static int footstepChannel;
};