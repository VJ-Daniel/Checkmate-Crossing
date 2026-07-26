/*
    ============================================================
    Gangster Survival - Scene
    Author: Leonardo Moura | Date: 2026

    Owns the ordered object collection and dispatches initialization
    and frame updates to every active entity.
    ============================================================
*/

#include "Scene.h"

#include <algorithm>

Scene::Scene()
{
}

void Scene::Add(const std::shared_ptr<GameObject>& object)
{
    if (object == nullptr)
        return;

    objects.push_back(object);
}

void Scene::Remove(const std::shared_ptr<GameObject>& object)
{
    objects.erase(
        std::remove(
            objects.begin(),
            objects.end(),
            object),
        objects.end());
}

void Scene::Clear()
{
    objects.clear();
}

void Scene::Initialize()
{
    for (auto& object : objects)
    {
        if (object)
        {
            object->Initialize();
        }
    }
}

void Scene::Update(float deltaTime)
{
    for (auto& object : objects)
    {
        if (object && object->IsActive())
        {
            object->Update(deltaTime);
        }
    }
}

const std::vector<std::shared_ptr<GameObject>>& Scene::GetObjects() const
{
    return objects;
}
