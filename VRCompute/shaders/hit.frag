#version 450 core

in vec3 vVolumeCoordinates;

layout(location = 0) out vec3 vFragColor;

void main(void)
{
	vFragColor = vVolumeCoordinates;
}