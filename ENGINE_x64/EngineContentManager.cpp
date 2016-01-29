#include "EngineContentManager.h"

std::map<std::string, EngineShader> EngineContentManager::EShaders;
std::map<std::string, EngineTexture> EngineContentManager::ETextures;

EngineShader EngineContentManager::LoadShader(const GLchar* vertexPath, const GLchar* fragmentPath,
	const GLchar* geometryPath, std::string shaderName)
{
	//EShaders[shaderName] =#
	std::string vertexRawData;
	std::string fragmentRawData;
	std::string geometryRawData;

	try
	{
		//Load Files
		std::ifstream vertexShaderFile(vertexPath);
		std::ifstream fragmentShaderFile(fragmentPath);
		std::stringstream vertexStream, fragmentStream;

		vertexStream << vertexShaderFile.rdbuf();
		fragmentStream << fragmentShaderFile.rdbuf();

		vertexShaderFile.close();
		fragmentShaderFile.close();

		vertexRawData = vertexStream.str();
		fragmentRawData = fragmentStream.str();

		if(geometryPath != nullptr)
		{
			std::ifstream geometryShaderFile(geometryPath);
			std::stringstream geometryStream;

			geometryStream << geometryShaderFile.rdbuf();
			geometryShaderFile.close();
			geometryRawData = geometryStream.str();
		}
	}catch (std::exception exception)
	{
		std::cout << "ERROR::SHADER: Failed to read shader files" << std::endl;
	}

	const GLchar* vertexShaderData = vertexRawData.c_str();
	const GLchar* fragmentShaderData = fragmentRawData.c_str();
	const GLchar* geometryShaderData = geometryRawData.c_str();

	EngineShader tempShader;
	tempShader.InitializeShader(vertexShaderData, fragmentShaderData,
		geometryPath != nullptr ? geometryShaderData : nullptr);

	EShaders[shaderName] = tempShader;
	return tempShader;
}

SDL_Surface* LoadSurface(std::string path)
{
	//The final optimized image
	SDL_Surface* optimizedSurface = NULL;

	SDL_PixelFormat pf;
	pf.palette = 0;
	pf.BitsPerPixel = 32;
	pf.BytesPerPixel = 4;
	//pf.alpha = 255;
	pf.Rshift = pf.Rloss = pf.Gloss = pf.Bloss = pf.Aloss = 0;
	pf.Rmask = 0x000000ff;
	pf.Gshift = 8;
	pf.Gmask = 0x0000ff00;
	pf.Bshift = 16;
	pf.Bmask = 0x00ff0000;
	pf.Ashift = 24;
	pf.Amask = 0xff000000;
	pf.refcount = 1;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Convert surface to screen format
		optimizedSurface = SDL_ConvertSurface(loadedSurface, &pf, SDL_SWSURFACE);
		if (optimizedSurface == NULL)
		{
			printf("Unable to optimize image %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	return optimizedSurface;
}

EngineTexture EngineContentManager::LoadTexture(const GLchar* filePath, 
	std::string textureName)
{
	EngineTexture tempTexture;

	SDL_Surface* image = LoadSurface("textures/wall.png");
	tempTexture.Generate(image->w, image->h, (char*)image->pixels);
	SDL_FreeSurface(image);

	ETextures[textureName] = tempTexture;
	return tempTexture;
}

EngineTexture EngineContentManager::CreateTexture(std::vector<float> textureData, std::string textureName,
	int depthLayer)
{
	EngineTexture tempTexture;

	//////TODO Change to Heap
	GLchar textureData2[500 * (512) * 4];
	std::fill_n(textureData2, 500 * 512 * 4, 255);

	for (int i = 0; i < (500*512);i++)
	{
		textureData2[(i * 4)] = textureData[(i * 3)];
		textureData2[(i * 4)+1] = textureData[(i * 3)+1];
		textureData2[(i * 4)+2] = textureData[(i * 3)+2];
		//textureData2[(i * 4) + 3] = textureData[(i * 4) + 3];

		if(textureData[(i * 3)] <= 50 || ((i) < (5*500)))
			textureData2[(i * 4) + 3] = 0;
		else
			textureData2[(i * 4)+3] = textureData[(i * 3)];
	}

	tempTexture.Generate(500, 512, textureData2);

	ETextures[textureName] = tempTexture;
	return tempTexture;
}

void EngineContentManager::FreeResources()
{
	
}
