#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

in mat4 instanceTransform;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

void main()
{
    // Compute MVP for current instance
    mat4 mvpi = mvp * instanceTransform;

    // Send vertex attributes to fragment shader
    fragPosition = vec3(mvpi * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));

    // Extract per-instance color from unused matrix slots
    // Color is stored in: m3 (column 0, row 3), m7 (column 1, row 3), m11 (column 2, row 3)
    fragColor = vec4(instanceTransform[0][3], instanceTransform[1][3], instanceTransform[2][3], 1.0);

    // Calculate final vertex position
    gl_Position = mvpi*vec4(vertexPosition, 1.0);
}