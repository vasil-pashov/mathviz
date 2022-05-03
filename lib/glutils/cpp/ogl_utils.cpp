#include <cerrno>
#include <memory>
#include <array>
#include "glad/glad.h" 
#include "ogl_utils.h"
#include "error_code.h"

namespace GLUtils {

	static const char* getGLErrorString(GLenum err) {
		switch (err) {
			case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
			case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
			case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
			case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
			case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
			case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
			case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
			case GL_CONTEXT_LOST: return "GL_CONTEXT_LOST";
			default: return "Unknown OpenGL error";
		}
	}

	extern EC::ErrorCode checkGLError() {
		GLenum err;
		err = glGetError();
		if (err != GL_NO_ERROR) {
			return EC::ErrorCode(err, "OpenGL error: %s", getGLErrorString(err));
		}
		return EC::ErrorCode();
	}

	inline static GLenum convertBufferType(BufferType type) {
		switch (type) {
			case GLUtils::BufferType::Vertex: return GL_ARRAY_BUFFER;
			case GLUtils::BufferType::Index:  return GL_ELEMENT_ARRAY_BUFFER;
			case GLUtils::BufferType::Uniform: return GL_UNIFORM_BUFFER;
			case GLUtils::BufferType::ShaderStorage: return GL_SHADER_STORAGE_BUFFER;
			default:
			{
				assert(false);
				return -1;
			}
		}
	}

	inline static GLenum convertVertexType(VertexType type) {
		switch (type) {
			case GLUtils::VertexType::Int: return GL_INT;
			case GLUtils::VertexType::Float: return GL_FLOAT;
			default:
			{
				assert(false);
				return -1;
			}
		}
	}

	inline static GLenum convertShaderType(ShaderType type) {
		switch (type) {
			case GLUtils::ShaderType::Vertex: return GL_VERTEX_SHADER;
			case GLUtils::ShaderType::Fragment: return GL_FRAGMENT_SHADER;
			default:
			{
				assert(false);
				return -1;
			}
		}
	}

	inline static GLenum convertBufferAccesType(BufferAccessType type) {
		switch (type) {
			case GLUtils::BufferAccessType::Read: return GL_READ_ONLY;
			case GLUtils::BufferAccessType::Write: return GL_WRITE_ONLY;
			case GLUtils::BufferAccessType::ReadWrite: return GL_READ_WRITE;
			default:
			{
				assert(false);
				return -1;
			}
		}
	}

	Buffer::Buffer(Buffer&& other) noexcept {
		this->handle = other.handle;
		this->type = other.type;
		other.handle = 0;
	}

	Buffer::~Buffer() {
		freeMem();
	}

	EC::ErrorCode Buffer::init(BufferType type) {
		RETURN_ON_ERROR(glGenBuffers(1, &handle));
		this->type = convertBufferType(type);
		return EC::ErrorCode();
	}

	EC::ErrorCode Buffer::upload(int64_t size, const void* data) {
		RETURN_ON_ERROR(glBindBuffer(type, handle));
		RETURN_ON_ERROR(glBufferData(type, size, data, GL_STATIC_DRAW));
		return EC::ErrorCode();
	}

	template<typename T, typename ...Args>
	EC::ErrorCode Buffer::setLayout(const T& layout, Args &... args) {
		RETURN_ON_ERROR_CODE(setLayout(layout));
		return setLayout(args...);
	}

	template<typename T>
	EC::ErrorCode Buffer::setLayout(const T& layout) {
		return setLayout(layout);
	}

	EC::ErrorCode Buffer::setLayout(const AttributeLayout& layout) {
		RETURN_ON_ERROR(glBindBuffer(type, handle));
		const GLenum atribType = convertVertexType(layout.type);
		RETURN_ON_ERROR(glVertexAttribPointer(
			layout.slot,
			layout.componentSize,
			atribType,
			GL_FALSE,
			layout.stride,
			(void*)layout.offset
		));
		RETURN_ON_ERROR(glEnableVertexAttribArray(layout.slot));
		return EC::ErrorCode();
	}

