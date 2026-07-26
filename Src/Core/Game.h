#pragma once

#include <memory>
#include <random>
#include <vector>

#include "Camera2D.h"
#include "Enemy.h"
#include "GameUI.h"
#include "Player.h"
#include "Scene.h"
#include "SpriteRenderer.h"

/// Owns the high-level lifetime and state transitions of one game session.
class Game
{
public:

    Game();

    ~Game();

    /// Creates rendering services, scene objects, UI and the first enemy.
    bool Initialize(
        float screenWidth,
        float screenHeight);

    /// Advances gameplay using frame-independent elapsed time.
    void Update(float deltaTime);

    /// Draws the world first and the UI overlay last.
    void Render();

    /// Releases OpenGL, scene, UI, resource and audio objects safely.
    void Shutdown();

    /// Rebuilds the dynamic match state after Game Over.
    void Restart();

    /// Reports whether the Game Over overlay is accepting restart input.
    bool IsGameOver() const;

    Camera2D& GetCamera();

    SpriteRenderer& GetRenderer();

    Scene& GetScene();

private:

    /// Spawns one enemy at a randomly selected horizontal edge.
    void SpawnEnemy();

    std::shared_ptr<Camera2D> camera;

    std::shared_ptr<SpriteRenderer> renderer;

    std::shared_ptr<Scene> scene;

    std::shared_ptr<GameObject> background;

    std::shared_ptr<Player> player;

    std::shared_ptr<GameUI> ui;

    std::vector<std::shared_ptr<Enemy>> enemies;

    std::mt19937 randomEngine;

    float enemySpawnTimer = 0.0f;

    bool gameOverActive = false;

    float deathDelayTimer = 0.0f;

    bool deathSequenceStarted = false;

    bool gameLosePlayed = false;
};
