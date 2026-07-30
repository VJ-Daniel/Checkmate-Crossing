/*
    ============================================================
    Checkmate Crossing - Mesh

    Wraps the VAO / VBO / EBO triple that stores indexed 3D geometry
    on the GPU, and builds the placeholder primitives the prototype
    needs: a cube for characters and obstacles, and a flat quad for
    the ground lanes.
    ============================================================
*/

#include "Mesh.h"

#include "MeshBuilder.h"

namespace
{
    /// Builds the two triangles of one quad face from four ordered corners.
    void AddQuadIndices(
        std::vector<unsigned int>& indices,
        unsigned int firstVertex)
    {
        indices.push_back(firstVertex + 0);
        indices.push_back(firstVertex + 1);
        indices.push_back(firstVertex + 2);

        indices.push_back(firstVertex + 2);
        indices.push_back(firstVertex + 3);
        indices.push_back(firstVertex + 0);
    }
}

Mesh::Mesh()
    : vao(0),
    vbo(0),
    ebo(0),
    indexCount(0)
{
}

Mesh::~Mesh()
{
    Destroy();
}

void Mesh::Create(
    const std::vector<MeshVertex>& vertices,
    const std::vector<unsigned int>& indices)
{
    Destroy();

    if (vertices.empty() || indices.empty())
        return;

    indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(MeshVertex)),
        vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(),
        GL_STATIC_DRAW);

    // Location 0: position.
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(MeshVertex),
        (void*)offsetof(MeshVertex, position));

    // Location 1: normal, used by the directional light.
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(MeshVertex),
        (void*)offsetof(MeshVertex, normal));

    // Location 2: texture coordinate, reserved for future texture mapping.
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(MeshVertex),
        (void*)offsetof(MeshVertex, texCoord));

    // Location 3: per-vertex tint, so one mesh can carry several materials.
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(MeshVertex),
        (void*)offsetof(MeshVertex, color));

    glBindVertexArray(0);
}

void Mesh::Draw() const
{
    if (vao == 0 || indexCount == 0)
        return;

    glBindVertexArray(vao);

    glDrawElements(
        GL_TRIANGLES,
        indexCount,
        GL_UNSIGNED_INT,
        nullptr);

    glBindVertexArray(0);
}

void Mesh::Destroy()
{
    if (ebo != 0)
    {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }

    if (vbo != 0)
    {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }

    if (vao != 0)
    {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }

    indexCount = 0;
}

bool Mesh::IsValid() const
{
    return vao != 0 && indexCount > 0;
}

GLsizei Mesh::GetIndexCount() const
{
    return indexCount;
}

std::shared_ptr<Mesh> Mesh::CreateCube()
{
    // The cube's corner data lives in MeshBuilder, which is also what the
    // voxel models are assembled from, so there is only one description of
    // a cube in the project.
    MeshBuilder builder;

    builder.AddBox(
        glm::vec3(0.0f),
        glm::vec3(1.0f));

    return builder.Build();
}

std::shared_ptr<Mesh> Mesh::CreateGroundPlane()
{
    const float h = 0.5f;

    // Wound counter-clockwise when seen from above, so the lit side is up.
    const std::vector<MeshVertex> vertices =
    {
        { { -h, 0.0f,  h }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
        { {  h, 0.0f,  h }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
        { {  h, 0.0f, -h }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
        { { -h, 0.0f, -h }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } }
    };

    std::vector<unsigned int> indices;
    AddQuadIndices(indices, 0);

    auto mesh = std::make_shared<Mesh>();
    mesh->Create(vertices, indices);

    return mesh;
}
