/*
    ============================================================
    Gangster Survival - Audio Module

    Author: Leonardo Moura
    Date: 2026

    Description:
    Provides independent playback for looping background music,
    movement sounds, randomized impacts and Game Over audio.
    ============================================================
*/

#include "AudioPlayer.h"

#include <Windows.h>
#include <mmsystem.h>

#include <array>
#include <chrono>
#include <random>

std::wstring AudioPlayer::currentSound;
bool AudioPlayer::musicOpen = false;

bool AudioPlayer::PlayLoop(const std::wstring& filename)
{
    if (currentSound == filename)
        return true;

    Stop();

    const BOOL played = PlaySoundW(
        filename.c_str(), nullptr,
        SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT);

    if (played)
        currentSound = filename;

    return played == TRUE;
}

bool AudioPlayer::PlayOnce(const std::wstring& filename)
{
    Stop();

    const BOOL played = PlaySoundW(
        filename.c_str(), nullptr,
        SND_FILENAME | SND_ASYNC | SND_NODEFAULT);

    if (played)
        currentSound = filename;

    return played == TRUE;
}

bool AudioPlayer::PlayPunch(bool fatal)
{
    constexpr const wchar_t* fatalPunch =
        L"Src/Resources/Sounds/Punch 03.wav";

    if (fatal)
        return PlayOnce(fatalPunch);

    static constexpr std::array<const wchar_t*, 4> regularPunches =
    {
        L"Src/Resources/Sounds/Punch 01.wav",
        L"Src/Resources/Sounds/Punch 02.wav",
        L"Src/Resources/Sounds/Punch 04.wav",
        L"Src/Resources/Sounds/Punch 05.wav"
    };

    static std::mt19937 randomEngine(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    static std::uniform_int_distribution<std::size_t> distribution(
        0, regularPunches.size() - 1);

    return PlayOnce(regularPunches[distribution(randomEngine)]);
}

bool AudioPlayer::PlayMusicLoop()
{
    StopMusic();

    wchar_t absolutePath[MAX_PATH] = {};
    if (GetFullPathNameW(
        L"Src/Resources/Sounds/SoftGangsta.wav",
        MAX_PATH, absolutePath, nullptr) == 0)
    {
        return false;
    }

    const std::wstring openCommand =
        L"open \"" + std::wstring(absolutePath) +
        L"\" type waveaudio alias GameMusic";

    const MCIERROR openResult = mciSendStringW(
        openCommand.c_str(), nullptr, 0, nullptr);

    if (openResult != 0)
        return false;

    musicOpen = true;
    return mciSendStringW(
        L"play GameMusic from 0", nullptr, 0, nullptr) == 0;
}

void AudioPlayer::UpdateMusic()
{
    if (!musicOpen)
        return;

    wchar_t mode[32] = {};
    if (mciSendStringW(
        L"status GameMusic mode", mode, 32, nullptr) == 0 &&
        std::wstring(mode) == L"stopped")
    {
        mciSendStringW(L"seek GameMusic to start", nullptr, 0, nullptr);
        mciSendStringW(L"play GameMusic", nullptr, 0, nullptr);
    }
}

bool AudioPlayer::PlayGameLose()
{
    return PlayOnce(L"Src/Resources/Sounds/gamelose.wav");
}

void AudioPlayer::Stop()
{
    if (currentSound.empty())
        return;

    PlaySoundW(nullptr, nullptr, 0);
    currentSound.clear();
}

void AudioPlayer::StopMusic()
{
    mciSendStringW(L"stop GameMusic", nullptr, 0, nullptr);
    mciSendStringW(L"close GameMusic", nullptr, 0, nullptr);
    musicOpen = false;
}

void AudioPlayer::StopAll()
{
    Stop();
    StopMusic();
}
