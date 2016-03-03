#include "Engine.h"
#include "EngineContentManager.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <cstdio>

#include <gtc/matrix_transform.hpp>
#include <gtx/transform.hpp>

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

}

void Engine::UpdateEngine(GLfloat engineTime)
{
	if (SDL_PollEvent(&(this->EngineEvent)) != 0)
	{
		//int uiEvent = TwEventSDL20(&(this->EngineEvent));

		//if (!uiEvent) {

			//if (this->EngineEvent.type == SDL_QUIT)
			//{
				//this->TerminateEngine = true;
			//}
			//else if (this->EngineEvent.type == SDL_KEYDOWN &&
			//	this->EngineEvent.key.repeat == 0)
			//{
			//	switch (this->EngineEvent.key.keysym.sym) {
			//	case SDLK_q:
			//		if(RenderDepth < EngineSlices.size()-1)
			//			RenderDepth += 1;
			//		break;
			//	case SDLK_w:
			//		if (RenderDepth > 0)
			//			RenderDepth -= 1;
			//		break;
			//	default:
			//		break;
			//	}
			//}
		//}
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

float mfRot[3];
int x, y;

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
void Render(SDL_Window* EngineWindow,double mdRotation[16])
{
	SDL_GetMouseState(&x, &y);
	if (SDL_PollEvent(&(EngineEvent2)) != 0)
	{
		int uiEvent = TwEventSDL20(&(EngineEvent2));
		if (!uiEvent) {
			if (EngineEvent2.type == SDL_QUIT)
			{
			}
			//If a mouse button was pressed

			if (EngineEvent2.button.button == SDL_BUTTON_LEFT)
			{
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

			}
		}
	}
	rotRef.x = x;
	rotRef.y = y;

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


	//glScaled(2.0f, 2.0f, 2.0f);

	// A scaling applied to normalize the axis 
	// (Usually the number of slices will be less so if this is not - 
	// normalized then the z axis will look bulky)
	// Flipping of the y axis is done by giving a negative value in y axis.
	// This can be achieved either by changing the y co ordinates in -
	// texture mapping or by negative scaling of y axis
	glScaled(((float)500 / (float)500)*2.0f,
		(-1.0f*(float)500 / (float)(float)512)*2.0f,
			((float)500 / (float)400)*2.0f);

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

	TwDraw();

	SDL_GL_SwapWindow(EngineWindow);
}



int main(int argc, char *argv[])
{
	Engine engine;

	engine.OCT.LoadOCTData();
	engine.OCT.OpenCLCompute();

	engine.EngineRenderer.InitializeEngine();

	UI engineUI;
	engineUI.InitialiseUI(1280, 720, engine);

	engine.PassRenderData();

	engine.TerminateEngine = false;
	while (!engine.TerminateEngine)
	{
		//engine.UpdateEngine(0);
		Render(engine.EngineRenderer.EngineWindow, engine.EngineRenderer.ObjectOrientation);
	}

	return 0;
}

void Engine::PassRenderData()
{
	std::vector<GLchar> testData(OCT.CompositeResults.begin(),
		OCT.CompositeResults.begin() + ((500 * 512 * 4)) * 400);

	EngineRenderer.InitializeRenderData(testData);
}

