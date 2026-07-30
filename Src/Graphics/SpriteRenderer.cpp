/*
    ============================================================
    Checkmate Crossing - Sprite Renderer

    One quad, one shader, three render modes. Turns a Sprite into a
    model matrix and a handful of uniforms, and brackets the pass so it
    hands the OpenGL state back exactly as it found it.
    ============================================================
*/

#include "SpriteRenderer.h"

#include <algorithm>
#include <iostream>

#include <gtc/matrix_transform.hpp>

#include "ResourceManager.h"

namespace
{
    /// Fragments at or below this alpha are discarded instead of blended.
    constexpr float AlphaCutoff = 0.01f;

    /// Texture unit the sprite shader samples from.
    constexpr GLuint SpriteTextureUnit = 0;

    /// Sorting key for one sprite. Screen sprites always come after world
    /// sprites so UI ends up on top.
    bool IsScreenMode(SpriteRenderMode mode)
    {
        return mode == SpriteRenderMode::Screen;
    }
}

SpriteRenderer::SpriteRenderer(std::shared_ptr<Camera3D> camera)
    : camera(std::move(camera)),
    vao(0),
    vbo(0),
    ebo(0),
    screenWidth(1280.0f),
    screenHeight(720.0f),
    inPass(false),
    savedDepthTest(GL_TRUE),
    savedDepthMask(GL_TRUE),
    savedBlend(GL_TRUE),
    depthTestEnabled(true)
{
}

SpriteRenderer::~SpriteRenderer()
{
    Shutdown();
}

bool SpriteRenderer::Initialize()
{
    shader = std::make_shared<Shader>();

    if (!shader->Load(
        "Src/Shaders/sprite_vertex.glsl",
        "Src/Shaders/sprite_fragment.glsl"))
    {
        std::cerr << "SpriteRenderer: sprite shader failed to load.\n";

        shader.reset();

        return false;
    }

    shader->Use();
    shader->SetInt("spriteTexture", static_cast<int>(SpriteTextureUnit));
    shader->SetFloat("alphaCutoff", AlphaCutoff);

    BuildQuad();

    return vao != 0;
}

void SpriteRenderer::BuildQuad()
{
    // A unit quad spanning 0..1, with one attribute that serves as both the
    // local position and the interpolant used to walk across a UV region.
    // Two uses for one stream keeps the vertex format to a single vec2.
    //
    // The quad is not pre-centred. Each model matrix shifts it half a unit
    // after scaling, so a sprite is centred on its position and rotation
    // happens about its middle - which is what both a spinning billboard and
    // a turning ground decal want.
    const float corners[] =
    {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    const unsigned int indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void SpriteRenderer::Shutdown()
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

    // Shader owns its own GL program and deletes it in its destructor.
    shader.reset();

    reportedMissing.clear();
}

bool SpriteRenderer::IsReady() const
{
    return vao != 0 && shader != nullptr;
}

void SpriteRenderer::SetScreenSize(float width, float height)
{
    screenWidth = std::max(width, 1.0f);
    screenHeight = std::max(height, 1.0f);
}

void SpriteRenderer::Begin()
{
    if (!IsReady() || inPass)
        return;

    inPass = true;

    // Remember what the 3D pass left behind, so End can restore it exactly.
    glGetBooleanv(GL_DEPTH_TEST, &savedDepthTest);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &savedDepthMask);
    glGetBooleanv(GL_BLEND, &savedBlend);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Test against the world, but never write. Transparent surfaces that write
    // depth occlude each other and blending breaks down.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    depthTestEnabled = true;

    shader->Use();
    glBindVertexArray(vao);
}

void SpriteRenderer::End()
{
    if (!inPass)
        return;

    glBindVertexArray(0);

    glDepthMask(savedDepthMask);

    if (savedDepthTest == GL_TRUE)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (savedBlend == GL_TRUE)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    inPass = false;
}

