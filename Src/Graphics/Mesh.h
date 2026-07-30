#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>
#include <glew.h>

/// One vertex of a 3D mesh: where it is, which way its surface faces, where
/// it samples a texture and what colour it is tinted.
///
/// Textures are not used yet, but the layout is already in place so texture
/// mapping can be added without rebuilding data.
///
/// The colour lets a single mesh hold more than one material - a tree trunk
/// and its leaves, or a wooden shaft and an iron tip - while still costing
/// one draw call. It multiplies with the object's own colour, so meshes that
/// leave it white behave exactly as they did before it existed.
struct MeshVertex
{
    glm::vec3 position;

    glm::vec3 normal;

    glm::vec2 texCoord;

    glm::vec3 color = glm::vec3(1.0f);
};

/// Owns one VAO / VBO / EBO triple describing indexed 3D geometry.
///
/// The VBO holds the vertex data, the EBO holds the triangle indices so
/// vertices can be reused, and the VAO remembers how the attributes are
/// laid out. Meshes are shared through std::shared_ptr, so the whole
/// battlefield can be drawn from a single ground-plane buffer.
class Mesh
{
public:

    Mesh();

    ~Mesh();

    // GPU handles must not be copied, or they would be deleted twice.
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    /// Uploads vertex and index data and configures the vertex attributes.
    void Create(
        const std::vector<MeshVertex>& vertices,
        const std::vector<unsigned int>& indices);

    /// Submits the whole mesh as one indexed draw call.
    void Draw() const;

    void Destroy();

    bool IsValid() const;

    GLsizei GetIndexCount() const;

    //---------------------------------------------------------
    // Primitive factories
    //---------------------------------------------------------

    /// Unit cube centred on the origin (-0.5 .. +0.5 on every axis).
    /// Used for the pawn placeholder, and later for obstacles and pieces.
    static std::shared_ptr<Mesh> CreateCube();

    /// Flat 1 x 1 quad lying in the XZ plane, centred on the origin and
    /// facing up. Scaled by the transform to become a lane or the ground.
    static std::shared_ptr<Mesh> CreateGroundPlane();

private:

    GLuint vao;

    GLuint vbo;

    GLuint ebo;

    GLsizei indexCount;
};
