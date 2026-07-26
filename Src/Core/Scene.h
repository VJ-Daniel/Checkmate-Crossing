#pragma once

#include <memory>
#include <vector>

#include "GameObject.h"

/// Stores, initializes and updates the ordered collection of game objects.
class Scene
{
public:

    Scene();

    ~Scene() = default;

    /// Appends an object; insertion order also defines rendering order.
    void Add(const std::shared_ptr<GameObject>& object);

    void Remove(const std::shared_ptr<GameObject>& object);

    void Clear();

    void Initialize();

    void Update(float deltaTime);

    const std::vector<std::shared_ptr<GameObject>>& GetObjects() const;

private:

    std::vector<std::shared_ptr<GameObject>> objects;
};
