#include "Engine.h"
#include "EngineContentManager.h"
#include <algorithm>
#include <functional>
#include <numeric>

int TW_CALL TwEventSDL20(const void *sdlEvent)
{
	int handled = 0;
	const SDL_Event *event = (const SDL_Event *)sdlEvent;

	if (event == NULL)
		return 0;

	switch (event->type)
	{
	case SDL_KEYDOWN:
		if (event->key.keysym.sym & SDLK_SCANCODE_MASK) {
			int key = 0;
			switch (event->key.keysym.sym) {
			case SDLK_UP:
				key = TW_KEY_UP;
				break;
			case SDLK_DOWN:
				key = TW_KEY_DOWN;
				break;
			case SDLK_RIGHT:
				key = TW_KEY_RIGHT;
				break;
			case SDLK_LEFT:
				key = TW_KEY_LEFT;
				break;
			case SDLK_INSERT:
				key = TW_KEY_INSERT;
				break;
			case SDLK_HOME:
				key = TW_KEY_HOME;
				break;
			case SDLK_END:
				key = TW_KEY_END;
				break;
			case SDLK_PAGEUP:
				key = TW_KEY_PAGE_UP;
				break;
			case SDLK_PAGEDOWN:
				key = TW_KEY_PAGE_DOWN;
				break;
			default:
				if (event->key.keysym.sym >= SDLK_F1 &&
					event->key.keysym.sym <= SDLK_F12) {
					key = event->key.keysym.sym + TW_KEY_F1 - SDLK_F1;
				}
				break;
			}
			if (key != 0) {
				handled = TwKeyPressed(key, event->key.keysym.mod);
			}
		}
		else {
			handled = TwKeyPressed(event->key.keysym.sym /*& 0xFF*/,
				event->key.keysym.mod);
		}
		break;
	case SDL_MOUSEMOTION:
		handled = TwMouseMotion(event->motion.x, event->motion.y);
		break;
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		if (event->type == SDL_MOUSEBUTTONDOWN &&
			(event->button.button == 4 || event->button.button == 5)) {
			// mouse wheel
			static int s_WheelPos = 0;
			if (event->button.button == 4)
				++s_WheelPos;
			else
				--s_WheelPos;
			handled = TwMouseWheel(s_WheelPos);
		}
		else {
			handled = TwMouseButton(
				(event->type == SDL_MOUSEBUTTONUP) ?
				TW_MOUSE_RELEASED : TW_MOUSE_PRESSED,
				(TwMouseButtonID)event->button.button);
		}
		break;
	case SDL_WINDOWEVENT:
		if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
			// tell the new size to TweakBar
			TwWindowSize(event->window.data1, event->window.data2);
			// do not set 'handled'
			// SDL_VIDEORESIZE may be also processed by the calling application
		}
		break;
	}

	return handled;
}

Engine::Engine()
{
	
}

Engine::~Engine()
{
	TwTerminate();

	EngineContentManager::FreeResources();
}

