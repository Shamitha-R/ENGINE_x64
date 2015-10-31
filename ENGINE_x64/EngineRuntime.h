#pragma once
#include <stdio.h>
#include <SDL.h>

class EngineRuntime
{
public:

	const int SCREENWIDTH = 1280;
	const int SCREENHEIGHT = 720;

	SDL_Window* EngineWindow = NULL;
	SDL_GLContext EngineContext;
	bool EnableRendering = true;

	EngineRuntime();
	~EngineRuntime();

	bool InitializeEngine();	
	void HandleInput(unsigned char targetKey, int xPos, int yPos);
	void Update();
	void Render();


private:
	bool InitializeOpenGL();

};

