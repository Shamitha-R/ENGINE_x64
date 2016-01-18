#include "Engine.h"
#include "EngineContentManager.h"

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
	if (!this->Renderer.InitializeEngine())
		printf("Engine Initialization Failed\n");
	else {
		UI engineUI;

		engineUI.InitialiseUI(this->Renderer.SCREENWIDTH, this->Renderer.SCREENHEIGHT,OCT);

		EngineContentManager::LoadShader("./shaders/vertexshader.vs",
			"./shaders/fragmentshader.fs",nullptr,"sprite");

		glm::mat4 projection;
		glm::mat4 view;
		view = glm::translate(view, glm::vec3(0.0f, 0.0f, -4.0f));
		projection = glm::perspective(75.0f, (GLfloat)800 / (GLfloat)600, 0.1f, 100.0f);
		
		EngineContentManager::EShaders["sprite"].Enable().SetInteger("image", 0);
		EngineContentManager::EShaders["sprite"].SetMatrix4("view", view);
		EngineContentManager::EShaders["sprite"].SetMatrix4("projection", projection);

		//EngineContentManager::LoadTexture("null", "testTex");

		int imgCount = 100;

		for (int corrImage = 0; corrImage < imgCount;corrImage++)
		{
			std::string textureName = "tex" + std::to_string(corrImage);
			EngineContentManager::CreateTexture(OCT.CorrelationResults, textureName, corrImage);

			EngineObject* testObject = new EngineObject(EngineContentManager::EShaders["sprite"],
				EngineContentManager::ETextures[textureName],
				glm::vec3(0.0, +((corrImage*1.0f) / (imgCount*1.0f))*0.5f,
					0.5f-((corrImage*1.0f)/(imgCount*1.0f)))*0.5f,
				glm::vec2(1, 1),
				45.0f,
				glm::vec3(1.0f, 1.0f, 1.0f));
			EngineSlices.push_back(testObject);

		}

		RenderDepth = 15;
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
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (int i = EngineSlices.size()-1; i >= RenderDepth; i--)
		EngineSlices[i]->Render();

	TwDraw();

	SDL_GL_SwapWindow(this->Renderer.EngineWindow);
}

int main(int argc, char *argv[])
{
	Engine engine;

	engine.OCT.LoadOCTData();
	engine.OCT.OpenCLCompute();

	engine.InitalizeEngine();

	engine.TerminateEngine = false;

	while(!engine.TerminateEngine)
	{
		engine.UpdateEngine(0);
		engine.RenderEngine();
	}

	return 0;
}