std::vector<float> RemoveNoise(std::vector<float> &originalBScan,std::vector<float> &corrResult)
{
	std::vector<float> avgBScan = originalBScan;

	//Threshold BScan
	int topIgnoreSize = 15 * 500 * 3;
	int dataIgnoreSize = 199 * 500 * 3;

	std::vector<float> noiseDataBScan;

	for (int i = 0; i < 51;i++)
	{
		noiseDataBScan.insert(noiseDataBScan.end(),
			&avgBScan[((i*500*3) + topIgnoreSize + dataIgnoreSize)], 
			&avgBScan[((i*500*3) + topIgnoreSize + dataIgnoreSize) + (499*3)]);
	}

	double sumBScan = std::accumulate(noiseDataBScan.begin(), noiseDataBScan.end(), 0.0);
	double meanBScan = sumBScan / noiseDataBScan.size();

	std::vector<double> diff(noiseDataBScan.size());
	std::transform(noiseDataBScan.begin(), noiseDataBScan.end(), diff.begin(),
		std::bind2nd(std::minus<double>(), meanBScan));
	double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
	double stdevBScan = std::sqrt(sq_sum / noiseDataBScan.size());

	float noiseFloorBScan = meanBScan + 3 * stdevBScan;

	std::replace_if(avgBScan.begin(), avgBScan.end(),
		[noiseFloorBScan](float value)
	{
		if (value < noiseFloorBScan)
			return true;
		else
			return false;
	}, 0);

	//Threshold correlation Map
	int topIgnoreCorr = 5 * 500;

	std::vector<float> noiseDataCorr;

	for (int i = 0; i < 10; i++)
	{
		noiseDataCorr.insert(noiseDataCorr.end(),
			&corrResult[(i * 500) + topIgnoreCorr],
			&corrResult[((i * 500) + topIgnoreCorr) + (499)]);
	}


	double sumCorr = std::accumulate(noiseDataCorr.begin(), noiseDataCorr.end(), 0.0f);
	double meanCorr = sumCorr / noiseDataCorr.size();

	std::replace_if(corrResult.begin(),corrResult.end(),
	[meanCorr](float value)
	{
		if (value < meanCorr)
			return true;
		else
			return false;
	}
	, 0);

	//Now use the B-Scan to filter only the areas of the correlation map
	//that are contributed to by actual OCT signal rather than  noise.
	for (int i = 0; i < corrResult.size();i++)
	{
		if (avgBScan[i * 3] == 0)
			corrResult[i] = 0;
	}

	//The filtered cmOCT gives the location of the vasculature
	//Now extract the corresponding regions from the original OCT data
	std::vector<float> vasculature = originalBScan;

	for (int i = 0; i < corrResult.size(); i++)
	{
		if (corrResult[i] == 0)
			vasculature[i*3] = 0;
	}

	//TestBMP(vasculature);

	//*********************************** Compositing *******************************************
	std::vector<float> composite(500*512*3);

	for (int i = 0; i < (500*512);i++)
	{
		if (vasculature[i * 3] > 0) {
			composite[i * 3] = vasculature[i * 3];
			composite[(i * 3) + 1] = 0;
			composite[(i * 3) + 2] = 0;
		}else
		{
			composite[i * 3] = originalBScan[i * 3];
			composite[(i * 3) + 1] = originalBScan[(i * 3) + 1];
			composite[(i * 3) + 2] = originalBScan[(i * 3) + 2];
		}
	}

	//Remove top area
	//std::vector<float> result(composite.begin() + topIgnoreSize, composite.end());

	return originalBScan;
}

void Engine::InitalizeEngine()
{
	if (!this->Renderer.InitializeEngine())
		printf("Engine Initialization Failed\n");
	else {
		UI engineUI;

		engineUI.InitialiseUI(this->Renderer.SCREENWIDTH, this->Renderer.SCREENHEIGHT,OCT);

		EngineContentManager::LoadShader("./shaders/vertexshader.vs",
			"./shaders/fragmentshader.fs",nullptr,"sprite");

		glm::mat4 projection;
		glm::mat4 view;
		view = glm::translate(view, glm::vec3(0.0f, 1.0f, -7.0f));
		projection = glm::perspective(75.0f, (GLfloat)800 / (GLfloat)600, 0.1f, 100.0f);
		
		EngineContentManager::EShaders["sprite"].Enable().SetInteger("image", 0);
		EngineContentManager::EShaders["sprite"].SetMatrix4("view", view);
		EngineContentManager::EShaders["sprite"].SetMatrix4("projection", projection);

		//EngineContentManager::LoadTexture("null", "testTex");

		int imgCount = 2;

		//printf("Init\n");
		for (int corrImage = 0; corrImage < imgCount;corrImage++)
		{
			if(corrImage % 10 == 0)
				printf("Rendering Slices %0.2f %%\n",(corrImage*1.0f)/(imgCount*1.0f)*100.0f);

			std::string textureName = "tex" + std::to_string(corrImage);

			std::vector<GLchar> filteredTexData(this->OCT.CompositeResults.begin() + (500 * 512 * 4 * (corrImage)),
				this->OCT.CompositeResults.begin() + (500 * 512 * 4 * (corrImage + 1)));

			EngineContentManager::CreateTexture(filteredTexData, textureName, corrImage);

			EngineObject* testObject = new EngineObject(EngineContentManager::EShaders["sprite"],
				EngineContentManager::ETextures[textureName],
				glm::vec3(0,
					0,
					-0.50f+((corrImage*1.0f) / (imgCount*1.0f))*1.0f),
				glm::vec2(1, 1),
				0,
				glm::vec3(1.0f, 1.0f, 1.0f));
			EngineSlices.push_back(testObject);
		}

		RenderDepth = 0;
	}
}

