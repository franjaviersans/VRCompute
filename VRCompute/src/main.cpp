#include "Definitions.h"
#include "GLSLProgram.h"
#include "TransferFunction.h"
#include "TextureManager.h"
#include "CubeIntersection.h"
#include "FBOCube.h"
#include "FBOQuad.h"
#include "Volume.h"
#include "FinalImage.h"
#include <iostream>
#include <sstream>
#include <fstream>

using std::cout;
using std::endl;



namespace glfwFunc
{
	GLFWwindow* glfwWindow;
	int WINDOW_WIDTH = 1024;
	int WINDOW_HEIGHT = 768;
	std::string strNameWindow = "Compute Shader Volume Ray Casting";

	const float NCP = 1.0f;
	const float FCP = 10.0f;
	const float fAngle = 45.f * (3.14f / 180.0f); //In radians

	//Declare the transfer function
	TransferFunction *g_pTransferFunc;

	char * volume_filepath = "./Raw/volume.raw";
	char * transfer_func_filepath = NULL;
	glm::ivec3 vol_size = glm::ivec3(256, 256, 256);
	glm::ivec2 working_group = glm::ivec2(8, 8);
	glm::mat4 scale = glm::mat4();
	bool bits8 = true;
	int offset = 0;

	float color[]={1,1,1};
	bool pintar = false;

	GLSLProgram m_computeProgram;
	glm::mat4x4 mProjMatrix, mModelViewMatrix, mMVP;

	//Variables to do rotation
	glm::quat quater = glm::quat(0.8847f, -.201f, 0.398f, -0.1339f), q2; //begin with a diagonal view
	glm::mat4x4 RotationMat = glm::mat4x4();
	float angle = 0;
	float *vector=(float*)malloc(sizeof(float)*3);
	double lastx, lasty;
	bool pres = false;

#ifdef NOT_RAY_BOX
	CCubeIntersection *m_BackInter, *m_FrontInter;
#endif
	CFinalImage *m_FinalImage;

#ifdef MEASURE_TIME
	std::ofstream time_file("Time.txt", std::ios::out);
	// helper variable
	LARGE_INTEGER temp;
	int num;
	LARGE_INTEGER start_time, end_time;
	double freq, diff_time;
#endif

	Volume *volume = NULL;


	
	///< Callback function used by GLFW to capture some possible error.
	void errorCB(int error, const char* description)
	{
		printf("%s\n",description );
	}



	///
	/// The keyboard function call back
	/// @param window id of the window that received the event
	/// @param iKey the key pressed or released
	/// @param iScancode the system-specific scancode of the key.
	/// @param iAction can be GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT
	/// @param iMods Bit field describing which modifier keys were held down (Shift, Alt, & so on)
	///
	void keyboardCB(GLFWwindow* window, int iKey, int iScancode, int iAction, int iMods)
	{
		if (iAction == GLFW_PRESS)
		{
			switch (iKey)
			{
				case GLFW_KEY_ESCAPE:
				case GLFW_KEY_Q:
					glfwSetWindowShouldClose(window, GL_TRUE);
					break;
				case GLFW_KEY_SPACE:
					g_pTransferFunc->isVisible = !g_pTransferFunc->isVisible;
					break;
			}
		}
	}

	inline int TwEventMousePosGLFW3(GLFWwindow* window, double xpos, double ypos)
	{ 
	
		g_pTransferFunc->CursorPos(int(xpos), int(ypos));
		if(pres){
			//Rotation
			float dx = float(xpos - lastx);
			float dy = float(ypos - lasty);

			if(!(dx == 0 && dy == 0)){
				//Calculate angle and rotation axis
				float angle = sqrtf(dx*dx + dy*dy)/50.0f;
					
				//Acumulate rotation with quaternion multiplication
				q2 = glm::angleAxis(angle, glm::normalize(glm::vec3(dy,dx,0.0f)));
				quater = glm::cross(q2, quater);

				lastx = xpos;
				lasty = ypos;
			}
			return false;
		}
		return true;
	}

	int TwEventMouseButtonGLFW3(GLFWwindow* window, int button, int action, int mods)
	{ 
		pres = false;

		double x, y;   
		glfwGetCursorPos(window, &x, &y);  

		if (!g_pTransferFunc->MouseButton((int)x, (int)y, button, action)){
			
			if(button == GLFW_MOUSE_BUTTON_LEFT){
				if(action == GLFW_PRESS){
					lastx = x;
					lasty = y;
					pres = true;
				}

				return true;
			}else{
				if(action == GLFW_PRESS){			
				}
				
			}
			
			return false;
		}

		return true;
	}
	
