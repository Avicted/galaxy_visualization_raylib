#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

const float THRESHOLD = 0.2;
const float SOFT_KNEE = 0.5;
const float INTENSITY = 3.5;
const float KERNEL_SCALE = 24.0;

vec3 sampleBright(vec2 uv)
{
    vec2 clamped_uv = clamp(uv, vec2(0.0), vec2(1.0));
    vec3 color = texture(texture0, clamped_uv).rgb;
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float knee = THRESHOLD * SOFT_KNEE + 1e-5;
    float soft = smoothstep(THRESHOLD - knee, THRESHOLD + knee, luma);
    return color * soft;
}

void main()
{
    vec2 texel = 1.0 / vec2(textureSize(texture0, 0));
    vec3 bloom = vec3(0.0);
    float weight_sum = 0.0;

    const int SAMPLES = 64;
    const float GOLDEN_ANGLE_RAD = 2.39996323;

    for (int i = 0; i < SAMPLES; ++i)
    {
        float t = (float(i) + 0.5) / float(SAMPLES);
        float radius = t * KERNEL_SCALE;
        float angle = float(i) * GOLDEN_ANGLE_RAD;
        vec2 offset = vec2(cos(angle), sin(angle)) * radius * texel;

        float w = pow(1.0 - t, 2.0);
        bloom += sampleBright(fragTexCoord + offset) * w;
        weight_sum += w;
    }

    bloom += sampleBright(fragTexCoord) * 0.5;
    weight_sum += 0.5;

    bloom /= max(weight_sum, 1e-5);

    vec4 color = vec4(bloom * INTENSITY, 1.0) * colDiffuse * fragColor;
    finalColor = color;
}
