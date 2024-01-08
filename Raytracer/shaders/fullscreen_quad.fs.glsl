#version 330 core
out vec4 FragColor;

// Your texture sampler uniform here
uniform sampler2D yourTexture;
// Screen resolution uniform
uniform vec2 screenResolution;

void main() {
    vec2 stretchedTexCoords = gl_FragCoord.xy / screenResolution;

    // Sample the texture
    vec4 texColor = texture(yourTexture, stretchedTexCoords);

    // Output the sampled color
    FragColor = texColor;
}
