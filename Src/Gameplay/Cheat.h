#pragma once

#include <array>

class Pawn;
class PieceMeshLibrary;

/// Developer-only switches. Turn this off before shipping a build.
///
/// A compile-time constant rather than a runtime toggle on purpose: with it
/// false the cheat polling is dead code the optimiser removes outright, so a
/// shipped build cannot have cheat keys reachable by accident.
namespace CheatConfig
{
    inline constexpr bool EnableCheatKeys = true;
}

/// Number-key shortcuts for becoming each playable character, so a tester
/// can reach any of them without playing the level up to its collectibles.
///
/// This is input plumbing and nothing else. It detects a press and asks the
/// pawn to change character; every part of what that means - the model, the
/// animation body plan, the footprint, the ability - belongs to Pawn and the
/// existing ability system, and none of it is duplicated here.
///
/// Bound to 1-6, which normal gameplay does not use. Movement stays on
/// WASD/arrows, Space still jumps and E still interacts.
class CheatSystem
{
public:

    /// Polls the cheat keys once per frame. Does nothing at all when
    /// CheatConfig::EnableCheatKeys is false.
    void Update(Pawn& pawn, PieceMeshLibrary& meshLibrary);

private:

    /// One entry per binding, so a held key switches once rather than every
    /// frame. The project's Input only reports whether a key is currently
    /// down, so the edge has to be found here - the same way Pawn already
    /// tracks Space and E.
    static constexpr int BindingCount = 6;

    std::array<bool, BindingCount> keyWasDown{};
};
