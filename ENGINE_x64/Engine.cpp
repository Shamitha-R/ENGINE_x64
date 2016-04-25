#include "Engine.h"
#include "EngineContentManager.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <cstdio>

#include <gtc/matrix_transform.hpp>
#include <gtx/transform.hpp>

#include <Commdlg.h>
#include <chrono>

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

void Engine::InitalizeEngine()
{

}

void Engine::UpdateEngine(GLfloat engineTime)
{

}

float mfRot[3];
int x, y;
glm::vec2 rotRef;
GLfloat dOrthoSize = 1.0f;
float renderDensity = 0.01f;
void RenderTexture(double mdRotation[16], int lCount, EngineOCT &oct)
{
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	// Translate and make 0.5f as the center 
	glTranslatef(0.5f, 0.5f, 0.5f);

	// A scaling applied to normalize the axis 
	glScaled(((float)oct.NumAScans / (float)oct.NumAScans)*2.0f,
		(-1.0f*(float)oct.NumAScans / (float)(float)oct.OutputImageHeight)*2.0f,
		((float)oct.OutputImageHeight / (float)lCount)*2.0f);

	glMultMatrixd(mdRotation);

	glTranslatef(-0.5f, -0.5f, -0.5f);
	//Texture Mapping
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, 1);
	for (float TexIndex = -1.0f; TexIndex <= 1.0f; TexIndex += renderDensity)
	{
		glBegin(GL_QUADS);
		//Map texels to each Quad
		glTexCoord3f(0.0f, 0.0f, ((float)TexIndex + 1.0f) / 2.0f);
		glVertex3f(-dOrthoSize, -dOrthoSize, TexIndex);
		glTexCoord3f(1.0f, 0.0f, ((float)TexIndex + 1.0f) / 2.0f);
		glVertex3f(dOrthoSize, -dOrthoSize, TexIndex);
		glTexCoord3f(1.0f, 1.0f, ((float)TexIndex + 1.0f) / 2.0f);
		glVertex3f(dOrthoSize, dOrthoSize, TexIndex);
		glTexCoord3f(0.0f, 1.0f, ((float)TexIndex + 1.0f) / 2.0f);
		glVertex3f(-dOrthoSize, dOrthoSize, TexIndex);
		glEnd();
	}
}
bool enableRendering = false;

void Engine::RenderEngine()
{
	SDL_GetMouseState(&x, &y);
	if (SDL_PollEvent(&(EngineEvent)) != 0)
	{
		int uiEvent = TwEventSDL20(&(EngineEvent));
		if (!uiEvent) {
			if (EngineEvent.type == SDL_QUIT)
			{
			}
			//If a mouse button was pressed

			if (EngineEvent.button.button == SDL_BUTTON_LEFT)
			{
				mfRot[0] = rotRef.y - y;
				mfRot[1] = rotRef.x - x;
				mfRot[2] = 0;

				glMatrixMode(GL_MODELVIEW);
				glLoadMatrixd(EngineRenderer.ObjectOrientation);
				glRotated(mfRot[0], 1.0f, 0, 0);
				glRotated(mfRot[1], 0, 1.0f, 0);
				glRotated(mfRot[2], 0, 0, 1.0f);

				glGetDoublev(GL_MODELVIEW_MATRIX, EngineRenderer.ObjectOrientation);
				glLoadIdentity();

			}
		}
	}
	rotRef.x = x;
	rotRef.y = y;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (enableRendering)
		RenderTexture(EngineRenderer.ObjectOrientation, RenderLayerCount, this->OCT);

	TwDraw();

	SDL_GL_SwapWindow(this->EngineRenderer.EngineWindow);
}

int main(int argc, char *argv[])
{
	Engine engine;

	engine.EngineRenderer.InitializeEngine();

	UI engineUI;
	engineUI.InitialiseUI(1280, 720, engine);

	//Main Render Loop
	engine.TerminateEngine = false;
	while (!engine.TerminateEngine)
	{
		engine.RenderEngine();
	}

	return 0;
}

void Engine::PassRenderData()
{
	std::vector<GLchar> testData(OCT.CompositeResults.begin(),
		OCT.CompositeResults.begin() + ((500 * 512 * 4)) * RenderLayerCount);

	EngineRenderer.InitializeRenderData(testData,RenderLayerCount);
	enableRendering = true;

}

