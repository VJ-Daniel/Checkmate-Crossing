#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <glew.h>
#include <glm.hpp>

#include "Camera3D.h"
#include "Shader.h"
#include "Sprite.h"

/// Draws sprites through one shared quad and one shared shader.
///
/// Every sprite in the game reuses the same VAO, VBO, EBO and shader program:
/// only the uniforms change between draws. Adding a sprite never means
/// building geometry or compiling a shader.
///
/// ---------------------------------------------------------------------
/// OpenGL state
/// ---------------------------------------------------------------------
/// Sprites need different depth settings from the opaque 3D meshes, so the
/// renderer brackets its work with Begin and End. Begin records the state it
/// is about to change and End puts it back, which is why the mesh renderer -
/// that relies on the state main.cpp sets once at start-up - keeps working
/// either side of a sprite pass.
///
/// Inside a pass:
///
///   - Blending stays on, with the same src-alpha / one-minus-src-alpha
///     function main.cpp already sets. Transparent PNGs just work.
///   - Depth TESTING stays on for world sprites, so a billboard behind a
///     tree is correctly hidden by it.
///   - Depth WRITING is off. Transparent surfaces must not occlude each
///     other, or two overlapping fireballs would clip into a hard edge
///     instead of blending.
///   - Because they do not write depth, world sprites have to be drawn after
///     all opaque geometry, and back to front among themselves. DrawAll sorts
///     them by camera distance to get that right.
///   - Screen sprites additionally turn depth testing off, so UI always wins.
///   - Face culling is never enabled anywhere in this project, so a quad is
///     visible from both sides and needs no special handling.
///
/// ---------------------------------------------------------------------
/// Textures
/// ---------------------------------------------------------------------
/// The renderer never owns a texture. A Sprite names one, and the renderer
/// looks it up in ResourceManager at draw time. A name that is missing is
/// skipped, and reported once rather than every frame.
class SpriteRenderer
{
public:

    explicit SpriteRenderer(std::shared_ptr<Camera3D> camera);

    ~SpriteRenderer();

    // Owns GL objects, so copying it would delete them twice.
    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    /// Builds the shared quad and loads the sprite shader. Returns false if
    /// the shader fails to compile, which is a hard error worth aborting on.
    bool Initialize();

    /// Releases the quad and the shader. Must run while a GL context is
    /// current.
    void Shutdown();

    bool IsReady() const;

    /// Window size in pixels, for the screen-space projection. Call this from
    /// the framebuffer resize path so UI keeps its scale.
    void SetScreenSize(float width, float height);

    //---------------------------------------------------------
    // Drawing
    //---------------------------------------------------------

    /// Configures sprite GL state. Every Draw must sit between Begin and End.
    void Begin();

    /// Draws one sprite. Invisible ones, fully transparent ones and ones
    /// whose texture is not loaded are skipped.
    void Draw(const Sprite& sprite);

    void End();

    /// Draws a whole list with the correct ordering, bracketing it in Begin
    /// and End. Returns immediately when the list is empty, so an unused
    /// sprite pass costs nothing.
    ///
    /// World sprites are sorted by layer, then far to near, so blending
    /// stacks correctly. Screen sprites are sorted by layer and drawn last,
    /// on top of everything.
    void DrawAll(const std::vector<Sprite>& sprites);

private:

    void BuildQuad();

    /// Model matrix for a sprite, per render mode.
    glm::mat4 BuildScreenMatrix(const Sprite& sprite) const;
    glm::mat4 BuildBillboardMatrix(const Sprite& sprite) const;
    glm::mat4 BuildGroundDecalMatrix(const Sprite& sprite) const;

    /// Region with the sprite's flips applied, ready for the shader.
    static void ResolveUV(
        const Sprite& sprite,
        glm::vec2& uvMin,
        glm::vec2& uvMax);

    std::shared_ptr<Camera3D> camera;

    std::shared_ptr<Shader> shader;

    GLuint vao;

    GLuint vbo;

    GLuint ebo;

    float screenWidth;

    float screenHeight;

    /// True between Begin and End.
    bool inPass;

    // State captured by Begin so End can restore it.
    GLboolean savedDepthTest;
    GLboolean savedDepthMask;
    GLboolean savedBlend;

    /// Depth testing is toggled per sprite mode inside a pass; this tracks
    /// what it currently is so it is only changed when it has to be.
    bool depthTestEnabled;

    /// Texture names already reported as missing, so the log is not spammed
    /// once per frame.
    std::unordered_set<std::string> reportedMissing;
};
