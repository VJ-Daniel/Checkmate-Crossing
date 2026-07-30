#version 330 core

// Sprite fragment shader: samples the texture and applies the sprite's tint
// and opacity.
//
// Sprites are unlit on purpose. They are meant for effects and UI, which read
// better at a constant brightness than shaded against the world's directional
// light, and it keeps this shader independent of the 3D lighting setup.

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D spriteTexture;

// rgb is the tint, a is the opacity.
uniform vec4 tint;

// Fragments at or below this alpha are thrown away rather than blended.
//
// This matters for world sprites: a fully transparent fragment that reaches
// the blend stage still costs a blend, and if depth writing is ever enabled it
// would also stamp the quad's rectangle into the depth buffer and punch a hole
// through whatever is behind it.
uniform float alphaCutoff;

void main()
{
    vec4 sampled = texture(spriteTexture, TexCoord);

    vec4 result = sampled * tint;

    if (result.a <= alphaCutoff)
        discard;

    FragColor = result;
}
