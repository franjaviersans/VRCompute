#version 450 core


layout(binding = 0) uniform sampler2D tex;

in vec2 vVertexTexture;

layout(location = 0) out vec4 vFragColor;

void main(void)
{
	vFragColor = texture(tex, vVertexTexture);
}