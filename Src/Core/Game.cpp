/*
    ============================================================
    Gangster Survival - Game Orchestration Module

    Author: Leonardo Moura
    Date: 2026

    Description:
    Coordinates the scene, player, enemies, UI, audio, spawning,
    Game Over sequence, rendering order and match restart flow.
    ============================================================
*/

#include "Game.h"

#include <memory>
#include <algorithm>
#include <chrono>

#include "Shader.h"
#include "ResourceManager.h"
#include "AudioPlayer.h"

namespace
{
    constexpr float EnemySpawnInterval = 2.0f;
    constexpr std::size_t MaxEnemies = 5;
    constexpr float GameOverDelay = 1.5f;
}

Game::Game()
    : randomEngine(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count()))
{
    ResourceManager::Initialize();

}

Game::~Game()
{
    Shutdown();
}

bool Game::Initialize(
    float screenWidth,
    float screenHeight)
{
    camera = std::make_shared<Camera2D>(
        screenWidth,
        screenHeight);

    auto shader = std::make_shared<Shader>();

    if (!shader->Load(
        "Src/Shaders/vertex.glsl",
        "Src/Shaders/fragment.glsl"))
    {
        return false;
    }

    shader->Use();
    shader->SetInt("image", 0);

    renderer = std::make_shared<SpriteRenderer>(
        shader,
        camera);

    renderer->Initialize();

    scene = std::make_shared<Scene>();

    auto backgroundTexture = ResourceManager::LoadTexture(
        "ForestBackground",
        "Src/Resources/Background/2304x1296.png");

    if (!backgroundTexture)
        return false;

    auto backgroundSheet = ResourceManager::CreateSpriteSheet(
        "ForestBackgroundSheet", "ForestBackground", 1, 1);

    background = std::make_shared<GameObject>();
    background->GetSprite().SetSpriteSheet(backgroundSheet);
    background->GetTransform().SetPosition(
        -screenWidth * 0.5f,
        -screenHeight * 0.5f);
    background->GetTransform().SetScale(screenWidth, screenHeight);
    scene->Add(background);

    player = std::make_shared<Player>();
    scene->Add(player);

    scene->Initialize();

    ui = std::make_shared<GameUI>();
    if (!ui->Initialize(screenWidth, screenHeight))
        return false;

    SpawnEnemy();
    AudioPlayer::PlayMusicLoop();

    return true;
}

void Game::Update(float deltaTime)
{
    if (scene)
    {
        AudioPlayer::UpdateMusic();

        if (gameOverActive)
        {
            ui->Update(deltaTime);
            return;
        }

        scene->Update(deltaTime);

        if (player->IsDead())
        {
            ui->SetHealth(player->GetHealth());

            if (!deathSequenceStarted)
            {
                deathSequenceStarted = true;
                AudioPlayer::StopMusic();
            }

            if (player->GetAnimator().IsFinished())
                deathDelayTimer += deltaTime;

            if (deathDelayTimer >= GameOverDelay)
            {
                gameOverActive = true;
                gameLosePlayed = true;
                AudioPlayer::PlayGameLose();
                ui->ShowGameOver();
            }

            return;
        }

        for (auto it = enemies.begin(); it != enemies.end();)
        {
            if ((*it)->ShouldDisappear())
            {
                scene->Remove(*it);
                it = enemies.erase(it);
            }
            else
            {
                ++it;
            }
        }

        enemySpawnTimer += deltaTime;

        if (enemies.size() < MaxEnemies &&
            enemySpawnTimer >= EnemySpawnInterval)
            SpawnEnemy();

        ui->SetHealth(player->GetHealth());

    }
}

bool Game::IsGameOver() const
{
    return gameOverActive;
}

void Game::Restart()
{
    if (!scene || !gameOverActive)
        return;

    for (const auto& enemy : enemies)
        scene->Remove(enemy);
    enemies.clear();

    scene->Remove(player);
    player.reset();

    player = std::make_shared<Player>();
    player->Initialize();
    scene->Add(player);

    enemySpawnTimer = 0.0f;
    gameOverActive = false;
    deathDelayTimer = 0.0f;
    deathSequenceStarted = false;
    gameLosePlayed = false;
    ui->Reset();
    SpawnEnemy();
    AudioPlayer::PlayMusicLoop();
}

void Game::SpawnEnemy()
{
    if (!scene || !player || enemies.size() >= MaxEnemies)
        return;

    auto enemy = std::make_shared<Enemy>(player);
    enemy->Initialize();

    std::uniform_int_distribution<int> sideDistribution(0, 1);
    enemy->Spawn(sideDistribution(randomEngine) == 0);

    scene->Add(enemy);
    enemies.push_back(enemy);
    enemySpawnTimer = 0.0f;
}

void Game::Render()
{
    if (!scene || !renderer)
        return;

    for (const auto& object : scene->GetObjects())
    {
        if (!object)
            continue;

        renderer->Draw(
            object->GetTransform(),
            object->GetSprite());
    }

    // UI is rendered last so it always remains above the game world.
    if (ui)
        ui->Render(*renderer);
}

void Game::Shutdown()
{
    AudioPlayer::StopAll();

    if (renderer)
    {
        renderer->Destroy();
        renderer.reset();
    }

    player.reset();

    enemies.clear();

    ui.reset();

    background.reset();

    scene.reset();

    camera.reset();

    ResourceManager::Shutdown();
}

Camera2D& Game::GetCamera()
{
    return *camera;
}

SpriteRenderer& Game::GetRenderer()
{
    return *renderer;
}

Scene& Game::GetScene()
{
    return *scene;
}
