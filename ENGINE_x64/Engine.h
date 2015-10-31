#include <glm.hpp>
#include <gtx/transform.hpp>
#include <glew.h>
#include <SDL_opengl.h>
#include <GL\GLU.h>
#include <string>
#include "stb_image.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>
#include "obj_loader.h"

#pragma once

class EngineCamera;
class EngineTransform;
struct EngineVertex;

class Engine
{
public:
	Engine();
	~Engine();
};

class EngineObject
{
public:
	EngineObject(EngineVertex* vertexList, unsigned int vertexCount,
		unsigned int* indices, unsigned int numIndices);
	EngineObject(const std::string& fileName);

	virtual ~EngineObject();

	void RenderObject();

private:
	void operator=(const EngineObject& other)
	{
	}

	EngineObject(const EngineObject& other)
	{
	}

	void InitMesh(const IndexedModel* model);

	enum
	{
		POSITION_VB,
		TEXCOORD_VB,
		INDEX_VB,
		NUM_BUFFERS
	};

	GLuint m_vertexArrayObject;
	GLuint m_vertexArrayBuffers[NUM_BUFFERS];
	unsigned int m_drawCount;
};

struct EngineVertex
{
public:
	EngineVertex(const glm::vec3& vertexPosition,const glm::vec2& texCoord)
	{
		this->vertexPosition = vertexPosition;
		this->texCoord = texCoord;
	}

	inline glm::vec3* GetPos() { return &vertexPosition; }
	inline glm::vec2* GetCoordinate() { return &texCoord; }

protected:
private:

	glm::vec3 vertexPosition;
	glm::vec2 texCoord;
};

struct EngineTexture
{
public:
	EngineTexture(const std::string& fileName);

	void Bind(unsigned int unit);

	virtual ~EngineTexture();
protected:
private:
	EngineTexture(const EngineTexture& other)
	{
	}

	void operator=(const EngineTexture& other)
	{
	}

	GLuint m_Texture;
};

class EngineShader
{
public:
	EngineShader(const std::string& fileName);
	virtual ~EngineShader();
	
	void BindShader();

	void Update(const EngineTransform& transform,const EngineCamera& camera);

	std::string LoadShader(const std::string& fileName);
	void CheckShaderError(GLuint shader, GLuint flag, bool isProgram, const std::string& errorMessage);
	GLuint CreateShader(const std::string& text, GLenum shaderType);

private:
	static const unsigned int NUM_SHADERS = 2;
	EngineShader(const EngineShader& other){}
	void operator=(const EngineShader& other){}

	enum
	{
		TRANSFORM_U,
		NUM_UNIFORMS
	};

	GLuint m_Program;
	GLuint m_Shaders[NUM_SHADERS];
	GLuint m_uniforms[NUM_UNIFORMS];
};

class EngineTransform
{
public:
	EngineTransform(const glm::vec3& position = glm::vec3(),
		const glm::vec3& rotation = glm::vec3(), 
		const glm::vec3 scale = glm::vec3(1.0f,1.0f,1.0f)):
			m_position(position),
			m_rotation(rotation),
			m_scale(scale)
	{}

	inline glm::mat4 GetModel() const
	{
		glm::mat4 positionMatrix = glm::translate(m_position);
		glm::mat4 scaleMatrix = glm::scale(m_scale);

		glm::mat4 rotationXMatrix = glm::rotate(m_rotation.x, glm::vec3(1, 0, 0));
		glm::mat4 rotationYMatrix = glm::rotate(m_rotation.y, glm::vec3(0, 1, 0));
		glm::mat4 rotationZMatrix = glm::rotate(m_rotation.z, glm::vec3(0, 0, 1));

		glm::mat4 rotationMatrix = rotationZMatrix * rotationYMatrix * rotationXMatrix;

		return positionMatrix * rotationMatrix * scaleMatrix;
	}

	inline glm::vec3& GetPosition() { return m_position; }
	inline glm::vec3& GetRotation() { return m_rotation; }
	inline glm::vec3& GetScale() { return m_scale; }

	inline void SetPosition(const glm::vec3 position) { m_position = position; }
	inline void SetRotation(const glm::vec3 rotation) { m_rotation = rotation; }
	inline void SetScale(const glm::vec3 scale) { m_scale = scale; }

private:
	glm::vec3 m_position;
	glm::vec3 m_rotation;
	glm::vec3 m_scale;
};

class EngineCamera
{
public:
	EngineCamera(glm::vec3& pos, float fov, float aspect, float zNear, float zFar)
	{
		m_Perspective = glm::perspective(fov, aspect, zNear, zFar);
		m_Position = pos;
		m_Forward = glm::vec3(0, 0, 1);
		m_Up = glm::vec3(0, 1, 0);
	}

	inline glm::mat4 GetViewProjection() const
	{
		return m_Perspective *
			glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
	}

private:
	glm::mat4 m_Perspective;
	glm::vec3 m_Position;
	glm::vec3 m_Forward;
	glm::vec3 m_Up;
};
 