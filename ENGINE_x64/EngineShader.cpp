#include "EngineShader.h"

void checkCompileErrors(GLuint object, std::string type)
{
	GLint success;
	GLchar infoLog[1024];
	if (type != "PROGRAM")
	{
		glGetShaderiv(object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(object, 1024, NULL, infoLog);
			std::cout << "| ERROR::SHADER: Compile-time error: Type: " << type << "\n"
				<< infoLog << "\n -- --------------------------------------------------- -- "
				<< std::endl;
		}
	}
	else
	{
		glGetProgramiv(object, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(object, 1024, NULL, infoLog);
			std::cout << "| ERROR::Shader: Link-time error: Type: " << type << "\n"
				<< infoLog << "\n -- --------------------------------------------------- -- "
				<< std::endl;
		}
	}
}

void EngineShader::InitializeShader(const GLchar* vertexSource, const GLchar* fragmentSource,
	const GLchar* geometrySource)
{
	GLuint vertexShader, fragmentShader, geometryShader;

	//Vertex Shader
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	checkCompileErrors(vertexShader, "VERTEX");
	//Fragment Shader
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	checkCompileErrors(fragmentShader, "FRAGMENT");
	//Geometry Shader
	if(geometrySource != nullptr)
	{
		geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometryShader, 1, &geometrySource, NULL);
		glCompileShader(geometryShader);
		checkCompileErrors(geometryShader, "GEOMETRY");
	}

	//Create The Shader Program
	this->ShaderID = glCreateProgram();
	glAttachShader(this->ShaderID, vertexShader);
	glAttachShader(this->ShaderID, fragmentShader);
	if(geometrySource != NULL)
		glAttachShader(this->ShaderID, geometryShader);

	glLinkProgram(this->ShaderID);
	checkCompileErrors(this->ShaderID, "PROGRAM");
	//Delete Temporary Shader content
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	if (geometrySource != NULL)
		glDeleteShader(geometryShader);
}

EngineShader& EngineShader::Enable()
{
	glUseProgram(this->ShaderID);
	return *this;
}

void EngineShader::SetFloat(const GLchar *name, GLfloat value, GLboolean useShader)
{
	if (useShader)
		this->Enable();
	glUniform1f(glGetUniformLocation(this->ShaderID, name), value);
}
void EngineShader::SetInteger(const GLchar *name, GLint value, GLboolean useShader)
{
	if (useShader)
		this->Enable();
	glUniform1i(glGetUniformLocation(this->ShaderID, name), value);
}
void EngineShader::SetVector2f(const GLchar *name, GLfloat x, GLfloat y, GLboolean useShader)
{
	if (useShader)
		this->Enable();
	glUniform2f(glGetUniformLocation(this->ShaderID, name), x, y);
}
void EngineShader::SetVector2f(const GLchar *name, const glm::vec2 &value, GLboolean useShader)
{
	if (useShader)
		this->Enable();
	glUniform2f(glGetUniformLocation(this->ShaderID, name), value.x, value.y);
}
void EngineShader::SetVector3f(const GLchar *name, GLfloat x, GLfloat y, GLfloat z, GLboolean useShader)
{
	if (useShader)
		this->Enable();
	glUniform3f(glGetUniformLocation(this->ShaderID, name), x, y, z);
}
void EngineShader::SetVector3f(const GLchar *name, const glm::vec3 &value, GLboolean useShader)
{
	if (useShader)
		this->Enable();
	glUniform3f(glGetUniformLocation(this->ShaderID, name), value.x, value.y, value.z);
}
void EngineShader::SetVector4f(const GLchar *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w, GLboolean useShader)
{
	if (useShader)
		this->Enable();
	glUniform4f(glGetUniformLocation(this->ShaderID, name), x, y, z, w);
}
void EngineShader::SetVector4f(const GLchar *name, const glm::vec4 &value, GLboolean useShader)
{
	if (useShader)
		this->Enable();
	glUniform4f(glGetUniformLocation(this->ShaderID, name), value.x, value.y, value.z, value.w);
}
void EngineShader::SetMatrix4(const GLchar *name, const glm::mat4 &matrix, GLboolean useShader)
{
	if (useShader)
		this->Enable();
	glUniformMatrix4fv(glGetUniformLocation(this->ShaderID, name), 1, 
		GL_FALSE, glm::value_ptr(matrix));
}







