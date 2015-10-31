#include "EngineRuntime.h"
#include "Engine.h"


EngineRuntime::EngineRuntime()
{
}

EngineRuntime::~EngineRuntime()
{
	//Destroy window	
	SDL_DestroyWindow(EngineWindow);
	EngineWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();

	printf("Engine Terminated\n");
}

bool EngineRuntime::InitializeEngine()
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
		//Use OpenGL 2.1
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
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
			//Create context
			EngineContext = SDL_GL_CreateContext(EngineWindow);
			if (EngineContext == NULL)
			{
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Use Vsync
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

bool EngineRuntime::InitializeOpenGL()
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

	// Initialize GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		//printf("Error: %s\n", glewGetErrorString(err));
	}

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	return success;
}

void EngineRuntime::HandleInput(unsigned char targetKey, int xPos, int yPos)
{
	//Toggle Rendering
	if (targetKey == 'q')
	{
		EnableRendering = !EnableRendering;
	}
}

void EngineRuntime::Update()
{
	
}

void EngineRuntime::Render()
{
	//Clear color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//if (EnableRendering)
	//{
	//	glBegin(GL_QUADS);
	//	glVertex2f(-0.5f, -0.25f);
	//	glVertex2f(0.5f, -0.5f);
	//	glVertex2f(0.5f, 0.5f);
	//	glVertex2f(-0.5f, 0.5f);
	//	glEnd();
	//}
}

int RenderOpenGL()
{

	EngineRuntime engine;

	if (!engine.InitializeEngine())
	{
		printf("Engine Initialization Failed\n");
	}
	else
	{
		EngineVertex vertices[] = {
			EngineVertex(glm::vec3(-0.5, -0.5, 0), glm::vec2(0.0, 1.0)),
			EngineVertex(glm::vec3(-0.5, 0.5, 0), glm::vec2(0.0,0.0)),
			EngineVertex(glm::vec3(0.5, 0.5, 0), glm::vec2(1.0, 0.0)),
			EngineVertex(glm::vec3(0.5, -0.5, 0), glm::vec2(1.0,1.0))
		};

		unsigned int indicesArray[] = { 0, 1, 2 };

		//EngineObject eObject(vertices, sizeof(vertices) / sizeof(vertices[0]),
		//	indicesArray,sizeof(indicesArray)/sizeof(indicesArray[0]));
		EngineObject eObject("./meshes/test.obj");

		EngineTexture eTexture("./textures/test.png");
		EngineShader eshader("./shaders/basicshader");
		EngineTransform transform;
		EngineCamera camera(glm::vec3(0, 0, -100), 70.0f, 
			(float)engine.SCREENWIDTH/(float)engine.SCREENHEIGHT,
			0.01f,1000.0f);

		float angle = 0.0f;

		printf("Engine Running...\n");

		bool quit = false;

		SDL_Event e;
		SDL_StartTextInput();

		while (!quit)
		{
			angle += 0.01f;
			transform.GetRotation().y = angle;
			transform.GetRotation().z = angle;
			transform.SetScale(glm::vec3(1, 0.5f, 1));

			while (SDL_PollEvent(&e) != 0)
			{	
				if (e.type == SDL_QUIT)
				{
					quit = true;
				}
				else if (e.type == SDL_TEXTINPUT)
				{
					int x = 0, y = 0;
					SDL_GetMouseState(&x, &y);
					engine.HandleInput(e.text.text[0], x, y);
				}
			}

			eshader.BindShader();
			eshader.Update(transform,camera);

			eTexture.Bind(0);
			eObject.RenderObject();

			SDL_GL_SwapWindow(engine.EngineWindow);
			engine.Render();
		}

		
		SDL_StopTextInput();
	}

	engine.~EngineRuntime();

	return 0;
}

