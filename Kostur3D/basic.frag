#version 330 core

in vec4 vColor;
in vec2 vUV;
in vec3 vN;
in vec3 vWorldPos;

uniform sampler2D uTex;
uniform int useTex;
uniform vec4 uTint;

uniform vec3  uLightPos;
uniform vec3  uLightColor;
uniform float uLightPower;

uniform vec3  uLightPos2;
uniform vec3  uLightColor2;
uniform float uLightPower2;

uniform float uAmbient;

uniform int uUnlit;
uniform float uEmissive;

out vec4 FragColor;

void main()
{
    vec4 base = vColor * uTint;

    if (useTex == 1) {
        vec4 texCol = texture(uTex, vUV);
        if(texCol.a < 0.1) discard;
        base *= texCol;
    }

    if (uUnlit == 1) {
        base.rgb += base.rgb * uEmissive;
        FragColor = base;
        return;
    }

    vec3 N = normalize(vN);

    vec3 L1 = normalize(uLightPos - vWorldPos);
    float diff1 = max(dot(N, L1), 0.0);

    float dist1 = length(uLightPos - vWorldPos);
    float att1 = 1.0 / (1.0 + 0.20 * dist1 + 0.05 * dist1 * dist1);

    vec3 light1 =
        (uAmbient + diff1 * uLightPower * att1) *
        uLightColor;

    vec3 L2 = normalize(uLightPos2 - vWorldPos);
    float diff2 = max(dot(N, L2), 0.0);

    float dist2 = length(uLightPos2 - vWorldPos);
    float att2 = 1.0 / (1.0 + 0.30 * dist2 + 0.10 * dist2 * dist2);

    vec3 light2 =
        (diff2 * uLightPower2 * att2) *
        uLightColor2;

    vec3 totalLight = light1 + light2;

    base.rgb *= totalLight;
    base.rgb += (uLightColor + uLightColor2) * uEmissive * 0.15;

    FragColor = base;
}
