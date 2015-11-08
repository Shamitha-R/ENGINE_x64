#include "EngineOCT.h"
#include "EngineRenderer.h" 
#include "UI.h"

void EngineOCT::LoadFileData(std::string fileName,std::vector<std::string> &data)
{
	std::string currentLine;
	std::ifstream fileStreamer(fileName);

	if(fileStreamer.is_open())
	{
		while (getline(fileStreamer, currentLine))
			data.push_back(currentLine);

		fileStreamer.close();
		std::cout << fileName + " successfully read\n";

	}else std::cout << "Unable to open file";
}

void EngineOCT::LoadOCTData()
{
	std::cout << "Loading OCT data\n";

	LoadFileData("resamplingTable.csv", ResamplingTableData);
	LoadFileData("referenceAScan.csv", ReferenceAScanData);
	LoadFileData("referenceSpectrum.csv", ReferenceSpectrumData);

	//Load Raw Data
	short int *hostSpectraData = new short int[BSCANCOUNT * OUTPUTASCANLENGTH * ASCANBSCANRATIO];

	FILE* inputSpecFile = fopen("Spectra.bin", "rb");

	short* inputSpectra = (short*)malloc(sizeof(short) * 25600000);

	for (int i = 0; i < 1; i++)
	{
		fseek(inputSpecFile, sizeof(short) * 25600000 * i, SEEK_SET);
		fread(inputSpectra, sizeof(short), 25600000, inputSpecFile);

	}	
	delete[] hostSpectraData;
}

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

void EngineRendering(EngineOCT &oct)
{
	EngineRenderer engine;

	if (!engine.InitializeEngine())
	{
		printf("Engine Initialization Failed\n");
	}
	else
	{
		UI engineUI;
		engineUI.InitialiseUI(engine.SCREENWIDTH, engine.SCREENHEIGHT,oct);

		// Build and compile our shader program
		Shader ourShader("shaders/vertexshader.vs", "shaders/fragmentshader.fs");

		// Set up vertex data (and buffer(s)) and attribute pointers
		GLfloat vertices[] = {
			// Positions          // Colors           // Texture Coords
			0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Right
			0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Bottom Right
			-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // Bottom Left
			-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // Top Left 
		};
		GLuint indices[] = {  // Note that we start from 0!
			0, 1, 3, // First Triangle
			1, 2, 3  // Second Triangle
		};
		GLuint VBO, VAO, EBO;
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		// Position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		// Color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
		// TexCoord attribute
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0); // Unbind VAO


							  // Load and create a texture 
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture); // All upcoming GL_TEXTURE_2D operations now have effect on this texture object
											   // Set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT (usually basic wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// Set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Load image, create texture and generate mipmaps
		SDL_Surface* image = engine.LoadSurface("textures/wall.png");
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
		glGenerateMipmap(GL_TEXTURE_2D);

		SDL_FreeSurface(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.


		SDL_Event e;
		SDL_StartTextInput();
		int test;
		float val = 0.0f;
		bool quit = false;

		while (!quit)
		{

			while (SDL_PollEvent(&e) != 0)
			{
				//test = TwEventSDL20(&e, SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
				test = TwEventSDL20(&e);

				if (!test) {

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
			}

			// Render
			// Clear the color buffer
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);


			// Bind Textures using texture units
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
			glUniform1i(glGetUniformLocation(ourShader.Program, "ourTexture1"), 0);

			// Activate shader
			ourShader.Use();

			// Create transformations
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 projection;

			val += 0.01f;

			model = glm::rotate(model, val, glm::vec3(0.0f, 1.0f, 0.0f));
			view = glm::translate(view, glm::vec3(0.0f, 0.0f, -2.0f));
			projection = glm::perspective(70.0f, (GLfloat)800 / (GLfloat)600, 0.1f, 100.0f);
			// Get their uniform location
			GLint modelLoc = glGetUniformLocation(ourShader.Program, "model");
			GLint viewLoc = glGetUniformLocation(ourShader.Program, "view");
			GLint projLoc = glGetUniformLocation(ourShader.Program, "projection");
			// Pass them to the shaders
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
			// Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

			// Draw container
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);

			TwDraw();

			SDL_GL_SwapWindow(engine.EngineWindow);

			//engine.Render();
		}


		SDL_StopTextInput();

		// Properly de-allocate all resources once they've outlived their purpose
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}


	// Terminate AntTweakBar
	TwTerminate();

	engine.~EngineRenderer();
}

int main(int argc, char *argv[])
{
	EngineOCT engineOCT;

	EngineRendering(engineOCT);

	return 0;
}

