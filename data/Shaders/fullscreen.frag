out vec4 color;

in vec2 texCoords;

uniform sampler2D tex;

void main()
{
    vec3 texColor = texture(tex, texCoords).rgb;
    color = vec4(texColor, 1.0);
}