void SpriteRenderer::ResolveUV(
    const Sprite& sprite,
    glm::vec2& uvMin,
    glm::vec2& uvMax)
{
    uvMin = sprite.region.min;
    uvMax = sprite.region.max;

    // Flipping is a swap of the region's bounds, so the shader stays free of
    // any flip logic and costs nothing for the common unflipped case.
    if (sprite.flipX)
        std::swap(uvMin.x, uvMax.x);

    if (sprite.flipY)
        std::swap(uvMin.y, uvMax.y);
}

glm::mat4 SpriteRenderer::BuildScreenMatrix(const Sprite& sprite) const
{
    // Pixels, origin at the window's top-left, centred on the position.
    glm::mat4 model(1.0f);

    model = glm::translate(
        model,
        glm::vec3(sprite.position.x, sprite.position.y, 0.0f));

    if (sprite.rotationDegrees != 0.0f)
    {
        model = glm::rotate(
            model,
            glm::radians(sprite.rotationDegrees),
            glm::vec3(0.0f, 0.0f, 1.0f));
    }

    model = glm::scale(
        model,
        glm::vec3(sprite.size.x, sprite.size.y, 1.0f));

    // The quad's corners run 0..1, so shift it half a unit to centre it.
    model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));

    return model;
}

glm::mat4 SpriteRenderer::BuildBillboardMatrix(const Sprite& sprite) const
{
    // The camera's right and up axes in world space are the first two columns
    // of the view matrix's rotation part, read across the rows. Building the
    // quad from them makes it exactly parallel to the screen, which is what
    // "faces the camera" means for a billboard.
    const glm::mat4& view = camera->GetViewMatrix();

    glm::vec3 right(view[0][0], view[1][0], view[2][0]);
    glm::vec3 up(view[0][1], view[1][1], view[2][1]);

    // Spin inside the quad's own plane, about the axis pointing at the camera.
    if (sprite.rotationDegrees != 0.0f)
    {
        const glm::vec3 forward = glm::normalize(glm::cross(right, up));

        const glm::mat4 spin = glm::rotate(
            glm::mat4(1.0f),
            glm::radians(sprite.rotationDegrees),
            forward);

        right = glm::vec3(spin * glm::vec4(right, 0.0f));
        up = glm::vec3(spin * glm::vec4(up, 0.0f));
    }

    const glm::vec3 xAxis = right * sprite.size.x;
    const glm::vec3 yAxis = up * sprite.size.y;
    const glm::vec3 zAxis = glm::normalize(glm::cross(right, up));

    // Columns are the quad's local axes; the last is its world position,
    // offset by half its size so the sprite is centred on it.
    glm::mat4 model(1.0f);

    model[0] = glm::vec4(xAxis, 0.0f);
    model[1] = glm::vec4(yAxis, 0.0f);
    model[2] = glm::vec4(zAxis, 0.0f);
    model[3] = glm::vec4(
        sprite.position - xAxis * 0.5f - yAxis * 0.5f,
        1.0f);

    return model;
}

glm::mat4 SpriteRenderer::BuildGroundDecalMatrix(const Sprite& sprite) const
{
    // Laid flat in the XZ plane and lifted clear of the surface it sits on.
    glm::mat4 model(1.0f);

    model = glm::translate(
        model,
        glm::vec3(
            sprite.position.x,
            sprite.position.y + sprite.groundOffset,
            sprite.position.z));

    // Turn on the ground, about the world up axis.
    if (sprite.rotationDegrees != 0.0f)
    {
        model = glm::rotate(
            model,
            glm::radians(sprite.rotationDegrees),
            glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // Tip the quad from standing upright onto its back. This sends the
    // quad's local +Y to world -Z, so the top of the image points away from
    // a camera looking down the lanes, and its normal to world +Y.
    model = glm::rotate(
        model,
        glm::radians(-90.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));

    model = glm::scale(
        model,
        glm::vec3(sprite.size.x, sprite.size.y, 1.0f));

    model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));

    return model;
}