void Engine::UpdateEngine(GLfloat engineTime)
{
	if (SDL_PollEvent(&(this->EngineEvent)) != 0)
	{
		int uiEvent = TwEventSDL20(&(this->EngineEvent));

		if (!uiEvent) {

			if (this->EngineEvent.type == SDL_QUIT)
			{
				this->TerminateEngine = true;
			}
			else if (this->EngineEvent.type == SDL_KEYDOWN &&
				this->EngineEvent.key.repeat == 0)
			{
				switch (this->EngineEvent.key.keysym.sym) {
				case SDLK_q:
					if(RenderDepth < EngineSlices.size()-1)
						RenderDepth += 1;
					break;
				case SDLK_w:
					if (RenderDepth > 0)
						RenderDepth -= 1;
					break;
				default:
					break;
				}
			}
		}
		int xpos, ypos;
		SDL_GetMouseState(&xpos, &ypos);
		float angle = (400.0f - xpos);
		angle = angle / 360.0f * M_PI * 1.0f;

		for (int i = 0; i < EngineSlices.size(); i++)
			EngineSlices[i]->ObjectRotation = angle;
	}
}

void Engine::RenderEngine()
{

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (int i = RenderDepth; i < EngineSlices.size(); i++)
		EngineSlices[i]->Render();

	TwDraw();

	SDL_GL_SwapWindow(this->Renderer.EngineWindow);
}

SDL_Window* EngineWindow2 = NULL;
SDL_GLContext EngineContext2;
SDL_Surface* gScreenSurface2 = NULL;
double mdRotation[16];
float mfRot[3];
int x, y;
bool InitOpenGL()
{
	printf("Initializing OpenGL\n");

	GLenum error = GL_NO_ERROR;

	glClearColor(0.f, 0.f, 0.f, 1.0f);

	glewExperimental = GL_TRUE;
	error = glewInit();
	if (GL_TRUE != glewGetExtension("GL_EXT_texture3D"))
	{
		
	}

	return true;
}

bool InitializeTest()
{
	printf("Initializing Engine\n");


	//The surface contained by the window


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
		EngineWindow2 = SDL_CreateWindow("ENGINE_WINx64",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			800, 600,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

		if (EngineWindow2 == NULL)
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
				gScreenSurface2 = SDL_GetWindowSurface(EngineWindow2);
			}

			//Create context
			EngineContext2 = SDL_GL_CreateContext(EngineWindow2);
			if (EngineContext2 == NULL)
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
				if (!InitOpenGL())
				{
					printf("Unable to initialize OpenGL!\n");
					success = false;
				}


			}
		}
	}

	return success;
}

// Macro to draw the quad.
// Performance can be achieved by making a call list.
// To make it simple i am not using that now :-)
SDL_Event EngineEvent2;
GLfloat dOrthoSize = 1.0f;
#define MAP_3DTEXT( TexIndex ) \
            glTexCoord3f(0.0f, 0.0f, ((float)TexIndex+1.0f)/2.0f);  \
        glVertex3f(-dOrthoSize,-dOrthoSize,TexIndex);\
        glTexCoord3f(1.0f, 0.0f, ((float)TexIndex+1.0f)/2.0f);  \
        glVertex3f(dOrthoSize,-dOrthoSize,TexIndex);\
        glTexCoord3f(1.0f, 1.0f, ((float)TexIndex+1.0f)/2.0f);  \
        glVertex3f(dOrthoSize,dOrthoSize,TexIndex);\
        glTexCoord3f(0.0f, 1.0f, ((float)TexIndex+1.0f)/2.0f);  \
        glVertex3f(-dOrthoSize,dOrthoSize,TexIndex);
