#pragma once

#include <memory>
#include <vector>

#include "GameObject.h"

class SpriteRenderer;

/// Presentation layer for health and Game Over information.
class GameUI
{
public:
    /// Loads UI textures and positions elements in screen coordinates.
    bool Initialize(float screenWidth, float screenHeight);
    /// Advances visual UI transitions such as Game Over fade-in.
    void Update(float deltaTime);
    /// Draws all visible UI elements above the world.
    void Render(SpriteRenderer& renderer) const;

    /// Shows one heart for every remaining player health point.
    void SetHealth(int health);
    void ShowGameOver();
    /// Restores five hearts and hides the Game Over overlay.
    void Reset();

private:
    std::vector<std::shared_ptr<GameObject>> healthHearts;
    std::shared_ptr<GameObject> gameOverImage;
    float gameOverFadeTimer = 0.0f;
    bool gameOverVisible = false;
};