	void Buffer::freeMem() {
		glDeleteBuffers(1, &handle);
	}

	BufferHandle Buffer::getHandle() const {
		return handle;
	}

	EC::ErrorCode Buffer::bind() const {
		RETURN_ON_ERROR(glBindBuffer(type, handle));
		return EC::ErrorCode();
	}

	EC::ErrorCode Buffer::bind(const int bindingIndex) {
		RETURN_ON_ERROR(bind());
		RETURN_ON_ERROR(glBindBufferBase(type, bindingIndex, handle));
		return EC::ErrorCode();
	}

	EC::ErrorCode Buffer::map(void*& map, BufferAccessType access) const {
		const GLenum glAccess = convertBufferAccesType(access);
		glBindBuffer(type, handle);
		map = glMapBuffer(type, glAccess);
		return checkGLError();
	}

	EC::ErrorCode Buffer::unmap() const {
		RETURN_ON_ERROR(glUnmapBuffer(type));
		return EC::ErrorCode();
	}

	// =========================================================
	// ===================== SHADER ============================
	// =========================================================

	Shader::Shader(Shader&& other) noexcept : handle(other.handle) {
		other.handle = 0;
	}

	Shader& Shader::operator=(Shader&& other) noexcept {
		glDeleteShader(handle);
		handle = other.handle;
		other.handle = 0;
		return *this;
	}

	Shader::~Shader() {
		glDeleteShader(handle);
	}

	EC::ErrorCode Shader::loadFromSource(const char* source, ShaderType type) {
		const GLenum shaderType = convertShaderType(type);
		RETURN_ON_ERROR(handle = glCreateShader(shaderType););
		RETURN_ON_ERROR(glShaderSource(handle, 1, &source, nullptr));
		RETURN_ON_ERROR(glCompileShader(handle));
		{
			const EC::ErrorCode err = checkShaderCompilationError();
			if (err.hasError()) {
				return err;
			}
		}
		return EC::ErrorCode();
	}

	EC::ErrorCode Shader::loadFromFile(const char* path, ShaderType type) {
		std::unique_ptr <FILE, decltype(&fclose)> file(fopen(path, "rb"), &fclose);
		if (file == nullptr) {
			return EC::ErrorCode(-1, "Cannot load file %s\n, errno: %d", path, errno);
		}

		fseek(file.get(), 0L, SEEK_END);
		const int64_t size = ftell(file.get());
		rewind(file.get());
		std::unique_ptr<char[]> source = std::make_unique<char[]>(size + 1);
		const int64_t bytesRead = fread((void*)source.get(), 1, size, file.get());
		source[size] = '\0';
		if (size != bytesRead) {
			return EC::ErrorCode(
				-1,
				"Different number of bytes read. Expected to read: %l actually read: %l",
				size,
				bytesRead
			);
		}
		const EC::ErrorCode& err = loadFromSource(source.get(), type);
		if (err.hasError()) {
			return EC::ErrorCode(
				err.getStatus(),
				"Error: \"%s\" while compiling shader from file: %s",
				err.getMessage(),
				path
			);
		}
		return EC::ErrorCode();
	}

	ShaderHandle Shader::getHandle() const {
		return handle;
	}

	EC::ErrorCode Shader::checkShaderCompilationError() {
		int success;
		RETURN_ON_ERROR(glGetShaderiv(handle, GL_COMPILE_STATUS, &success))

			if (!success) {
				char infoLog[512];
				RETURN_ON_ERROR(glGetShaderInfoLog(handle, sizeof(infoLog), NULL, infoLog));
				return EC::ErrorCode(-1, "Error in shader code.\n%s", infoLog);
			}
		return EC::ErrorCode();
	}

	// =========================================================
	// ===================== PROGRAM ===========================
	// =========================================================

	Program::Program(Program&& program) noexcept {
		this->handle = program.handle;
		program.handle = 0;
	}

