#version 330 core

// World fragment shader: a flat object colour lit by one directional light
// plus an ambient term. Without the lighting every face of a cube would be
// the same colour and the placeholder blocks would read as flat silhouettes.

in vec3 Normal;
in vec2 TexCoord;
in vec3 VertexColor;

out vec4 FragColor;

uniform vec4 objectColor;

// Direction the light travels, pointing away from the sun.
uniform vec3 lightDirection;

// How lit a surface is when it faces fully away from the light.
uniform float ambientStrength;

void main()
{
    vec3 normal = normalize(Normal);

    // Negated because lightDirection points away from the light source.
    float diffuse = max(dot(normal, -normalize(lightDirection)), 0.0);

    float lighting = ambientStrength + (1.0 - ambientStrength) * diffuse;

    // The vertex tint is white unless the mesh was built with a palette, so
    // single-material objects are unaffected by this multiply.
    FragColor = vec4(
        objectColor.rgb * VertexColor * lighting,
        objectColor.a);
}
