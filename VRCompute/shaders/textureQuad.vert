#version 450 core

uniform mat4 mTransform;

layout(location = 0) in vec2 vVertexCoord;
layout(location = 3) in vec2 vTextureCoord;

out vec2 vVertexTexture;

void main()
{
	vVertexTexture = vTextureCoord;

	gl_Position = mTransform * vec4(vVertexCoord,0.0f,1.0f);
	
}