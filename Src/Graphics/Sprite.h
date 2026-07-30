#pragma once

#include <string>

#include <glm.hpp>

/// How a sprite is oriented and projected when it is drawn.
enum class SpriteRenderMode
{
    /// Fixed on the screen, in pixels, through an orthographic projection.
    /// For HUD, icons and menus. Ignores the 3D camera entirely.
    Screen,

    /// A quad at a world position, turned to face the camera. For effects
    /// that should read the same from any camera angle.
    Billboard,

    /// A quad lying flat on the ground, for decals: scorch marks, puddles,
    /// damage zones, floor effects.
    GroundDecal
};

/// A rectangle of a texture, in OpenGL UV space.
///
/// The whole project loads textures flipped (see TextureSettings), so v = 0
/// is the bottom of the picture and v grows upwards. min is the bottom-left
/// corner of the region and max the top-right.
///
/// Build these with the helpers rather than by hand: they take the top-left
/// pixel coordinates an artist reads off a sheet and do the flip for you.
struct SpriteRegion
{
    glm::vec2 min = glm::vec2(0.0f, 0.0f);

    glm::vec2 max = glm::vec2(1.0f, 1.0f);

    /// The whole texture.
    static SpriteRegion Full();

    /// A region given in pixels, measured from the image's TOP-LEFT corner -
    /// the way an image editor shows it.
    ///
    /// Because the texture was flipped on load, the region's top edge in
    /// pixels becomes the larger v and its bottom edge the smaller one:
    ///
    ///     v_max = 1 - y / textureHeight
    ///     v_min = 1 - (y + height) / textureHeight
    ///
    /// Getting this backwards is the classic reason sprites come out upside
    /// down or one row off, so it is done in exactly one place.
    static SpriteRegion FromPixels(
        int x,
        int y,
        int width,
        int height,
        int textureWidth,
        int textureHeight);

    /// One cell of a regular grid. Column 0, row 0 is the TOP-LEFT cell.
    static SpriteRegion FromGrid(
        int column,
        int row,
        int frameWidth,
        int frameHeight,
        int textureWidth,
        int textureHeight);

    /// One cell of a grid described by how many cells it has rather than how
    /// big they are. Column 0, row 0 is again the top-left cell.
    static SpriteRegion FromGridCounts(
        int column,
        int row,
        int columns,
        int rows);

    /// Size of the region in UV space, useful as a uvScale.
    glm::vec2 GetSize() const;
};

/// Everything needed to draw one sprite.
///
/// A plain data struct on purpose: it holds no GL objects and no texture
/// ownership, only the NAME of a texture that ResourceManager holds. Copying
/// a Sprite is free and safe, so gameplay code can keep them in containers,
/// hand them around and rebuild them per frame without worrying about
/// resource lifetime.
struct Sprite
{
    /// Key the texture was registered under with ResourceManager. Empty or
    /// unknown names are skipped by the renderer.
    std::string textureName;

    /// Which part of the texture to draw. Defaults to all of it.
    SpriteRegion region = SpriteRegion::Full();

    /// Screen mode: pixels, origin top-left of the window, and z is ignored.
    /// World modes: world units.
    glm::vec3 position = glm::vec3(0.0f);

    /// Screen mode: pixels. World modes: world units. This is the full size
    /// of the quad, and the sprite is centred on its position.
    glm::vec2 size = glm::vec2(1.0f);

    /// Rotation in degrees inside the sprite's own plane. For a billboard
    /// that is a spin on the screen; for a decal, a turn on the ground.
    float rotationDegrees = 0.0f;

    /// Multiplied into the texture's colour. White leaves it untouched.
    glm::vec3 tint = glm::vec3(1.0f);

    /// Multiplied into the texture's alpha.
    float opacity = 1.0f;

    SpriteRenderMode mode = SpriteRenderMode::Billboard;

    /// Draw order within a batch, low to high. Sprites with the same layer
    /// keep the order they were submitted in.
    int layer = 0;

    bool visible = true;

    bool flipX = false;

    bool flipY = false;

    /// GroundDecal only: how far above the ground to sit, to stay clear of
    /// the surface it lies on. The lane tops and the fake shadows already use
    /// 0.02 for the same reason.
    float groundOffset = 0.02f;

    //---------------------------------------------------------
    // Factories
    //
    // These exist so the common cases read in one line and cannot be built
    // half-configured.
    //---------------------------------------------------------

    static Sprite CreateScreen(
        const std::string& textureName,
        const glm::vec2& pixelPosition,
        const glm::vec2& pixelSize);

    static Sprite CreateBillboard(
        const std::string& textureName,
        const glm::vec3& worldPosition,
        const glm::vec2& worldSize);

    static Sprite CreateGroundDecal(
        const std::string& textureName,
        const glm::vec3& groundPosition,
        const glm::vec2& worldSize);
};