	///< The resizing function
	void resizeCB(GLFWwindow* window, int iWidth, int iHeight)
	{

		WINDOW_WIDTH = iWidth;
		WINDOW_HEIGHT = iHeight;

		if(iHeight == 0) iHeight = 1;
		float ratio = iWidth / float(iHeight);
		glViewport(0, 0, iWidth, iHeight);

		mProjMatrix = glm::perspective(float(fAngle), ratio, NCP, FCP);
	//	mProjMatrix = glm::ortho(-1.0f,1.0f,-1.0f,1.0f,-1.0f,5.0f);



		// Update size in some buffers!!!
#ifdef NOT_RAY_BOX
		m_BackInter->SetResolution(iWidth, iHeight);
		m_FrontInter->SetResolution(iWidth, iHeight);
#endif
		m_FinalImage->SetResolution(iWidth, iHeight);
		g_pTransferFunc->Resize(&WINDOW_WIDTH, &WINDOW_HEIGHT);

		m_computeProgram.use();
		{
			//Bind the texture
			glBindImageTexture(0, TextureManager::Inst()->GetID(TEXTURE_FINAL_IMAGE), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
			//glBindImageTexture(1, TextureManager::Inst()->GetID(TEXTURE_TRANSFER_FUNC), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
			//glBindImageTexture(2, TextureManager::Inst()->GetID(TEXTURE_VOLUME), 0, GL_TRUE, 0, GL_READ_ONLY, GL_R8);
#ifdef NOT_RAY_BOX
			/*glBindImageTexture(3, TextureManager::Inst()->GetID(TEXTURE_BACK_HIT), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
			glBindImageTexture(4, TextureManager::Inst()->GetID(TEXTURE_FRONT_HIT), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);*/
#else
			m_computeProgram.setUniform("constantWidth", WINDOW_WIDTH);
			m_computeProgram.setUniform("constantHeight", WINDOW_HEIGHT);
			m_computeProgram.setUniform("constantAngle", fAngle);
			m_computeProgram.setUniform("constantNCP", NCP);
#endif
		}
		
	}


	///< Function to warup opencl
	void WarmUP(unsigned int cycles, bool measure = false){

		RotationMat = glm::mat4_cast(glm::normalize(quater));

		mModelViewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -2.0f)) *
			RotationMat * scale;

		mMVP = mProjMatrix * mModelViewMatrix;

		//Obtain Back hits
#ifdef NOT_RAY_BOX
		m_BackInter->Draw(mMVP);
		//Obtain the front hits
		m_FrontInter->Draw(mMVP);
#endif

#ifdef MEASURE_TIME
		if (measure){
			QueryPerformanceCounter((LARGE_INTEGER *)&start_time);	//set start time
		}
#endif

