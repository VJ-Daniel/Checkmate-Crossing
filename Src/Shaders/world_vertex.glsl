#version 330 core

// World vertex shader: transforms battlefield geometry through the standard
// model / view / projection chain and passes the surface normal on to the
// fragment stage so each face can be shaded separately.

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 Normal;
out vec2 TexCoord;
out vec3 VertexColor;

void main()
{
    gl_Position =
        projection *
        view *
        model *
        vec4(aPosition, 1.0);

    // The normal matrix keeps normals pointing the right way even when the
    // model is scaled unevenly, which every lane quad is.
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Passed through for future texture mapping (GDD section 5).
    TexCoord = aTexCoord;

    // Lets one mesh hold several materials. White for anything that only
    // needs the object's own flat colour.
    VertexColor = aColor;
}
