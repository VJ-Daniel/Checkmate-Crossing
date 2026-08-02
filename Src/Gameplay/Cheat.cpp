/*
    ============================================================
    Checkmate Crossing - Developer Cheats

    Number keys that turn the player into any playable character, for
    testing them without collecting the allies first.

    Strictly input plumbing: read a key edge, ask the pawn to become
    something else, say so on the console. The model swap, the animation
    body plan, the footprint and the ability all belong to Pawn and to
    the existing ability system, and none of them are reimplemented here.
    ============================================================
*/

#include "Cheat.h"

#include <iostream>
#include <iterator>

#include "Input.h"
#include "Pawn.h"
#include "PieceMeshFactory.h"

namespace
{
    /// One cheat key and the character it selects.
    struct CheatBinding
    {
        Key key;
        PieceType character;
        const char* name;
    };

    /// The six playable characters.
    ///
    /// Knight is deliberately absent and has no key: it is the riderless
    /// horse, a collectible ally rather than a body the player wears. The
    /// mounted pawn on 6 is the playable horse character.
    constexpr CheatBinding Bindings[] =
    {
        { Key::Num1, PieceType::Pawn,        "Pawn" },
        { Key::Num2, PieceType::Bishop,      "Bishop" },
        { Key::Num3, PieceType::Rook,        "Rook" },
        { Key::Num4, PieceType::Queen,       "Queen" },
        { Key::Num5, PieceType::King,        "King" },
        { Key::Num6, PieceType::MountedPawn, "Pawn riding the Horse" }
    };

    static_assert(
        static_cast<int>(std::size(Bindings)) == 6,
        "Cheat bindings and CheatSystem::BindingCount must agree.");
}

void CheatSystem::Update(Pawn& pawn, PieceMeshLibrary& meshLibrary)
{
    if constexpr (!CheatConfig::EnableCheatKeys)
        return;

    for (int index = 0; index < BindingCount; ++index)
    {
        const CheatBinding& binding = Bindings[index];

        const bool isDown = Input::IsKeyPressed(binding.key);

        // Edge-triggered: only the frame the key goes down counts, so
        // holding it does not rebuild the character sixty times a second.
        const bool justPressed = isDown && !keyWasDown[index];

        keyWasDown[index] = isDown;

        if (!justPressed)
            continue;

        // Everything about becoming this character - model, animation,
        // footprint, clearing the old ability and banking the new one -
        // happens inside Pawn. Position, velocity, spawn point, checkpoint
        // and camera are all untouched by it.
        pawn.SetCharacter(binding.character, meshLibrary);

        // Flushed rather than buffered: a debug line that only shows up once
        // the process exits cleanly is no use while testing.
        std::cout << "[Cheat] Switched to " << binding.name << std::endl;

        PieceType ability = PieceType::Bishop;

        if (Pawn::GetCharacterAbility(binding.character, ability))
        {
            std::cout
                << "[Cheat]   ability ready: "
                << GetPieceTypeName(ability)
                << " (press E)" << std::endl;
        }
        else if (binding.character == PieceType::King)
        {
            // Reported rather than invented: no King ability exists in the
            // project or in the GDD, so the switch works and the ability
            // slot stays empty until someone designs one.
            std::cout
                << "[Cheat]   no ability: the King has none implemented"
                << std::endl;
        }
    }
}