		//Draw a Cube
		m_computeProgram.use();
		{

			g_pTransferFunc->Use(GL_TEXTURE1);
			volume->Use(GL_TEXTURE2);
#ifdef NOT_RAY_BOX
			m_BackInter->Use(GL_TEXTURE3);
			m_FrontInter->Use(GL_TEXTURE4);
#else
			m_computeProgram.setUniform("c_invViewMatrix", glm::inverse(mModelViewMatrix));
#endif
			
			
			for (int i = 0; i < cycles; ++i) {
					//Do calculation with Compute Shader
					glDispatchCompute((WINDOW_WIDTH + working_group.x) / working_group.x, (WINDOW_HEIGHT + working_group.y) / working_group.y, 1);

					//Wait for memory writes
					glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			
			}
		}

#ifdef MEASURE_TIME
		if (measure){
			QueryPerformanceCounter((LARGE_INTEGER *)&end_time); //end time
			diff_time = (float)(((double)end_time.QuadPart - (double)start_time.QuadPart) / freq); // get total time
			diff_time /= cycles; // get time per cycle
			time_file << diff_time << endl;
			time_file.close();
		}
#endif

	}

	///< The main rendering function.
	void draw()
	{

		GLenum err = GL_NO_ERROR;
		while((err = glGetError()) != GL_NO_ERROR)
		{
		  std::cout<<"INICIO "<< err<<std::endl;
		}

#ifdef MEASURE_TIME
		QueryPerformanceCounter((LARGE_INTEGER *)&start_time);	//set start time
#endif
		
		RotationMat = glm::mat4_cast(glm::normalize(quater));

		mModelViewMatrix =  glm::translate(glm::mat4(), glm::vec3(0.0f,0.0f,-2.0f)) * 
							RotationMat * scale; 


		mMVP = mProjMatrix * mModelViewMatrix;



		//Obtain Back hits
#ifdef NOT_RAY_BOX
		m_BackInter->Draw(mMVP);
		//Obtain the front hits
		m_FrontInter->Draw(mMVP);
#endif

		//Draw a Cube
		m_computeProgram.use();
		{	


#ifdef NOT_RAY_BOX
			g_pTransferFunc->Use(GL_TEXTURE1);
			volume->Use(GL_TEXTURE2);
			m_BackInter->Use(GL_TEXTURE3);
			m_FrontInter->Use(GL_TEXTURE4);
#else
			m_computeProgram.setUniform("c_invViewMatrix", glm::inverse(mModelViewMatrix));
#endif

			//Do calculation with Compute Shader
			glDispatchCompute((WINDOW_WIDTH + working_group.x) / working_group.x, (WINDOW_HEIGHT + working_group.y) / working_group.y, 1);

			//Wait for memory writes
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			//bind the default texture to the image unit, hopefully freeing ours for editing
			/*glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
			glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
			glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
			glBindImageTexture(3, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
			glBindImageTexture(4, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);*/
			

			//glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			//glPointSize(10);
			
			//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}


		//Draw the quad with the final result
		glClearColor(0.15f, 0.15f, 0.15f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		//Blend with bg
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		m_FinalImage->Draw();
		glDisable(GL_BLEND);



		//Draw the AntTweakBar
		//TwDraw();

		//Draw the transfer function
		g_pTransferFunc->Display();

		glfwSwapBuffers(glfwWindow);

#ifdef MEASURE_TIME
		QueryPerformanceCounter((LARGE_INTEGER *)&end_time); //end time
		diff_time += (float)(((double)end_time.QuadPart - (double)start_time.QuadPart) / freq); // get total time
		++num;
#endif

		while((err = glGetError()) != GL_NO_ERROR)
		{
		  std::cout<<"Swap "<< err<<std::endl;
		  //exit(0);
		}

		
	}
	

	///
	/// Init all data and variables.
	/// @return true if everything is ok, false otherwise
	///
	bool initialize()
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);

		//Init GLEW
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK)
		{
			printf("- glew Init failed :(\n");
			return false;
		}
		printf("OpenGL version: %s\n", glGetString(GL_VERSION));
		printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
		printf("Vendor: %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));


		//Init the transfer function
		g_pTransferFunc = new TransferFunction();
		g_pTransferFunc->InitContext(glfwWindow, &WINDOW_WIDTH, &WINDOW_HEIGHT, transfer_func_filepath, - 1, -1);

		// send window size events to AntTweakBar
		glfwSetWindowSizeCallback(glfwWindow, resizeCB);
		glfwSetMouseButtonCallback(glfwWindow, (GLFWmousebuttonfun)TwEventMouseButtonGLFW3);
		glfwSetCursorPosCallback(glfwWindow, (GLFWcursorposfun)TwEventMousePosGLFW3);
		glfwSetKeyCallback(glfwWindow, (GLFWkeyfun)keyboardCB);



		//Create volume
		volume = new Volume();
		volume->Load(volume_filepath, vol_size.x, vol_size.y, vol_size.z, bits8, offset);

		//Set compute shader
		try{
			std::ostringstream oss;
			oss << "#version 450 core \n layout(local_size_x = " << working_group.x << ", local_size_y = " << working_group.y<<", local_size_z = 1) in; \n";
			std::string header = oss.str();
#ifdef NOT_RAY_BOX
			m_computeProgram.compileShader("./shaders/compute.cs", GLSLShader::COMPUTE, header);
#else
			m_computeProgram.compileShader("./shaders/computeraybox.cs", GLSLShader::COMPUTE, header);
#endif
			m_computeProgram.link();
		}
		catch (GLSLProgramException & e) {
			std::cerr << e.what() << std::endl;
			exit(EXIT_FAILURE);
		}
		m_computeProgram.use();
		{
			//Set the value of h
			m_computeProgram.setUniform("h", 1.0f / volume->m_fDiagonal);
			m_computeProgram.setUniform("width", volume->m_fWidht);
			m_computeProgram.setUniform("height", volume->m_fHeigth);
			m_computeProgram.setUniform("depth", volume->m_fDepth);

#ifndef NOT_RAY_BOX
			m_computeProgram.setUniform("constantWidth", WINDOW_WIDTH);
			m_computeProgram.setUniform("constantHeight", WINDOW_HEIGHT);
			m_computeProgram.setUniform("constantAngle", fAngle);
			m_computeProgram.setUniform("constantNCP", NCP);
#endif
		}

#ifdef NOT_RAY_BOX
		m_BackInter = new CCubeIntersection(false, WINDOW_WIDTH, WINDOW_HEIGHT);
		m_FrontInter = new CCubeIntersection(true, WINDOW_WIDTH, WINDOW_HEIGHT);
#endif
		m_FinalImage = new CFinalImage(WINDOW_WIDTH, WINDOW_HEIGHT);

#ifdef MEASURE_TIME
		// get the tick frequency from the OS
		QueryPerformanceFrequency((LARGE_INTEGER *)&temp);
		freq = ((double)temp.QuadPart) / 1000.0; //convert to the time needed
		num = 0;
#endif


		return true;
	}


	/// Here all data must be destroyed + glfwTerminate
	void destroy()
	{
#ifdef NOT_RAY_BOX
		delete m_BackInter;
		delete g_pTransferFunc;
#endif
		TextureManager::Inst()->UnloadAllTextures();
		glfwTerminate();
		glfwDestroyWindow(glfwWindow);
	}
}

