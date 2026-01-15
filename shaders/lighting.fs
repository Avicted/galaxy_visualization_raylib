#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;     // Diffuse texture
uniform sampler2D specularMap;  // Specular map
uniform vec4 colDiffuse;
uniform float shininess;        // Shininess (exponent for specular reflection)

// Output fragment color
out vec4 finalColor;

#define     MAX_LIGHTS              8
#define     LIGHT_DIRECTIONAL       0
#define     LIGHT_POINT             1

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
};

// Input lighting values
uniform Light lights[MAX_LIGHTS];
uniform vec4 ambient;
uniform vec3 viewPos;

void main()
{
    // Use per-instance color if provided (non-zero), otherwise use texture
    vec4 baseColor;
    if (fragColor.r > 0.01 || fragColor.g > 0.01 || fragColor.b > 0.01)
    {
        // Per-instance color provided - use it directly
        baseColor = fragColor;
    }
    else
    {
        // No per-instance color - use diffuse texture
        baseColor = texture(texture0, fragTexCoord);
    }
    
    // Fetch specular map value
    vec3 specularMapColor = texture(specularMap, fragTexCoord).rgb;

    // Lighting and specular setup
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 specular = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 1)
        {
            vec3 lightDir = vec3(0.0);

            if (lights[i].type == LIGHT_DIRECTIONAL)
            {
                lightDir = -normalize(lights[i].target - lights[i].position);
            }

            if (lights[i].type == LIGHT_POINT)
            {
                lightDir = normalize(lights[i].position - fragPosition);
            }

            // Diffuse lighting
            float NdotL = max(dot(normal, lightDir), 0.0);
            lightDot += lights[i].color.rgb * NdotL;
        }
    }

    // For per-instance colored objects, use simpler lighting to preserve color
    if (fragColor.r > 0.01 || fragColor.g > 0.01 || fragColor.b > 0.01)
    {
        // Simpler lighting that preserves the instance color better
        float brightness = max(0.4, (lightDot.r + lightDot.g + lightDot.b) / 3.0);
        finalColor = baseColor * brightness;
        finalColor.a = 1.0;
    }
    else
    {
        // Original lighting for textured objects
        finalColor = (baseColor * (colDiffuse + vec4(specular, 1.0)) * vec4(lightDot, 1.0));
        finalColor += baseColor * (ambient / 2.0) * colDiffuse;
    }

    // Gamma correction
    finalColor = pow(finalColor, vec4(1.0 / 2.2));
}
