/*
    ============================================================
    Checkmate Crossing - Mesh Builder

    Bakes a pile of axis-aligned boxes into a single indexed mesh, so a
    model built from dozens of voxels still costs one draw call.

    This file also owns the canonical unit-cube geometry, which
    Mesh::CreateCube reuses.
    ============================================================
*/

#include "MeshBuilder.h"

namespace
{
    /// One corner of the unit cube, in the order the faces are wound.
    struct CubeCorner
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };

    /// Unit cube spanning -0.5 .. +0.5 on every axis.
    ///
    /// Each face carries its own four corners so it can keep its own normal.
    /// Sharing corners between faces would average the normals and lose the
    /// crisp per-face shading the whole look depends on.
    const CubeCorner UnitCube[24] =
    {
        // Front (+Z)
        { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
        { {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },

        // Back (-Z)
        { {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
        { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },

        // Left (-X)
        { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },

        // Right (+X)
        { {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },

        // Top (+Y)
        { { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },

        // Bottom (-Y)
        { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },
        { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
        { { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } }
    };
}

MeshBuilder::MeshBuilder()
    : currentColor(1.0f)
{
}

void MeshBuilder::SetColor(const glm::vec3& color)
{
    currentColor = color;
}

const glm::vec3& MeshBuilder::GetColor() const
{
    return currentColor;
}

void MeshBuilder::AddBox(
    const glm::vec3& center,
    const glm::vec3& size)
{
    const unsigned int firstVertex =
        static_cast<unsigned int>(vertices.size());

    for (const CubeCorner& corner : UnitCube)
    {
        MeshVertex vertex;

        vertex.position = center + corner.position * size;

        // The boxes are only ever scaled on their axes, never rotated, so
        // the cube's own face normals stay correct as they are.
        vertex.normal = corner.normal;
        vertex.texCoord = corner.texCoord;
        vertex.color = currentColor;

        vertices.push_back(vertex);
    }

    for (unsigned int face = 0; face < 6; ++face)
    {
        const unsigned int base = firstVertex + face * 4;

        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);

        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }
}

void MeshBuilder::AddSlab(
    float width,
    float depth,
    float bottomY,
    float topY)
{
    AddSlabAt(0.0f, 0.0f, width, depth, bottomY, topY);
}

void MeshBuilder::AddSlabAt(
    float x,
    float z,
    float width,
    float depth,
    float bottomY,
    float topY)
{
    const float height = topY - bottomY;

    if (height <= 0.0f || width <= 0.0f || depth <= 0.0f)
        return;

    AddBox(
        glm::vec3(x, bottomY + height * 0.5f, z),
        glm::vec3(width, height, depth));
}

void MeshBuilder::AddFrustum(
    float bottomWidth,
    float topWidth,
    float bottomY,
    float topY)
{
    AddFrustumAt(0.0f, 0.0f, bottomWidth, topWidth, bottomY, topY);
}

void MeshBuilder::AddFrustumAt(
    float offsetX,
    float offsetZ,
    float bottomWidth,
    float topWidth,
    float bottomY,
    float topY)
{
    const float height = topY - bottomY;

    if (height <= 0.0f || bottomWidth <= 0.0f || topWidth <= 0.0f)
        return;

    const float b = bottomWidth * 0.5f;
    const float t = topWidth * 0.5f;

    // How far each side leans in per unit of height. The sloped faces lie
    // on the plane  side = b + slope * (y - bottomY), so the outward normal
    // is (axis, -slope, 0) once normalised.
    const float slope = (t - b) / height;

    const unsigned int firstVertex =
        static_cast<unsigned int>(vertices.size());

    const glm::vec3 faceNormals[6] =
    {
        glm::normalize(glm::vec3(0.0f, -slope, 1.0f)),   // front  (+Z)
        glm::normalize(glm::vec3(0.0f, -slope, -1.0f)),  // back   (-Z)
        glm::normalize(glm::vec3(1.0f, -slope, 0.0f)),   // right  (+X)
        glm::normalize(glm::vec3(-1.0f, -slope, 0.0f)),  // left   (-X)
        glm::vec3(0.0f, 1.0f, 0.0f),                     // top
        glm::vec3(0.0f, -1.0f, 0.0f)                     // bottom
    };

    // Corners of each face, wound counter-clockwise seen from outside, in
    // the same order as the unit cube above.
    const glm::vec3 faceCorners[6][4] =
    {
        {   // Front (+Z)
            { -b, bottomY,  b }, {  b, bottomY,  b },
            {  t, topY,     t }, { -t, topY,     t }
        },
        {   // Back (-Z)
            {  b, bottomY, -b }, { -b, bottomY, -b },
            { -t, topY,    -t }, {  t, topY,    -t }
        },
        {   // Right (+X)
            {  b, bottomY,  b }, {  b, bottomY, -b },
            {  t, topY,    -t }, {  t, topY,     t }
        },
        {   // Left (-X)
            { -b, bottomY, -b }, { -b, bottomY,  b },
            { -t, topY,     t }, { -t, topY,    -t }
        },
        {   // Top
            { -t, topY,  t }, {  t, topY,  t },
            {  t, topY, -t }, { -t, topY, -t }
        },
        {   // Bottom
            { -b, bottomY, -b }, {  b, bottomY, -b },
            {  b, bottomY,  b }, { -b, bottomY,  b }
        }
    };

    const glm::vec2 cornerUVs[4] =
    {
        { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };

    for (int face = 0; face < 6; ++face)
    {
        for (int corner = 0; corner < 4; ++corner)
        {
            MeshVertex vertex;

            vertex.position = faceCorners[face][corner] +
                glm::vec3(offsetX, 0.0f, offsetZ);

            vertex.normal = faceNormals[face];
            vertex.texCoord = cornerUVs[corner];
            vertex.color = currentColor;

            vertices.push_back(vertex);
        }

        const unsigned int base =
            firstVertex + static_cast<unsigned int>(face) * 4;

        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);

        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }
}

std::shared_ptr<Mesh> MeshBuilder::Build() const
{
    auto mesh = std::make_shared<Mesh>();

    mesh->Create(vertices, indices);

    return mesh;
}

void MeshBuilder::Clear()
{
    vertices.clear();
    indices.clear();
}

bool MeshBuilder::IsEmpty() const
{
    return vertices.empty();
}

std::size_t MeshBuilder::GetBoxCount() const
{
    return vertices.size() / 24;
}