void SpriteRenderer::Draw(const Sprite& sprite)
{
    if (!inPass || !IsReady())
        return;

    if (!sprite.visible || sprite.opacity <= 0.0f)
        return;

    if (sprite.textureName.empty())
        return;

    auto texture = ResourceManager::GetTexture(sprite.textureName);

    if (!texture || !texture->IsValid())
    {
        // Reported once per name. Doing it per frame would bury every other
        // message in the console.
        if (reportedMissing.insert(sprite.textureName).second)
        {
            std::cerr
                << "SpriteRenderer: no loaded texture named \""
                << sprite.textureName
                << "\". Load it with ResourceManager::LoadTexture before"
                   " drawing sprites that use it.\n";
        }

        return;
    }

    const bool screenMode = IsScreenMode(sprite.mode);

    // UI must not be depth-tested against the world, but world sprites must.
    if (screenMode == depthTestEnabled)
    {
        if (screenMode)
            glDisable(GL_DEPTH_TEST);
        else
            glEnable(GL_DEPTH_TEST);

        depthTestEnabled = !screenMode;
    }

    glm::mat4 model(1.0f);

    switch (sprite.mode)
    {
    case SpriteRenderMode::Screen:
        model = BuildScreenMatrix(sprite);
        break;

    case SpriteRenderMode::Billboard:
        model = BuildBillboardMatrix(sprite);
        break;

    case SpriteRenderMode::GroundDecal:
        model = BuildGroundDecalMatrix(sprite);
        break;
    }

    if (screenMode)
    {
        // Pixel coordinates with y growing downwards, so the origin is the
        // window's top-left corner the way UI is normally laid out.
        const glm::mat4 screenProjection = glm::ortho(
            0.0f, screenWidth,
            screenHeight, 0.0f,
            -1.0f, 1.0f);

        shader->SetMatrix4("projection", screenProjection);
        shader->SetMatrix4("view", glm::mat4(1.0f));
    }
    else
    {
        shader->SetMatrix4("projection", camera->GetProjectionMatrix());
        shader->SetMatrix4("view", camera->GetViewMatrix());
    }

    shader->SetMatrix4("model", model);

    glm::vec2 uvMin;
    glm::vec2 uvMax;
    ResolveUV(sprite, uvMin, uvMax);

    shader->SetVector2("uvMin", uvMin);
    shader->SetVector2("uvMax", uvMax);

    shader->SetVector4(
        "tint",
        glm::vec4(sprite.tint, sprite.opacity));

    texture->Bind(SpriteTextureUnit);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void SpriteRenderer::DrawAll(const std::vector<Sprite>& sprites)
{
    if (sprites.empty() || !IsReady())
        return;

    // Sort indices rather than the sprites themselves, so the caller's list is
    // left alone and no Sprite is copied.
    std::vector<std::size_t> order(sprites.size());

    for (std::size_t i = 0; i < order.size(); ++i)
        order[i] = i;

    const glm::vec3 eye = camera
        ? camera->GetPosition()
        : glm::vec3(0.0f);

    // Squared distance is enough for ordering and avoids the square roots.
    std::vector<float> depth(sprites.size(), 0.0f);

    for (std::size_t i = 0; i < sprites.size(); ++i)
    {
        if (!IsScreenMode(sprites[i].mode))
        {
            const glm::vec3 offset = sprites[i].position - eye;
            depth[i] = glm::dot(offset, offset);
        }
    }

    std::stable_sort(
        order.begin(),
        order.end(),
        [&](std::size_t a, std::size_t b)
        {
            const bool screenA = IsScreenMode(sprites[a].mode);
            const bool screenB = IsScreenMode(sprites[b].mode);

            // World sprites first, UI last and therefore on top.
            if (screenA != screenB)
                return screenB;

            if (sprites[a].layer != sprites[b].layer)
                return sprites[a].layer < sprites[b].layer;

            // Within a layer, world sprites go far to near so that blending
            // accumulates in the right order. Screen sprites keep submission
            // order, which stable_sort preserves.
            if (!screenA)
                return depth[a] > depth[b];

            return false;
        });

    Begin();

    for (std::size_t index : order)
        Draw(sprites[index]);

    End();
}
