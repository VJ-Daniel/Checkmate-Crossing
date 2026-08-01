#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>

#include "Mesh.h"

/// Accumulates axis-aligned boxes into one indexed mesh.
///
/// Voxel-style models are made of many small cubes. Drawing each cube as its
/// own object would mean one draw call per cube and a parent/child transform
/// system to hold them together. Instead the boxes are baked into a single
/// vertex buffer here, so a finished model costs exactly one draw call and
/// behaves like any other Mesh.
///
/// Models are authored with y = 0 at the ground, which puts the pivot at the
/// base of the model and lets it be positioned by its footprint.
class MeshBuilder
{
public:

    MeshBuilder();

    /// Sets the tint applied to everything added from here on.
    ///
    /// Keeping it as builder state rather than an argument on every call
    /// keeps the model code readable: name a material once, then add all the
    /// parts made of it. Starts white, which leaves a mesh entirely at the
    /// mercy of its object colour, exactly as before vertex tints existed.
    void SetColor(const glm::vec3& color);

    const glm::vec3& GetColor() const;

    /// Shifts everything added from here on, so a part can be authored in
    /// the model's own coordinates and still come out centred on its pivot.
    ///
    /// This is what lets an animated figure reuse the very same builders as
    /// the static one. A leg has to rotate about its hip, which means its
    /// mesh needs the hip at the origin - but the body builders are written
    /// in ground-relative coordinates, where the hip is most of a unit up.
    /// Setting the origin to the hip and calling the ordinary leg builder
    /// resolves that without a second set of part functions, and without any
    /// risk of the two drifting apart.
    void SetOrigin(const glm::vec3& origin);

    const glm::vec3& GetOrigin() const;

    /// Appends a box given its centre and its size on each axis.
    void AddBox(
        const glm::vec3& center,
        const glm::vec3& size);

    /// Appends a slab centred on the model axis, spanning a vertical range.
    /// This is the natural way to stack the parts of a chess piece.
    void AddSlab(
        float width,
        float depth,
        float bottomY,
        float topY);

    /// Appends a slab offset horizontally, for crenellations and the arms
    /// of the king's cross.
    void AddSlabAt(
        float x,
        float z,
        float width,
        float depth,
        float bottomY,
        float topY);

    /// Appends a square frustum: a box with a smaller top than bottom, so
    /// its four sides slope inwards.
    ///
    /// This is what a tapered chess-piece body actually is. Approximating
    /// the same shape with a stack of straight slabs is what produces the
    /// stepped, ring-covered look of a lathe-turned piece; one frustum is
    /// both simpler geometry and a cleaner silhouette.
    ///
    /// The sloped faces get their own tilted normals, so they catch the
    /// light slightly differently from the vertical faces around them.
    void AddFrustum(
        float bottomWidth,
        float topWidth,
        float bottomY,
        float topY);

    /// A frustum offset horizontally, for palisade stakes and spike beds.
    void AddFrustumAt(
        float x,
        float z,
        float bottomWidth,
        float topWidth,
        float bottomY,
        float topY);

    //---------------------------------------------------------
    // Flat-shaded faces
    //
    // The primitives everything curved is assembled from. Each face gets its
    // own copies of its corners and one normal computed from the winding, so
    // the surface stays faceted instead of smoothing out - which is what
    // keeps a sphere looking carved rather than rendered.
    //
    // Corners must be given counter-clockwise as seen from outside.
    //---------------------------------------------------------

    void AddFlatTriangle(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c);

    void AddFlatQuad(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        const glm::vec3& d);

    /// A faceted sphere, built as a ring-and-segment grid.
    ///
    /// Low counts are the point: 8 segments give an octagonal silhouette that
    /// reads as round while every facet stays visible.
    ///
    /// irregularity pushes each corner in or out by up to that fraction of
    /// the radius, turning a ball into a rock. The displacement fades to zero
    /// at the poles, so the top and bottom stay exactly one radius from the
    /// centre and a lumpy rock still sits flat on the ground.
    ///
    /// seed makes that lumpiness deterministic, so a cached model is the same
    /// shape every run.
    void AddFacetedSphere(
        const glm::vec3& center,
        float radius,
        int segments,
        int rings,
        float irregularity = 0.0f,
        unsigned int seed = 0u);

    /// Which world axis a cylinder's length runs along.
    enum class CylinderAxis
    {
        X,
        Z
    };

    /// A faceted cylinder, with flat side facets and two capped ends.
    ///
    /// A rolling cylinder's axis has to be perpendicular to the way it
    /// travels, so the axis is a parameter rather than baked in: a prop that
    /// crosses the lanes along X needs its length along Z, and vice versa.
    void AddFacetedCylinder(
        const glm::vec3& center,
        float radius,
        float length,
        int sides,
        CylinderAxis axis);

    /// Uploads everything added so far as one mesh.
    std::shared_ptr<Mesh> Build() const;

    void Clear();

    bool IsEmpty() const;

    /// Triangles accumulated so far. Boxes contribute twelve each.
    std::size_t GetTriangleCount() const;

private:

    std::vector<MeshVertex> vertices;

    std::vector<unsigned int> indices;

    glm::vec3 currentColor;

    glm::vec3 currentOrigin;
};
