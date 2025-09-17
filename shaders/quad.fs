#version 440 core

out vec4 FragColor;
in vec2 uvOut;
uniform sampler2D uTexture;

vec3 RRTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 toneMapACES(vec3 color) {
    // ACES approximation
    const float exposureBias = .8;
    color *= exposureBias;
    return clamp(RRTAndODTFit(color), 0.0, 1.0);
}

void main() {

    vec3 colorOut = texture(uTexture, uvOut).rgb;
    
    const float gamma = 2.2;
    vec3 mapped       = toneMapACES(colorOut);
    mapped            = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);
    // FragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);
}
