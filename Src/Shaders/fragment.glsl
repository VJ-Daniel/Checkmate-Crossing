#version 330 core

// Sprite fragment shader: samples RGBA texture data, discards transparent
// pixels and applies the sprite tint (also used by the Game Over fade).

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D image;

uniform vec4 spriteColor;

void main()
{
    vec4 texColor = texture(image, TexCoord);

    // Avoid writing fully transparent sprite-sheet padding to the frame.
    if (texColor.a <= 0.01)
        discard;

    FragColor = texColor * spriteColor;
}