glm::vec2 rotRef;
void Render()
{
	if (SDL_PollEvent(&(EngineEvent2)) != 0)
	{
		if (EngineEvent2.type == SDL_QUIT)
		{
		}
		//If a mouse button was pressed

			if (EngineEvent2.button.button == SDL_BUTTON_LEFT)
			{
				SDL_GetMouseState(&x, &y);

				mfRot[0] = rotRef.y - y;
				mfRot[1] = rotRef.x - x;
				mfRot[2] = 0;

				glMatrixMode(GL_MODELVIEW);
				glLoadMatrixd(mdRotation);
				glRotated(mfRot[0], 1.0f, 0, 0);
				glRotated(mfRot[1], 0, 1.0f, 0);
				glRotated(mfRot[2], 0, 0, 1.0f);

				glGetDoublev(GL_MODELVIEW_MATRIX, mdRotation);
				glLoadIdentity();

				rotRef.x = x;
				rotRef.y = y;
			}
		

	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.05f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	// Translate and make 0.5f as the center 
	// (texture co ordinate is from 0 to 1. so center of rotation has to be 0.5f)
	glTranslatef(0.5f, 0.5f, 0.5f);

	// A scaling applied to normalize the axis 
	// (Usually the number of slices will be less so if this is not - 
	// normalized then the z axis will look bulky)
	// Flipping of the y axis is done by giving a negative value in y axis.
	// This can be achieved either by changing the y co ordinates in -
	// texture mapping or by negative scaling of y axis
	glScaled((float)500 / (float)500,
		-1.0f*(float)500 / (float)(float)512,
		(float)500 / (float)499);

	glMultMatrixd(mdRotation);

	glTranslatef(-0.5f, -0.5f, -0.5f);

	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, 1);
	for (float fIndx = -1.0f; fIndx <= 1.0f; fIndx += 0.01f)
	{
		glBegin(GL_QUADS);
		MAP_3DTEXT(fIndx);
		glEnd();
	}

	SDL_GL_SwapWindow(EngineWindow2);
}

void InitData(std::vector<GLchar> &testData)
{
	int texID = 0;
	glGenTextures(1, (GLuint*)&texID);

	glBindTexture(GL_TEXTURE_3D, texID);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 500, 512, 499, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, testData.data());
	glBindTexture(GL_TEXTURE_3D, 0);

	mdRotation[0] = mdRotation[5] = mdRotation[10] = mdRotation[15] = 1.0;
	mdRotation[1] = mdRotation[2] = mdRotation[3] = mdRotation[4] = 0.0f;
	mdRotation[6] = mdRotation[7] = mdRotation[8] = mdRotation[9] = 0.0f;
	mdRotation[11] = mdRotation[12] = mdRotation[13] = mdRotation[14] = 0.0f;
}

int main(int argc, char *argv[])
{
	Engine engine;

	engine.OCT.LoadOCTData();
	engine.OCT.OpenCLCompute();

	//engine.InitalizeEngine();

	//engine.TerminateEngine = false;

	//while(!engine.TerminateEngine)
	//{
	//	engine.UpdateEngine(0);
	//	engine.RenderEngine();
	//}

	std::vector<GLchar> testData(engine.OCT.CompositeResults.begin(),
		engine.OCT.CompositeResults.begin()+((500*512*4))*499);

	InitializeTest();
	InitData(testData);

	engine.TerminateEngine = false;
	while (!engine.TerminateEngine)
	{
		Render();
	}

	return 0;
}

