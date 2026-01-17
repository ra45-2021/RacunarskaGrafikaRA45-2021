#version 330 core

in vec4 vColor;
in vec2 vUV;
in vec3 vN;

uniform sampler2D uTex;
uniform int useTex;
uniform int transparent;
uniform vec4 uTint;
uniform vec3 uLightDir; 

uniform int uUnlit;

out vec4 FragColor;

void main()
{
    vec4 col = vColor * uTint;

    if (useTex == 1) {
        vec4 texCol = texture(uTex, vUV);
        if(texCol.a < 0.1) discard; 
        col *= texCol;
    }

    if (uUnlit == 1) {
        FragColor = col;      
        return;
    }

    vec3 N = normalize(vN);
    float diff = max(dot(N, normalize(-uLightDir)), 0.0);

    float ambient = 0.85;          
    float diffuseStrength = 0.35;  

    float lighting = ambient + diffuseStrength * diff;

    if (useTex == 0) {
        col.rgb *= lighting;
    }

    FragColor = col;
}