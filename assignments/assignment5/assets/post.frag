#version 450
layout(location = 0) out vec3 FragColor;

in vec2 UV;
uniform sampler2D _ColorBuffer;
uniform float gamma = 2.2f;

void main()
{ 
    vec3 color = texture(_ColorBuffer,UV).rgb;
    color = pow(color,vec3(1.0/gamma));

    FragColor = color;
}