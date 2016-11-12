// $Id: $
//
// Author: Francisco Sans franjaviersans@gmail.com
//
// Complete history on bottom of file

#define FILE_REVISION "$Revision: $"

#include "FinalImage.h"
#include "TextureManager.h"
#include <stdlib.h>
#include <iostream>



/**
* Default constructor
*/
CFinalImage::CFinalImage(GLuint W, GLuint H) :m_uiWidth(W), m_uiHeight(H)
{
	//load the shaders
	try{
		m_program.compileShader("shaders/textureQuad.vert", GLSLShader::VERTEX);
		m_program.compileShader("shaders/textureQuad.frag", GLSLShader::FRAGMENT);
		m_program.link();
	}
	catch (GLSLProgramException & e) {
		std::cerr << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}

	SetResolution(W, H);
};

/**
* Default destructor
*/
CFinalImage::~CFinalImage()
{

}

/**
* Set resolution
*
* @params ResW new window width resolution
* @params ResH new window hegith resolution
*
*/
void CFinalImage::SetResolution(GLuint ResW, GLuint ResH)
{
	m_uiWidth = ResW;
	m_uiHeight = ResH;

	//If it exists, unload
	TextureManager::Inst().UnloadTexture(TEXTURE_FINAL_IMAGE);

	//Create texture!!
	glActiveTexture(GL_TEXTURE0);

	//Create new empty textures
	TextureManager::Inst().CreateTexture2D(TEXTURE_FINAL_IMAGE, m_uiWidth, m_uiHeight, GL_RGBA8, GL_RGBA, GL_FLOAT, GL_LINEAR, GL_LINEAR);

	m_program.use();
	{
		glm::mat4 trans = glm::translate(glm::mat4(1), glm::vec3(0.5, 0.5, 0.0f));
		glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(m_uiWidth, m_uiHeight, 1.0f));
		glm::mat4 proy = glm::ortho(0.0f, float(m_uiWidth), 0.0f, float(m_uiHeight), -1.0f, 1.0f);
		glm::mat4 mat = proy * scale * trans;
		m_program.setUniform("mTransform", mat);
	}
}

/**
* Method to Draw the Quad
*/
void CFinalImage::Draw()
{
	//Draw a Cube
	m_program.use();
	{
		Use(GL_TEXTURE0);
		VBOCube::Instance().Draw();
	}
}


/**
* Function to use the texture
*/
void CFinalImage::Use(GLenum activeTexture)
{
	glActiveTexture(activeTexture);
	TextureManager::Inst().BindTexture(TEXTURE_FINAL_IMAGE);
}


#undef FILE_REVISION

// Revision History:
// $Log: $
// $Header: $
// $Id: $