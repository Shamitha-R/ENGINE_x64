#pragma once
#include <glew.h>
#include "EngineRenderer.h"
#include "UI.h"
#include "EngineObject.h"
#include "EngineOCT.h"


class Engine
{
	public:
		//GLuint SCREENWIDTH, SCREENHEIGHT;
		EngineRenderer Renderer;
		SDL_Event EngineEvent;
		bool TerminateEngine;
		//UI EngineUI;
		EngineOCT OCT;
		EngineRenderer EngineRenderer;

		std::vector<EngineObject*> EngineSlices;
		int RenderDepth;
		int RenderLayerCount = 400;
		
		Engine();
		~Engine();

		void InitalizeEngine();
		void UpdateEngine(GLfloat engineTime);
		void RenderEngine();
		void PassRenderData();
};