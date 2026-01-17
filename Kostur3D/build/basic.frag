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
        vec4 texCol = texture(uTex, vUV);
        if(texCol.a < 0.1) discard; 
        col *= texCol;
    }

    vec3 N = normalize(vN);
    float diff = max(dot(N, normalize(-uLightDir)), 0.0);
    float ambient = 0.65;
    float lighting = ambient + (1.0 - ambient) * diff;

    if (useTex == 0) {
        col.rgb *= lighting;
    }

    FragColor = col;
}