#pragma once

#include <string>

/// Static Windows WAV playback service for music and sound effects.
class AudioPlayer
{
public:
    /// Plays a looping movement sound on the effect channel.
    static bool PlayLoop(const std::wstring& filename);
    /// Plays a randomized impact, or the dedicated fatal impact.
    static bool PlayPunch(bool fatal);
    /// Opens and starts the background music channel.
    static bool PlayMusicLoop();
    /// Restarts background music when the WAV reaches its end.
    static void UpdateMusic();
    static bool PlayGameLose();
    static void Stop();
    static void StopMusic();
    /// Stops and closes both effect and music channels.
    static void StopAll();

private:
    static bool PlayOnce(const std::wstring& filename);

    static std::wstring currentSound;
    static bool musicOpen;
};