	Program::~Program() {
		glDeleteProgram(handle);
		handle = 0;
	}

	EC::ErrorCode Program::init(const char* vertexShaderPath, const char* fragmentShaderPath) {
		Shader vertexShader;
		EC::ErrorCode err = vertexShader.loadFromFile(vertexShaderPath, GLUtils::ShaderType::Vertex);
		RETURN_ON_ERROR_CODE(err);

		Shader fragmentShader;
		err = fragmentShader.loadFromFile(fragmentShaderPath, GLUtils::ShaderType::Fragment);
		RETURN_ON_ERROR_CODE(err);

		std::array<Shader*, 2> shaders = { &vertexShader, &fragmentShader };

		handle = glCreateProgram();
		err = checkGLError();
		RETURN_ON_ERROR_CODE(err)

			for (int i = 0; i < shaders.size(); ++i) {
				const unsigned int shaderHandle = shaders[i]->getHandle();
				RETURN_ON_ERROR(glAttachShader(handle, shaderHandle));
			}
		RETURN_ON_ERROR(glLinkProgram(handle));
		return checkProgramLinkErrors();

		RETURN_ON_ERROR_CODE(err);

		glDetachShader(handle, vertexShader.getHandle());
		err = checkGLError();
		RETURN_ON_ERROR_CODE(err);

		glDetachShader(handle, fragmentShader.getHandle());
		err = checkGLError();
		RETURN_ON_ERROR_CODE(err);

		return EC::ErrorCode();
	}

	EC::ErrorCode Program::init(const Shader& vertexShader, const Shader& fragmentShader) {
		RETURN_ON_ERROR(handle = glCreateProgram(););
		std::array<const Shader*, 2> shaders = { &vertexShader, &fragmentShader };
		for (int i = 0; i < shaders.size(); ++i) {
			const unsigned int shaderHandle = shaders[i]->getHandle();
			RETURN_ON_ERROR(glAttachShader(handle, shaderHandle));
		}
		RETURN_ON_ERROR(glLinkProgram(handle));
		return checkProgramLinkErrors();
	}

	ProgramHandle Program::getHandle() const {
		return handle;
	}

	EC::ErrorCode Program::setUniform(const char* name, const glm::mat4& mat, bool transpose) const {
		const int location = glGetUniformLocation(handle, name);
		glUniformMatrix4fv(location, 1, transpose, &mat[0][0]);
		return GLUtils::checkGLError();
	}

	EC::ErrorCode Program::setUniform(const char* name, const glm::vec3& vec) const {
		const int location = glGetUniformLocation(handle, name);
		glUniform3fv(location, 1, &vec.x);
		return GLUtils::checkGLError();
	}

	EC::ErrorCode Program::bind() const {
		glUseProgram(handle);
		return checkGLError();
	}

	void Program::unbind() const {
		glUseProgram(0);
	}

	EC::ErrorCode Program::checkProgramLinkErrors() const {
		int success;
		glGetProgramiv(handle, GL_LINK_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetProgramInfoLog(handle, 512, NULL, infoLog);
			return EC::ErrorCode(-1, "Error while linking shaders code.\n%s", infoLog);
		}
		return EC::ErrorCode();
	}

	// =========================================================
	// ========================= VAO ===========================
	// =========================================================

	VAO::~VAO() {
		freeMem();
	}

	VAO::VAO(VAO&& other) noexcept : handle(other.handle) {
		other.handle = 0;
	}

	EC::ErrorCode VAO::init() {
		RETURN_ON_ERROR(glGenVertexArrays(1, &handle));
		return EC::ErrorCode();
	}

	EC::ErrorCode VAO::bind() const {
		RETURN_ON_ERROR(glBindVertexArray(handle));
		return EC::ErrorCode();
	}

	void VAO::freeMem() {
		unbind();
		glDeleteVertexArrays(1, &handle);
	}

	void VAO::unbind() const {
		glBindVertexArray(0);
	}
}
