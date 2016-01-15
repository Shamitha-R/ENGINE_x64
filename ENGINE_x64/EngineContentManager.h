#include "EngineShader.h"
#include "EngineTexture.h"
#include <map>
#include <SDL.h>
#include <SDL_image.h>

class EngineContentManager
{
public:
	static std::map<std::string,EngineShader> EShaders;
	static std::map<std::string,EngineTexture> ETextures;

	static EngineShader LoadShader(const GLchar* vertexPath, const GLchar* fragmentPath,
		const GLchar* geometryPath, std::string shaderName);

	static EngineTexture LoadTexture(const GLchar* filePath, std::string textureName);

	static void FreeResources();

private:
	EngineContentManager(){ }
};