#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;     // Diffuse texture
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

#define     MAX_LIGHTS              4
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
    // Branchless color selection: use instance color if luminance > threshold, else texture
    float instanceLuminance = dot(fragColor.rgb, vec3(0.299, 0.587, 0.114));
    float useInstance = step(0.01, instanceLuminance);
    vec4 texColor = texture(texture0, fragTexCoord);
    vec4 baseColor = mix(texColor, fragColor, useInstance);

    // Simplified lighting - single directional light calculation
    vec3 normal = normalize(fragNormal);
    vec3 lightDot = vec3(0.0);

    // Unrolled loop for first light only (most common case)
    if (lights[0].enabled == 1)
    {
        vec3 lightDir = (lights[0].type == LIGHT_DIRECTIONAL)
            ? -normalize(lights[0].target - lights[0].position)
            : normalize(lights[0].position - fragPosition);
        lightDot = lights[0].color.rgb * max(dot(normal, lightDir), 0.0);
    }

    // Branchless lighting blend
    float brightness = max(0.4, (lightDot.r + lightDot.g + lightDot.b) * 0.333);
    vec4 instanceResult = baseColor * brightness;
    instanceResult.a = 1.0;
    
    vec4 texturedResult = baseColor * colDiffuse * vec4(lightDot, 1.0);
    texturedResult += baseColor * (ambient * 0.5) * colDiffuse;
    
    finalColor = mix(texturedResult, instanceResult, useInstance);
}
