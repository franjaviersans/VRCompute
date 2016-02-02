// $Id: $
//
// Author: Francisco Sans franjaviersans@gmail.com
//
// Complete history on bottom of file

#ifndef FINAL_H
#define FINAL_H

//Includes
#include "Definitions.h"
#include "FBOCube.h"
#include "GLSLProgram.h"


class CFinalImage
{
	//Functions

public:
	///Default constructor
	CFinalImage(GLuint W, GLuint H);

	///Default destructor
	~CFinalImage();

	///Set resolution
	void SetResolution(GLuint ResW, GLuint ResH);

	///Method to Draw the Quad
	void Draw();

	///Use texture
	void Use(GLenum);

	//Variables
private:
	GLuint m_uiWidth, m_uiHeight;
	GLSLProgram m_program;
};


#endif //FBOQuad_H