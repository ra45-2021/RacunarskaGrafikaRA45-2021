#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aNormal;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

out vec4 vColor;
out vec2 vUV;
out vec3 vN;
out vec3 vWorldPos;   // NOVO

void main()
{
    vColor = aColor;
    vUV = aUV;

    vec4 world = uM * vec4(aPos, 1.0);   
    vWorldPos = world.xyz;              

    mat3 Nmat = mat3(transpose(inverse(uM)));
    vN = normalize(Nmat * aNormal);

    gl_Position = uP * uV * world;   
}
