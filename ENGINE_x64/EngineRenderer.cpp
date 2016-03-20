#include "EngineRenderer.h"

//The surface contained by the window
SDL_Surface* gScreenSurface = NULL;

EngineRenderer::EngineRenderer()
{
}

EngineRenderer::~EngineRenderer()
{
	//Destroy window	
	SDL_DestroyWindow(EngineWindow);
	EngineWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();

	printf("Engine Terminated\n");
}

bool EngineRenderer::InitializeEngine()
{
	printf("Initializing Engine\n");

	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//ActivateShader OpenGL 2.1
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, );
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		//Create window
		EngineWindow = SDL_CreateWindow("ENGINE_WINx64",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			1280, 720,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

		if (EngineWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Initialize PNG loading
			int imgFlags = IMG_INIT_PNG;
			if (!(IMG_Init(imgFlags) & imgFlags))
			{
				printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
				success = false;
			}
			else
			{
				//Get window surface
				gScreenSurface = SDL_GetWindowSurface(EngineWindow);
			}

			//Create context
			EngineContext = SDL_GL_CreateContext(EngineWindow);
			if (EngineContext == NULL)
			{
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//ActivateShader Vsync
				if (SDL_GL_SetSwapInterval(1) < 0)
				{
					printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
				}

				//Initialize OpenGL
				if (!InitializeOpenGL())
				{
					printf("Unable to initialize OpenGL!\n");
					success = false;
				}


			}
		}
	}

	return success;
}

bool EngineRenderer::InitializeOpenGL()
{
	printf("Initializing OpenGL\n");

	GLenum error = GL_NO_ERROR;

	glClearColor(0.f, 0.f, 0.f, 1.0f);

	glewExperimental = GL_TRUE;
	error = glewInit();
	if (GL_TRUE != glewGetExtension("GL_EXT_texture3D"))
	{

	}

	glViewport(0, 0, 1280, 720);
	glMatrixMode(GL_PROJECTION);
	float aspect = (float)1280 / (float)720;
	glOrtho(-aspect, aspect, -1, 1, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	printf("OpenGL Device: (%s): \n", glGetString(GL_RENDERER));

	return true;
}

void EngineRenderer::HandleInput(unsigned char targetKey, int xPos, int yPos)
{
	//Toggle Rendering
	if (targetKey == 'q')
	{
		EnableRendering = !EnableRendering;
	}
}

void EngineRenderer::Update()
{	
}

void EngineRenderer::Render()
{
	//Clear color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void EngineRenderer::InitializeRenderData(std::vector<GLchar> &renderData,int lCount)
{
	int texID = 0;
	glGenTextures(1, (GLuint*)&texID);

	texID = 1;

	glBindTexture(GL_TEXTURE_3D, texID);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 500, 512, lCount, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, renderData.data());
	glBindTexture(GL_TEXTURE_3D, 0);

	ObjectOrientation[0] = ObjectOrientation[5] = ObjectOrientation[10] = ObjectOrientation[15] = 1.0;
	ObjectOrientation[1] = ObjectOrientation[2] = ObjectOrientation[3] = ObjectOrientation[4] = 0.0f;
	ObjectOrientation[6] = ObjectOrientation[7] = ObjectOrientation[8] = ObjectOrientation[9] = 0.0f;
	ObjectOrientation[11] = ObjectOrientation[12] = ObjectOrientation[13] = ObjectOrientation[14] = 0.0f;
}


