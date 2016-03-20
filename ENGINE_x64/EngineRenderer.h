#pragma once
#include <stdio.h>
#include <SDL.h>

#include <SDL_image.h>
#include <AntTweakBar.h>

#include "EngineShader.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <vector>


class EngineRenderer
{
public:

	const int SCREENWIDTH = 800;
	const int SCREENHEIGHT = 600;

	SDL_Window* EngineWindow = NULL;
	SDL_GLContext EngineContext;
	bool EnableRendering = true;
	double ObjectOrientation[16];

	EngineRenderer();
	~EngineRenderer();

	bool InitializeEngine();	
	void HandleInput(unsigned char targetKey, int xPos, int yPos);
	void Update();
	void Render();
	void InitializeRenderData(std::vector<GLchar> &renderData,int lCount);

private:
	bool InitializeOpenGL();
};

