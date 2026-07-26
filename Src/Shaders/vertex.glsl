#version 330 core

// Sprite vertex shader: transforms a unit quad into world space and maps
// its local UV coordinates into the currently selected sprite-sheet frame.

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec2 uvMin;
uniform vec2 uvMax;

out vec2 TexCoord;

void main()
{
    gl_Position =
        projection *
        view *
        model *
        vec4(aPosition, 1.0);

    // Remap the quad's 0..1 UV range to one cell of the sprite sheet.
    TexCoord = mix(uvMin, uvMax, aTexCoord);
}
