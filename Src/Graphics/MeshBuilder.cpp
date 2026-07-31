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

#include <algorithm>
#include <cmath>

namespace
{
    /// Repeatable pseudo-random value in -1..1 for one grid corner.
    ///
    /// A hash rather than a random generator on purpose: the same corner has
    /// to produce the same number every time it is asked, because a mesh is
    /// built once, cached and shared. Anything stateful would give a
    /// different rock each run and, worse, different values for the same
    /// corner within one mesh.
    float HashNoise(unsigned int seed, int a, int b)
    {
        unsigned int hash = seed * 2654435761u;

        hash ^= static_cast<unsigned int>(a + 1) * 374761393u;
        hash ^= static_cast<unsigned int>(b + 1) * 668265263u;

        hash = (hash ^ (hash >> 13)) * 1274126177u;
        hash ^= (hash >> 16);

        return static_cast<float>(hash % 2001u) / 1000.0f - 1.0f;
    }

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

std::size_t MeshBuilder::GetTriangleCount() const
{
    return indices.size() / 3;
}

void MeshBuilder::AddFlatTriangle(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c)
{
    const glm::vec3 edge1 = b - a;
    const glm::vec3 edge2 = c - a;

    const glm::vec3 cross = glm::cross(edge1, edge2);

    // A face with no area has no direction to face. This happens naturally at
    // the poles of a sphere grid, where a quad collapses onto a point, so it
    // is skipped rather than treated as an error.
    //
    // The test is against the edge lengths rather than an absolute number,
    // because the cross product scales with the size of the triangle. A fixed
    // threshold either lets slivers through on large models or deletes real
    // faces on small ones - and a sliver is worse than a missing face, since
    // its normal comes out of floating-point noise and it gets lit at random.
    const float area = glm::length(cross);
    const float edgeScale = glm::length(edge1) * glm::length(edge2);

    if (edgeScale <= 0.0f || area <= edgeScale * 1e-6f)
        return;

    const glm::vec3 normal = cross / area;

    const unsigned int first = static_cast<unsigned int>(vertices.size());

    const glm::vec3 corners[3] = { a, b, c };
    const glm::vec2 uvs[3] =
    {
        { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }
    };

    for (int i = 0; i < 3; ++i)
    {
        MeshVertex vertex;

        vertex.position = corners[i];
        vertex.normal = normal;
        vertex.texCoord = uvs[i];
        vertex.color = currentColor;

        vertices.push_back(vertex);
    }

    indices.push_back(first + 0);
    indices.push_back(first + 1);
    indices.push_back(first + 2);
}

void MeshBuilder::AddFlatQuad(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec3& d)
{
    // Split into two triangles that each compute their own normal. On a
    // sphere the two halves are very slightly non-coplanar, and letting them
    // differ is what gives the surface its faceted break-up.
    AddFlatTriangle(a, b, c);
    AddFlatTriangle(a, c, d);
}

void MeshBuilder::AddFacetedSphere(
    const glm::vec3& center,
    float radius,
    int segments,
    int rings,
    float irregularity,
    unsigned int seed)
{
    segments = std::max(segments, 3);
    rings = std::max(rings, 2);

    if (radius <= 0.0f)
        return;

    // Corner positions are computed once into a grid and then referenced by
    // the faces. Recomputing them per face would let rounding - and, with
    // irregularity, different random values - pull neighbouring corners
    // apart and split the surface open along every edge.
    std::vector<glm::vec3> grid(
        static_cast<std::size_t>(rings + 1) *
        static_cast<std::size_t>(segments));

    const float pi = 3.14159265358979323846f;

    for (int ring = 0; ring <= rings; ++ring)
    {
        const float polar =
            pi * static_cast<float>(ring) / static_cast<float>(rings);

        // Snapped at the ends rather than trusting the trig. In single
        // precision sin(pi) comes out around -9e-8, not zero, which leaves
        // the pole smeared into a ring of points a hair apart instead of one
        // point - and every "degenerate" pole quad then becomes a real sliver
        // triangle with a normal made of rounding error.
        const bool atPole = (ring == 0 || ring == rings);

        const float sinPolar = atPole ? 0.0f : std::sin(polar);
        const float cosPolar = atPole
            ? ((ring == 0) ? 1.0f : -1.0f)
            : std::cos(polar);

        // Fades to nothing at the poles, so the highest and lowest points
        // stay exactly one radius out however lumpy the middle gets.
        const float lumpiness = irregularity * sinPolar;

        for (int segment = 0; segment < segments; ++segment)
        {
            const float azimuth =
                2.0f * pi *
                static_cast<float>(segment) / static_cast<float>(segments);

            const float displaced =
                radius * (1.0f + lumpiness * HashNoise(seed, ring, segment));

            grid[static_cast<std::size_t>(ring) * segments + segment] =
                center + glm::vec3(
                    displaced * sinPolar * std::cos(azimuth),
                    displaced * cosPolar,
                    displaced * sinPolar * std::sin(azimuth));
        }
    }

    for (int ring = 0; ring < rings; ++ring)
    {
        for (int segment = 0; segment < segments; ++segment)
        {
            const int next = (segment + 1) % segments;

            const glm::vec3& upperHere =
                grid[static_cast<std::size_t>(ring) * segments + segment];
            const glm::vec3& upperNext =
                grid[static_cast<std::size_t>(ring) * segments + next];
            const glm::vec3& lowerHere =
                grid[static_cast<std::size_t>(ring + 1) * segments + segment];
            const glm::vec3& lowerNext =
                grid[static_cast<std::size_t>(ring + 1) * segments + next];

            // Wound upper-here, upper-next, lower-next, lower-here, which
            // comes out counter-clockwise seen from outside the sphere.
            // The pole rows collapse to triangles, and AddFlatTriangle drops
            // the degenerate halves on its own.
            AddFlatQuad(upperHere, upperNext, lowerNext, lowerHere);
        }
    }
}

void MeshBuilder::AddFacetedCylinder(
    const glm::vec3& center,
    float radius,
    float length,
    int sides,
    CylinderAxis axis)
{
    sides = std::max(sides, 3);

    if (radius <= 0.0f || length <= 0.0f)
        return;

    const float pi = 3.14159265358979323846f;

    // A right-handed frame around the chosen axis: the ring is swept through
    // the two perpendicular vectors, and the ends sit along the axis itself.
    // Keeping the frame right-handed (across x up == along) is what makes one
    // winding order come out facing outwards for either axis.
    const glm::vec3 along = (axis == CylinderAxis::X)
        ? glm::vec3(1.0f, 0.0f, 0.0f)
        : glm::vec3(0.0f, 0.0f, 1.0f);

    const glm::vec3 across = (axis == CylinderAxis::X)
        ? glm::vec3(0.0f, 1.0f, 0.0f)
        : glm::vec3(1.0f, 0.0f, 0.0f);

    const glm::vec3 up = (axis == CylinderAxis::X)
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    const glm::vec3 halfSpan = along * (length * 0.5f);

    const glm::vec3 nearCentre = center - halfSpan;
    const glm::vec3 farCentre = center + halfSpan;

    std::vector<glm::vec3> ring(static_cast<std::size_t>(sides));

    for (int side = 0; side < sides; ++side)
    {
        const float angle =
            2.0f * pi *
            static_cast<float>(side) / static_cast<float>(sides);

        ring[static_cast<std::size_t>(side)] =
            across * (radius * std::cos(angle)) +
            up * (radius * std::sin(angle));
    }

    for (int side = 0; side < sides; ++side)
    {
        const int next = (side + 1) % sides;

        const glm::vec3& here = ring[static_cast<std::size_t>(side)];
        const glm::vec3& there = ring[static_cast<std::size_t>(next)];

        const glm::vec3 nearHere = nearCentre + here;
        const glm::vec3 nearThere = nearCentre + there;
        const glm::vec3 farThere = farCentre + there;
        const glm::vec3 farHere = farCentre + here;

        // Barrel facet, outward-facing.
        AddFlatQuad(nearHere, nearThere, farThere, farHere);

        // Caps as fans from the centre of each end. The two ends are wound
        // in opposite orders so both normals point away from the cylinder.
        AddFlatTriangle(farCentre, farHere, farThere);
        AddFlatTriangle(nearCentre, nearThere, nearHere);
    }
}
