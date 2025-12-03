#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D tex;
uniform vec3 color;
uniform float alpha;

void main() {
    vec4 texColor = texture(tex, TexCoord);
    FragColor = vec4(color, 1.0) * texColor * vec4(1.0,1.0,1.0,alpha);
}
