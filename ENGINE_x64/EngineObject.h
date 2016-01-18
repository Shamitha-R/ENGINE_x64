#include <glew.h>

#include "EngineShader.h"
#include "EngineTexture.h"


class EngineObject
{
public:
	EngineShader ObjectShader;
	EngineTexture ObjectTexture;
	glm::vec3 ObjectPosition;
	glm::vec2 ObjectSize;
	glm::vec3 ObjectColour;
	GLfloat ObjectRotation;

	EngineObject(EngineShader&  targetShader,EngineTexture& targetTexture,
		glm::vec3 position, glm::vec2 size = glm::vec2(10, 10), 
		GLfloat rotate = 0.0f, glm::vec3 colour = glm::vec3(1.0f));
	~EngineObject();

	void Render();

private:
	GLuint quadVAO;
	
	void SetupObjectGLData();
};