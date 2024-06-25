out vec4 color;

uniform sampler2D sceneColorsTexture;
uniform float exposure;

in vec2 texCoords;

void main()
{
    vec4 sceneColor = texture(sceneColorsTexture, texCoords);
    vec3 tonemappedColor = vec3(1.0) - exp(-sceneColor.rgb * exposure);
    vec3 gammaCorrectedColor = pow(tonemappedColor, vec3(1.0 / 2.2));
    color = vec4(gammaCorrectedColor, sceneColor.a);
}