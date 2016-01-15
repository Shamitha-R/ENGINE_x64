#pragma once
#include <glew.h>
#include "EngineRenderer.h"
#include "UI.h"
#include "EngineObject.h"


class Engine
{
	public:
		//GLuint SCREENWIDTH, SCREENHEIGHT;
		EngineRenderer Renderer;
		SDL_Event EngineEvent;
		bool TerminateEngine;
		UI EngineUI;
		EngineOCT OCT;

		EngineObject* TestObject;

		Engine();
		~Engine();

		void InitalizeEngine();
		void UpdateEngine(GLfloat engineTime);
		void RenderEngine();
};