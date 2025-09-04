#version 440 core

out vec4 FragColor;
in vec2 uvOut;
uniform sampler2D uTexture;

void main() {
    FragColor = vec4(texture(uTexture, uvOut).rgb, 1.0);
    // FragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);
}
