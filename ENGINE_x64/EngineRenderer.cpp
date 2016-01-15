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
			SCREENWIDTH, SCREENHEIGHT,
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

	bool success = true;
	GLenum error = GL_NO_ERROR;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		success = false;
	}

	//Initialize Modelview Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//Check for error
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		//printf("Error initializing OpenGL! %s\n", gluErrorString(error));
		success = false;
	}

	//Initialize clear color
	glClearColor(0.f, 0.f, 0.f, 1.f);

	//Check for error
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		//printf("Error initializing OpenGL! %s\n", gluErrorString(error));
		success = false;
	}

	glewExperimental = GL_TRUE;
	// Initialize GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		//printf("Error: %s\n", glewGetErrorString(err));
	}

	//glEnable(GL_DEPTH_TEST);

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	return success;
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



