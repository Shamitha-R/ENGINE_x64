#include "EngineTexture.h"

EngineTexture::EngineTexture()
	:TextureHeight(0), TextureWidth(0)
{
	glGenTextures(1, &this->TextureID);
}

EngineTexture::~EngineTexture()
{
	
}

void EngineTexture::Generate(GLuint textureWidth, GLuint textureHeight, 
	char* textureData)
{
	this->TextureWidth = textureWidth;
	this->TextureHeight = textureHeight;

	glBindTexture(GL_TEXTURE_2D, this->TextureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, 
		GL_RGBA,GL_UNSIGNED_BYTE, textureData);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void EngineTexture::Bind()
{
	glBindTexture(GL_TEXTURE_2D, this->TextureID);
}

