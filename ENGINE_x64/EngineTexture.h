#ifndef ENGINETEXTURE_HEADER
#define ENGINETEXTURE_HEADER

#include <glew.h>
#include <SDL.h>
#include <SDL_image.h>

class EngineTexture
{
public:
	GLuint TextureID;
	GLuint TextureHeight, TextureWidth;

	EngineTexture();
	~EngineTexture();

	void Generate(GLuint textureWidth, GLuint textureHeight, char* textureData);

	void Bind();
};

#endif