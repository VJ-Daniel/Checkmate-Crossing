#version 330 core

// Sprite vertex shader: transforms one shared unit quad and maps its local
// 0..1 corners into whatever region of the texture the sprite selected.
//
// One shader serves all three sprite modes. The difference between a screen
// sprite, a camera-facing billboard and a ground decal is entirely in the
// matrices the renderer uploads, so nothing here has to branch.

layout(location = 0) in vec2 aCorner;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Bottom-left and top-right of the region, already flipped if the sprite
// asked for it, so the fragment stage needs no flip logic.
uniform vec2 uvMin;
uniform vec2 uvMax;

out vec2 TexCoord;

void main()
{
    gl_Position =
        projection *
        view *
        model *
        vec4(aCorner, 0.0, 1.0);

    // aCorner runs 0..1 across the quad, which is exactly the interpolant
    // needed to walk from one corner of the region to the other.
    TexCoord = mix(uvMin, uvMax, aCorner);
}
