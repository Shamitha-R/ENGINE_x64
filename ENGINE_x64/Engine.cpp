#include "Engine.h"

#pragma region Engine Texture

EngineTexture::EngineTexture(const std::string& fileName)
	{
		int width, height, numComponents;
		unsigned char* imageData = 
			stbi_load(fileName.c_str(), &width, &height, &numComponents,4);

		if (imageData == NULL)
		{
			
		}

		glGenTextures(1, &m_Texture);
		glBindTexture(GL_TEXTURE_2D, m_Texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, imageData);

		stbi_image_free(imageData);
	}

	EngineTexture::~EngineTexture()
	{
		glDeleteTextures(1, &m_Texture);
	}

	void EngineTexture::Bind(unsigned int unit)
	{
		assert(unit >= 0 && unit <= 31);

		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, m_Texture);
	}

#pragma endregion 

#pragma region Engine Object

	EngineObject::EngineObject(EngineVertex* vertexList, unsigned int vertexCount,
		unsigned int* indices, unsigned int numIndices)
	{
		IndexedModel model;

		for (int i = 0; i < vertexCount; i++)
		{
			model.positions.push_back(*vertexList[i].GetPos());
			model.texCoords.push_back(*vertexList[i].GetCoordinate());
		}

		for (int i = 0; i < numIndices; i++)
			model.indices.push_back(indices[i]);

		InitMesh(&model);
	}

	EngineObject::EngineObject(const std::string& fileName)
	{
		IndexedModel model = OBJModel(fileName).ToIndexedModel();
		InitMesh(&model);
	}

	EngineObject::~EngineObject()
	{
		glDeleteBuffers(NUM_BUFFERS, m_vertexArrayBuffers);
		glDeleteVertexArrays(1, &m_vertexArrayObject);
	}

	void EngineObject::RenderObject()
	{
		glBindVertexArray(m_vertexArrayObject);

		glDrawElements(GL_TRIANGLES, m_drawCount, GL_UNSIGNED_INT, 0);
		//glDrawArrays(GL_TRIANGLES, 0, m_drawCount);

		glBindVertexArray(0);
	}

	void EngineObject::InitMesh(const IndexedModel* model)
	{
		m_drawCount = model->indices.size();

		glGenVertexArrays(1, &m_vertexArrayObject);
		glBindVertexArray(m_vertexArrayObject);

		glGenBuffers(NUM_BUFFERS, m_vertexArrayBuffers);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[POSITION_VB]);
		glBufferData(GL_ARRAY_BUFFER, model->positions.size() * sizeof(model->positions[0]),
			&model->positions[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[TEXCOORD_VB]);
		glBufferData(GL_ARRAY_BUFFER, model->positions.size() * sizeof(model->texCoords[0]),
			&model->texCoords[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vertexArrayBuffers[INDEX_VB]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->indices.size() * sizeof(model->indices[0]),
			&model->indices[0], GL_STATIC_DRAW);

		glBindVertexArray(0);
	}

#pragma endregion 

#pragma region Engine Shader

	EngineShader::EngineShader(const std::string& fileName)
	{
		m_Program = glCreateProgram();
		m_Shaders[0] = CreateShader(LoadShader(fileName + ".vs"), GL_VERTEX_SHADER);
		m_Shaders[1] = CreateShader(LoadShader(fileName + ".fs"), GL_FRAGMENT_SHADER);

		for (unsigned int i = 0; i < NUM_SHADERS; i++)
			glAttachShader(m_Program, m_Shaders[i]);

		glBindAttribLocation(m_Program, 0, "position");
		glBindAttribLocation(m_Program, 1, "texCoord");
		
		glLinkProgram(m_Program);
		CheckShaderError(m_Program, GL_LINK_STATUS, true, "Error in linking shaders");

		glValidateProgram(m_Program);
		CheckShaderError(m_Program, GL_VALIDATE_STATUS, true, "Error in validate shaders");
	
		m_uniforms[TRANSFORM_U] = glGetUniformLocation(m_Program, "transform");
			
	}

	EngineShader::~EngineShader()
	{
		for (unsigned int i = 0; i < NUM_SHADERS; i++)
		{
			glDetachShader(m_Program, m_Shaders[i]);
			glDeleteShader(m_Shaders[i]);
		}

		glDeleteProgram(m_Program);
	}

	std::string EngineShader::LoadShader(const std::string& fileName)
	{
		std::ifstream file;
		file.open((fileName).c_str());

		std::string output;
		std::string line;

		if (file.is_open())
		{
			while (file.good())
			{
				getline(file, line);
				output.append(line + "\n");
			}
		}
		else
		{
			std::cerr << "Unable to load shader: " << fileName << std::endl;
		}

		return output;
	}

	void EngineShader::CheckShaderError(GLuint shader, GLuint flag, bool isProgram, const std::string& errorMessage)
	{
		GLint success = 0;
		GLchar error[1024] = { 0 };

		if (isProgram)
			glGetProgramiv(shader, flag, &success);
		else
			glGetShaderiv(shader, flag, &success);

		if (success == GL_FALSE)
		{
			if (isProgram)
				glGetProgramInfoLog(shader, sizeof(error), NULL, error);
			else
				glGetShaderInfoLog(shader, sizeof(error), NULL, error);

			std::cerr << errorMessage << ": '" << error << "'" << std::endl;
		}
	}

	GLuint EngineShader::CreateShader(const std::string& text, GLenum shaderType)
	{
		GLuint shader = glCreateShader(shaderType);

		if (shader == 0)
			std::cerr << "Error in creating shaders" << std::endl;

		const GLchar* shaderSourceStrings[1];
		GLint shaderSourceStringLengths[1];
		shaderSourceStrings[0] = text.c_str();
		shaderSourceStringLengths[0] = text.length();

		glShaderSource(shader, 1, shaderSourceStrings,
			shaderSourceStringLengths);
		glCompileShader(shader);

		CheckShaderError(shader, GL_COMPILE_STATUS, false, 
			"Error in compiling shaders");

		return shader;
	}

	void EngineShader::BindShader()
	{
		glUseProgram(m_Program);
	}

	void EngineShader::Update(const EngineTransform& transform,const EngineCamera& camera)
	{
		glm::mat4 model = camera.GetViewProjection() *
			transform.GetModel();

		glUniformMatrix4fv(m_uniforms[TRANSFORM_U], 1, GL_FALSE, &model[0][0]);
	}


#pragma endregion

Engine::Engine()
{

}

Engine::~Engine()
{
	
}


