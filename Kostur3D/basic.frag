#version 330 core

in vec4 vColor;
in vec2 vUV;
in vec3 vN;

uniform sampler2D uTex;
uniform int useTex;
uniform int transparent;
uniform vec4 uTint;

uniform vec3 uLightDir; 

out vec4 FragColor;

void main()
{
    vec4 col = vColor * uTint;

    if (useTex == 1) {
        col *= texture(uTex, vUV);
    }

    vec3 N = normalize(vN);
    float diff = max(dot(N, normalize(-uLightDir)), 0.0);

    float ambient = 0.30;
    float lighting = ambient + (1.0 - ambient) * diff;

    col.rgb *= lighting;

    FragColor = col;
}
