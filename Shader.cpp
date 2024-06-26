#include "Shader.h"

Shader::Shader(const char * vertexPath, const char * fragmentPath, const char * geometryPath, const std::vector<std::string>& defines)
{
	static const std::string version = "#version 430 core\n";

	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	std::string defaultDefinesString;
	defaultDefinesString += "#define MAX_NUM_POINT_LIGHTS " + std::to_string(maxPointLights) + "\n";
	defaultDefinesString += "#define MAX_NUM_SPOT_LIGHTS " + std::to_string(maxSpotLights) + "\n";
	defaultDefinesString += "#define MAX_NUM_DIR_LIGHTS " + std::to_string(maxDirLights) + "\n";
	defaultDefinesString += "#define PI 3.14159265359\n";

	auto vertexSource = get_file_contents(vertexPath);
	auto fragmentSource = get_file_contents(fragmentPath);

	auto vShaderCode = vertexSource.c_str();
	auto fShaderCode = fragmentSource.c_str();

	std::string definesString;
	for (const std::string& define : defines)
	{
		definesString += "#define " + define + "\n";
		std::cout << define << '\n';
	}

	const char* vShaderSources[4] = { version.c_str(), definesString.c_str(), defaultDefinesString.c_str(),  vShaderCode};
	const char* fShaderSources[4] = { version.c_str(), definesString.c_str(), defaultDefinesString.c_str(), fShaderCode};

	glShaderSource(vertexShader, 4, vShaderSources, NULL);
	glShaderSource(fragmentShader, 4, fShaderSources, NULL);

	glCompileShader(vertexShader);

	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, sizeof(infoLog), NULL, infoLog);
		std::cout << "Error compiling vertex shader '" << vertexPath << "'\n" << infoLog << std::endl;
		//std::cout << "Vertex shader source:\n" << version + defaultDefinesString + definesString + vShaderCode;
		// TODO find a solution for this. It's affecting the next Shader object created
		// when this one fails
	}

	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, sizeof(infoLog), NULL, infoLog);
		std::cout << "Error compiling fragment shader '" << fragmentPath << "'\n" << infoLog << std::endl;
		//std::cout << "Fragment shader source:\n" << version + defaultDefinesString + definesString + fShaderCode;
	}

	id = glCreateProgram();
	glAttachShader(id, vertexShader);
	glAttachShader(id, fragmentShader);

	if (geometryPath != nullptr) {
		unsigned int geomShader = glCreateShader(GL_GEOMETRY_SHADER);
		auto geomSource = get_file_contents(geometryPath);
		auto code = geomSource.c_str();
		const char* gShaderSources[4] = { version.c_str(), definesString.c_str(), defaultDefinesString.c_str(), code };
		glShaderSource(geomShader, 4, gShaderSources, NULL);
		glCompileShader(geomShader);
		glGetShaderiv(geomShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(geomShader, sizeof(infoLog), NULL, infoLog);
			std::cout << "Error compiling geometry shader '" << geometryPath << "'\n" << infoLog << std::endl;
		}

		glAttachShader(id, geomShader);
	}

	glLinkProgram(id);

	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(id, sizeof(infoLog), NULL, infoLog);
		std::cout << "Error compiling shader program.\nVertex Shader: " << vertexPath << "\nFragment Shader: " << fragmentPath << "'\n" << infoLog << std::endl;
		//std::cout << "Vertex shader source:\n" << version + defaultDefinesString + definesString + vShaderCode << std::endl;
		//std::cout << "Fragment shader source:\n" << version + defaultDefinesString + definesString + fShaderCode << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	Use();
}

void Shader::Use()
{
	glUseProgram(id);
}

void Shader::SetBool(const char * name, bool value)
{
	glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetInt(const char * name, int value)
{
	glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetIntArray(const char* name, int* values, unsigned int count)
{
	glUniform1iv(GetUniformLocation(name), count, values);
}

void Shader::SetUint(const char* name, std::uint32_t value)
{	glUniform1ui(GetUniformLocation(name), value);
}

void Shader::SetFloat(const char * name, float value)
{
	glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetMat4(const char * name, const float * value)
{
	glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, value);
}

void Shader::SetMat4(const char* name, const float* value, int count)
{
	glUniformMatrix4fv(GetUniformLocation(name), count, GL_FALSE, value);
}

void Shader::SetMat4(const char* name, const glm::mat4& m)
{
	glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(m));
}

void Shader::SetMat3(const char* name, const float* value)
{
	glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, value);
}

void Shader::SetMat3(const char* name, const glm::mat3& m)
{
	glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(m));
}

void Shader::SetVec2(const char* name, float x, float y)
{
	glUniform2f(GetUniformLocation(name), x, y);
}

void Shader::SetVec3(const char * name, float x, float y, float z)
{
	glUniform3f(GetUniformLocation(name), x, y, z);
}

void Shader::SetVec3(const char* name, const glm::vec3& vec)
{
	glUniform3f(GetUniformLocation(name), vec.x, vec.y, vec.z);
}

void Shader::SetVec4(const char* name, float x, float y, float z, float w)
{
	glUniform4f(GetUniformLocation(name), x, y, z, w);
}

void Shader::SetVec4(const char* name, const glm::vec4& vec)
{
	glUniform4f(GetUniformLocation(name), vec.x, vec.y, vec.z, vec.w);
}

void Shader::SetVec3Array(const char* name, float* values, unsigned int count)
{
	glUniform3fv(GetUniformLocation(name), count, values);
}

std::string Shader::get_file_contents(const char * path)
{
	std::ifstream in(path);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return contents;
	}
	else
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: '" << path << "'\n";
		return {};
	}
}