int main(int argc, char** argv)
{

#ifdef NOT_RAY_BOX
	cout << "Using image intersection" << endl;
#else
	cout << "Using ray box intersection" << endl;
#endif

#ifdef NOT_DISPLAY
	cout << "NOT display" << endl;
#else
	cout << "Display result in screen" << endl;
#endif

#ifdef MEASURE_TIME
	cout << "Measuring time" << endl;
#else
	cout << "NOT Measuring time" << endl;
#endif

	if (argc == 12 || argc == 13) {

		//Copy volume file path
		glfwFunc::volume_filepath = new char[strlen(argv[1]) + 1];
		strncpy_s(glfwFunc::volume_filepath, strlen(argv[1]) + 1, argv[1], strlen(argv[1]));

		//Volume size
		int width = atoi(argv[2]), height = atoi(argv[3]), depth = atoi(argv[4]);
		glfwFunc::vol_size = glm::ivec3(width, height, depth);

		//number of bits;
		int bits = atoi(argv[5]);
		glfwFunc::bits8 = (bits == 8);

		//scale factor of the volume
		glfwFunc::scale = glm::scale(glm::mat4(), glm::vec3(atof(argv[6]), atof(argv[7]), atof(argv[8])));

		//offset
		glfwFunc::offset = atoi(argv[9]);

		//working group size
		glfwFunc::working_group.x = atoi(argv[10]);
		glfwFunc::working_group.y = atoi(argv[11]);

		//Copy volume transfer function path
		if (argc == 13){
			glfwFunc::transfer_func_filepath = new char[strlen(argv[12]) + 1];
			strncpy_s(glfwFunc::transfer_func_filepath, strlen(argv[12]) + 1, argv[12], strlen(argv[12]));
		}

	}
	else if (argc > 13) {
		printf("Too many arguments supplied!!!! \n");
	}


	glfwSetErrorCallback(glfwFunc::errorCB);
	if (!glfwInit())	exit(EXIT_FAILURE);
	glfwFunc::glfwWindow = glfwCreateWindow(glfwFunc::WINDOW_WIDTH, glfwFunc::WINDOW_HEIGHT, glfwFunc::strNameWindow.c_str(), NULL, NULL);
	if (!glfwFunc::glfwWindow)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(glfwFunc::glfwWindow);
	if(!glfwFunc::initialize()) exit(EXIT_FAILURE);
	glfwFunc::resizeCB(glfwFunc::glfwWindow, glfwFunc::WINDOW_WIDTH, glfwFunc::WINDOW_HEIGHT);	//just the 1st time


	//WarmUP!!!!
	glfwFunc::WarmUP(20);
	
#ifndef NOT_DISPLAY
	// main loop!
#ifndef MEASURE_TIME
	while (!glfwWindowShouldClose(glfwFunc::glfwWindow))
	{
#else
	while (!glfwWindowShouldClose(glfwFunc::glfwWindow) && glfwFunc::num <= NUM_CYCLES)
	{
#endif

		if(glfwFunc::g_pTransferFunc->updateTexture) // Check if the color palette changed    
		{
			glfwFunc::g_pTransferFunc->UpdatePallete();
			glfwFunc::g_pTransferFunc->updateTexture = false;

		}
		glfwFunc::draw();
		glfwPollEvents();	//or glfwWaitEvents()
	}

#ifdef MEASURE_TIME
	glfwFunc::diff_time /= glfwFunc::num; // get time per cycle
	glfwFunc::time_file << glfwFunc::diff_time << endl;
	glfwFunc::time_file.close();
#endif
#else

	glfwFunc::WarmUP(NUM_CYCLES, true);

#endif

	glfwFunc::destroy();
	return EXIT_SUCCESS;
}
