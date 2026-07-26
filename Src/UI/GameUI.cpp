/*
    ============================================================
    Gangster Survival - User Interface Module

    Author: Leonardo Moura
    Date: 2026

    Description:
    Owns and renders the health hearts and centered Game Over
    overlay, including its fade-in transition and reset state.
    ============================================================
*/

#include "GameUI.h"

#include <glm.hpp>

#include "ResourceManager.h"
#include "SpriteRenderer.h"

namespace
{
    constexpr float GameOverFadeDuration = 1.5f;
}

bool GameUI::Initialize(float screenWidth, float screenHeight)
{
    auto heartTexture = ResourceManager::LoadTexture(
        "UI_Heart", "Src/Resources/UI/hearts.png");
    auto gameOverTexture = ResourceManager::LoadTexture(
        "UI_GameOver", "Src/Resources/UI/gameover.png");

    if (!heartTexture || !gameOverTexture)
        return false;

    auto heartSheet = ResourceManager::CreateSpriteSheet(
        "UI_HeartSheet", "UI_Heart", 1, 1);
    auto gameOverSheet = ResourceManager::CreateSpriteSheet(
        "UI_GameOverSheet", "UI_GameOver", 1, 1);

    constexpr float heartSize = 32.0f;
    constexpr float spacing = 6.0f;
    constexpr float margin = 16.0f;

    for (int index = 0; index < 5; ++index)
    {
        auto heart = std::make_shared<GameObject>();
        heart->GetSprite().SetSpriteSheet(heartSheet);
        heart->GetTransform().SetScale(heartSize, heartSize);
        heart->GetTransform().SetPosition(
            -screenWidth * 0.5f + margin + index * (heartSize + spacing),
            screenHeight * 0.5f - margin - heartSize);
        healthHearts.push_back(heart);
    }

    constexpr float gameOverWidth = 620.0f;
    constexpr float gameOverHeight = 188.0f;

    gameOverImage = std::make_shared<GameObject>();
    gameOverImage->GetSprite().SetSpriteSheet(gameOverSheet);
    gameOverImage->GetTransform().SetScale(gameOverWidth, gameOverHeight);
    gameOverImage->GetTransform().SetPosition(
        -gameOverWidth * 0.5f, -gameOverHeight * 0.5f);

    Reset();
    return true;
}

void GameUI::Update(float deltaTime)
{
    if (!gameOverVisible || !gameOverImage)
        return;

    gameOverFadeTimer += deltaTime;
    const float alpha = glm::clamp(
        gameOverFadeTimer / GameOverFadeDuration, 0.0f, 1.0f);
    gameOverImage->GetSprite().SetTint(
        glm::vec4(1.0f, 1.0f, 1.0f, alpha));
}

void GameUI::Render(SpriteRenderer& renderer) const
{
    for (const auto& heart : healthHearts)
        renderer.Draw(heart->GetTransform(), heart->GetSprite());

    if (gameOverImage)
        renderer.Draw(
            gameOverImage->GetTransform(), gameOverImage->GetSprite());
}

void GameUI::SetHealth(int health)
{
    for (std::size_t index = 0; index < healthHearts.size(); ++index)
        healthHearts[index]->GetSprite().SetVisible(
            static_cast<int>(index) < health);
}

void GameUI::ShowGameOver()
{
    if (!gameOverImage)
        return;

    gameOverVisible = true;
    gameOverFadeTimer = 0.0f;
    gameOverImage->GetSprite().SetTint(
        glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
    gameOverImage->GetSprite().SetVisible(true);
}

void GameUI::Reset()
{
    gameOverVisible = false;
    gameOverFadeTimer = 0.0f;
    SetHealth(5);

    if (gameOverImage)
    {
        gameOverImage->GetSprite().SetTint(
            glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
        gameOverImage->GetSprite().SetVisible(false);
    }
}
