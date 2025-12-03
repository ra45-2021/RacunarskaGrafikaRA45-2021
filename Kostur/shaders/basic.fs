#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D tex;
uniform float alpha;

void main()
{
    FragColor = texture(tex, TexCoord) * vec4(1.0, 1.0, 1.0, alpha);
}
