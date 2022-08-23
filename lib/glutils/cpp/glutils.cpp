#include <cerrno>
#include <memory>
#include <array>
#include "glad/glad.h" 
#include "glutils.h"
#include "error_code.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace GLUtils {

	[[nodiscard]]
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

	[[nodiscard]]
	extern EC::ErrorCode checkGLError() {
		GLenum err;
		err = glGetError();
		if (err != GL_NO_ERROR) {
			return EC::ErrorCode(err, "OpenGL error: %s", getGLErrorString(err));
		}
		return EC::ErrorCode();
	}

	[[nodiscard]]
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

	[[nodiscard]]
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

	[[nodiscard]]
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

	[[nodiscard]]
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

	[[nodiscard]]
	inline static int getTypeSize(VertexType type) {
		switch (type) {
			case GLUtils::VertexType::Int: return 4;
			case GLUtils::VertexType::Float: return 4;
			default:
			{
				assert(false);
				return 0;
			}
		}
	}

	[[nodiscard]]
	inline static int convertTexture2DFormat(TextureFormat2D format) {
		switch (format) {
			case GLUtils::TextureFormat2D::RGB: return GL_RGB;
			default: {
				assert(false && "Unknown image format!");
				return -1;
			}
		}
	}

	[[nodiscard]]
	inline static int convertTextureWrap2D(TextureWrap2D wrap) {
		switch (wrap) {
			case GLUtils::TextureWrap2D::Repeat: return GL_REPEAT;
			case GLUtils::TextureWrap2D::Clamp: return GL_CLAMP_TO_EDGE;
			default:
			{
				assert(false && "Unknown texture wrap");
				return -1;
			}
		}
	}

	[[nodiscard]]
	inline static int convertTextureFilter2D(TextureFilter2D filter) {
		switch (filter) {
			case GLUtils::TextureFilter2D::Nearest: return GL_NEAREST;
			case GLUtils::TextureFilter2D::Linear: return GL_LINEAR;
			default:
			{
				assert(false, "Unknown texture filter");
				return -1;
			}
		}
	}

	[[nodiscard]]
	inline static int convertMipMapFilter(TextureFilter2D minFilter, MipMapFilter2D mipMapFilter) {
		assert(mipMapFilter != MipMapFilter2D::None);
		if (minFilter == TextureFilter2D::Nearest) {
			if (mipMapFilter == MipMapFilter2D::Nearest) {
				return GL_NEAREST_MIPMAP_NEAREST;
			} else if(mipMapFilter == MipMapFilter2D::Linear) {
				return GL_NEAREST_MIPMAP_LINEAR;
			} else {
				assert(false && "Unknown MipMapFilter2D option");
				return -1;
			}
		} else if(minFilter == TextureFilter2D::Linear) {
			assert(minFilter == TextureFilter2D::Linear);
			if (mipMapFilter == MipMapFilter2D::Nearest) {
				return GL_LINEAR_MIPMAP_NEAREST;
			} else if(mipMapFilter == MipMapFilter2D::Linear) {
				return GL_LINEAR_MIPMAP_LINEAR;
			} else {
				assert(false && "Unknown MipMapFilter2D option");
				return -1;
			}
		}
		assert(false && "Unknown TextureFilter2D min filter option.");
		return -1;
	}

	void BufferLayout::addAttribute(VertexType type, int count, bool normalized) {
		layout.push_back({ count, type, normalized });
		stride += getTypeSize(type) * count;
	}

	BufferBase::BufferBase(BufferType type) :
		type(convertBufferType(type)),
		handle(0)
	{

	}

	BufferBase::BufferBase(BufferBase&& other) noexcept {
		this->handle = other.handle;
		this->type = other.type;
		other.handle = 0;
	}

	BufferBase& BufferBase::operator=(BufferBase&& other) noexcept {
		assert(&other != this);
		freeMem();
		handle = other.handle;
		type = other.type;
		other.handle = 0;
		return *this;
	}


	BufferBase::~BufferBase() {
		freeMem();
	}

	EC::ErrorCode BufferBase::init(int64_t size, const void* data, const BufferLayout& layout) {
		RETURN_ON_GL_ERROR(glGenBuffers(1, &handle));
		RETURN_ON_ERROR_CODE(bind());
		RETURN_ON_GL_ERROR(glBufferData(type, size, data, GL_STATIC_DRAW));
		RETURN_ON_ERROR_CODE(setLayoutInternal(layout));
		RETURN_ON_ERROR_CODE(unbind());
		return EC::ErrorCode();
	}

	EC::ErrorCode BufferBase::init(int64_t size, const void* data) {
		RETURN_ON_GL_ERROR(glGenBuffers(1, &handle));
		RETURN_ON_ERROR_CODE(bind());
		RETURN_ON_GL_ERROR(glBufferData(type, size, data, GL_STATIC_DRAW));
		RETURN_ON_ERROR_CODE(unbind());
		return EC::ErrorCode();
	}

	EC::ErrorCode BufferBase::init(int64_t size) {
		return init(size, nullptr);
	}

	EC::ErrorCode BufferBase::upload(int64_t offset, int64_t size, const void* data) {
		RETURN_ON_ERROR_CODE(bind());
		RETURN_ON_GL_ERROR(glBufferSubData(type, offset, size, data));
		RETURN_ON_ERROR_CODE(unbind());
		return EC::ErrorCode();
	}

	EC::ErrorCode BufferBase::setLayout(const BufferLayout& layout) {
		RETURN_ON_ERROR_CODE(bind());
		RETURN_ON_ERROR_CODE(setLayout(layout));
		RETURN_ON_ERROR_CODE(unbind());
	}

	EC::ErrorCode BufferBase::setLayoutInternal(const BufferLayout& layout) {
		int offset = 0;
		for (int i = 0; i < layout.getAttributes().size(); ++i) {
			AttributeLayout attribute = layout.getAttributes()[i];
			const GLenum attribType = convertVertexType(attribute.type);
			RETURN_ON_GL_ERROR(glVertexAttribPointer(
				i,
				attribute.count,
				attribType,
				attribute.normalized ? GL_TRUE : GL_FALSE,
				layout.getStride(),
				(const void*)offset
			));
			RETURN_ON_GL_ERROR(glEnableVertexAttribArray(i));
			offset += attribute.count * getTypeSize(attribute.type);
		}
		return EC::ErrorCode();
	}

	void BufferBase::freeMem() {
		glDeleteBuffers(1, &handle);
	}

	BufferHandle BufferBase::getHandle() const {
		return handle;
	}

	EC::ErrorCode BufferBase::bind() const {
		RETURN_ON_GL_ERROR(glBindBuffer(type, handle));
		return EC::ErrorCode();
	}

	EC::ErrorCode BufferBase::map(void*& map, BufferAccessType access) const {
		const GLenum glAccess = convertBufferAccesType(access);
		map = glMapBuffer(type, glAccess);
		return checkGLError();
	}

	EC::ErrorCode BufferBase::unmap() const {
		RETURN_ON_GL_ERROR(glUnmapBuffer(type));
		return EC::ErrorCode();
	}

	EC::ErrorCode BufferBase::unbind() const {
		RETURN_ON_GL_ERROR(glBindBuffer(type, 0));
		return EC::ErrorCode();
	}

	VertexBuffer::VertexBuffer() :
		BufferBase(BufferType::Vertex)
	{ }

	IndexBuffer::IndexBuffer() :
		BufferBase(BufferType::Index) {
 }

	UniformBuffer::UniformBuffer() noexcept :
		BufferBase(BufferType::Uniform),
		bindingPosition(-1)
	{ }

	UniformBuffer::UniformBuffer(int bindingPosition) noexcept :
		BufferBase(BufferType::Uniform),
		bindingPosition(bindingPosition)
	{ }

	UniformBuffer::UniformBuffer(UniformBuffer&& ub) noexcept :
		BufferBase(std::move(ub)),
		bindingPosition(ub.bindingPosition)
	{
		ub.bindingPosition = -1;
	}

	UniformBuffer& UniformBuffer::operator=(UniformBuffer&& ub) noexcept {
		BufferBase::operator=(std::move(ub));
		bindingPosition = ub.bindingPosition;
		ub.bindingPosition = 0;
		return *this;
	}

	void UniformBuffer::setBindingPosition(unsigned int bindingPosition) {
		this->bindingPosition = bindingPosition;
	}

	EC::ErrorCode UniformBuffer::bind() {
		assert(bindingPosition >= 0);
		RETURN_ON_ERROR_CODE(BufferBase::bind());
		RETURN_ON_GL_ERROR(glBindBufferBase(type, bindingPosition, handle));
		return EC::ErrorCode();
	}

	// =========================================================
	// ===================== SHADER ============================
	// =========================================================

	Shader::Shader() : handle(0) { }

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
		RETURN_ON_GL_ERROR(handle = glCreateShader(shaderType););
		RETURN_ON_GL_ERROR(glShaderSource(handle, 1, &source, nullptr));
		RETURN_ON_GL_ERROR(glCompileShader(handle));
		{
			const EC::ErrorCode err = checkShaderCompilationError();
			if (err.hasError()) {
				return err;
			}
		}
		return EC::ErrorCode();
	}

	EC::ErrorCode Shader::loadFromSource(const char* source, const int length, ShaderType type) {
		const GLenum shaderType = convertShaderType(type);
		RETURN_ON_GL_ERROR(handle = glCreateShader(shaderType););
		RETURN_ON_GL_ERROR(glShaderSource(handle, 1, &source, &length));
		RETURN_ON_GL_ERROR(glCompileShader(handle));
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
		RETURN_ON_GL_ERROR(glGetShaderiv(handle, GL_COMPILE_STATUS, &success))

			if (!success) {
				char infoLog[512];
				RETURN_ON_GL_ERROR(glGetShaderInfoLog(handle, sizeof(infoLog), NULL, infoLog));
				return EC::ErrorCode(-1, "Error in shader code.\n%s", infoLog);
			}
		return EC::ErrorCode();
	}
	// =========================================================
	// ==================== PIPELINE ===========================
	// =========================================================

	EC::ErrorCode Pipeline::init(const char* path) {
		std::unique_ptr<FILE, decltype(&fclose)> shaderFile(fopen(path, "rb"), &fclose);
		if (shaderFile == nullptr) {
			return EC::ErrorCode(errno, "Cannot open file %s path: %s", path, strerror(errno));
		}
		fseek(shaderFile.get(), 0L, SEEK_END);
		const int64_t size = ftell(shaderFile.get());
		rewind(shaderFile.get());
		std::string joinedShader;
		joinedShader.resize(size);
		fread(joinedShader.data(), 1, size, shaderFile.get());

		// The internal convention is that when we have many shaders in a single file
		// each shader will start with the line #shader <type_of_shader>
		// After <type_of_shader> there must be a new line.
		int currentPos = joinedShader.find_first_of("#shader", 0);
		while (currentPos < joinedShader.size()) {
			while (std::isblank(joinedShader[currentPos])) {
				currentPos++;
			}
			// The #shader <shader_type> must end with a newline by convention
			const int shaderStart = joinedShader.find("\n", currentPos, 1) + 1;
			const int nextShaderStart = joinedShader.find("#shader", shaderStart, sizeof("#shader") - 1);
			const int shaderEnd = nextShaderStart == std::string::npos ? size : nextShaderStart;
			const int shaderSize = shaderEnd - shaderStart;
			const char* shaderDirective = joinedShader.c_str() + currentPos + sizeof("#shader");
			ShaderType shaderType;
			if (std::strncmp("fragment", shaderDirective, sizeof("fragment") - 1) == 0) {
				shaderType = ShaderType::Fragment;
			} else if (std::strncmp("vertex", shaderDirective, sizeof("vertex") - 1) == 0) {
				shaderType = ShaderType::Vertex;
			} else {
				const int shaderTypeLen = joinedShader.find('\n', shaderStart) - currentPos;
				const std::string shaderTypeStr = joinedShader.substr(currentPos, shaderTypeLen);
				return EC::ErrorCode("Unknown shader type: %s", shaderTypeStr.c_str());
			}

			RETURN_ON_ERROR_CODE(shaders[shaderType].loadFromSource(
				joinedShader.c_str() + shaderStart,
				shaderSize,
				shaderType)
			);

			currentPos = shaderEnd;
		}
		return EC::ErrorCode();
	}

	Pipeline::It Pipeline::begin() {
		return shaders.begin();
	}

	Pipeline::It Pipeline::end() {
		return shaders.end();
	}

	Pipeline::ConstIt Pipeline::begin() const {
		return shaders.begin();
	}

	Pipeline::ConstIt Pipeline::end() const {
		return shaders.end();
	}

	// =========================================================
	// ===================== PROGRAM ===========================
	// =========================================================

	Program::Program() : handle(0) { }

	Program::Program(Program&& program) noexcept {
		this->handle = program.handle;
		program.handle = 0;
	}

	Program& Program::operator=(Program&& other) noexcept {
		freeMem();
		this->handle = other.handle;
		other.handle = 0;
		return *this;
	}

	Program::~Program() {
		freeMem();
	}

	EC::ErrorCode Program::init(const Pipeline& pipeline) {
		RETURN_ON_GL_ERROR(handle = glCreateProgram(););
		for (auto& it : pipeline) {
			const ShaderHandle h = it.second.getHandle();
			RETURN_ON_GL_ERROR(glAttachShader(handle, h));
		}
		RETURN_ON_GL_ERROR(glLinkProgram(handle));
		RETURN_ON_ERROR_CODE(checkProgramLinkErrors());
		for (auto& it : pipeline) {
			const ShaderHandle h = it.second.getHandle();
			glDetachShader(handle, h);
			assert(checkGLError().hasError() == false);
		}
		return EC::ErrorCode();
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

	EC::ErrorCode Program::setUniform(const char* name, float value) const {
		const int location = glGetUniformLocation(handle, name);
		glUniform1f(location, value);
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


	void Program::freeMem() {
		if (handle) {
			glDeleteProgram(handle);
			assert(checkGLError().hasError() == false);
			handle = 0;
		}
	}
	// =========================================================
	// ========================= VAO ===========================
	// =========================================================

	VAO::VAO() : handle(0) { }

	VAO::~VAO() {
		freeMem();
	}

	VAO::VAO(VAO&& other) noexcept : handle(other.handle) {
		other.handle = 0;
	}

	VAO& VAO::operator=(VAO&& other) noexcept {
		freeMem();
		handle = other.handle;
		other.handle = 0;
		return *this;
	}

	[[nodiscard]]
	EC::ErrorCode VAO::init() {
		assert(handle == 0);
		RETURN_ON_GL_ERROR(glGenVertexArrays(1, &handle));
		return EC::ErrorCode();
	}

	[[nodiscard]]
	EC::ErrorCode VAO::bind() const {
		assert(handle != 0);
		RETURN_ON_GL_ERROR(glBindVertexArray(handle));
		return EC::ErrorCode();
	}

	void VAO::freeMem() {
		assert(handle != 0);
		glDeleteVertexArrays(1, &handle);
	}

	[[nodiscard]]
	EC::ErrorCode VAO::unbind() const {
		RETURN_ON_GL_ERROR(glBindVertexArray(0));
		return EC::ErrorCode();
	}

	// =========================================================
	// ======================= TEXTURE =========================
	// =========================================================

	EC::ErrorCode Texture2D::init(
		const char* path,
		TextureFormat2D format,
		TextureWrap2D wrap,
		TextureFilter2D filter
	) {
		std::unique_ptr<unsigned char, decltype(&stbi_image_free)> data(
			stbi_load(path, &width, &height, &channelsCount, 0),
			&stbi_image_free
		);
		if (data == nullptr) {
			return EC::ErrorCode("Failed to load texture: %s", path);
		}
		if (channelsCount != 3) {
			return EC::ErrorCode("Unsupported data format. Channels count is: %d", channelsCount);
		}
		RETURN_ON_GL_ERROR(glGenTextures(1, &texture));
		RETURN_ON_ERROR_CODE(bind());


		const int texWrap2DGL = convertTextureWrap2D(wrap);
		RETURN_ON_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texWrap2DGL));
		RETURN_ON_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texWrap2DGL));

		const int texFilter2DGL = convertTextureFilter2D(filter);
		RETURN_ON_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFilter2DGL));
		RETURN_ON_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texFilter2DGL));

		const int texFormat2DGL = convertTexture2DFormat(format);
		RETURN_ON_GL_ERROR(glTexImage2D(
			GL_TEXTURE_2D,
			0,
			texFormat2DGL,
			width,
			height,
			0,
			texFormat2DGL,
			GL_UNSIGNED_BYTE,
			data.get()
		));

		return EC::ErrorCode();
	}

	EC::ErrorCode Texture2D::init(
		const char* path,
		TextureFormat2D format,
		TextureWrap2D wrapU,
		TextureWrap2D wrapV,
		TextureFilter2D minFilter,
		TextureFilter2D maxFilter,
		MipMapFilter2D mipMapFilter
	) {
		std::unique_ptr<unsigned char, decltype(&stbi_image_free)> data(
			stbi_load(path, &width, &height, &channelsCount, 0),
			&stbi_image_free
		);
		if (data == nullptr) {
			return EC::ErrorCode("Failed to load texture: %s", path);
		}
		if (channelsCount != 3) {
			return EC::ErrorCode("Unsupported data format. Channels count is: %d", channelsCount);
		}
		RETURN_ON_GL_ERROR(glGenTextures(1, &texture));
		RETURN_ON_ERROR_CODE(bind());

		const int uTexWrap2DGL = convertTextureWrap2D(wrapU);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, uTexWrap2DGL);

		const int vTexWrap2DGL = convertTextureWrap2D(wrapV);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vTexWrap2DGL);

		const int minFilter2DGL = mipMapFilter != MipMapFilter2D::None ?
			convertMipMapFilter(minFilter, mipMapFilter) :
			convertTextureFilter2D(minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter2DGL);

		const int maxFilter2DGL = convertTextureFilter2D(maxFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, maxFilter2DGL);

		const int glTextureFormat = convertTexture2DFormat(format);
		RETURN_ON_GL_ERROR(glTexImage2D(
			GL_TEXTURE_2D,
			0,
			glTextureFormat,
			width,
			height,
			0,
			glTextureFormat,
			GL_UNSIGNED_BYTE,
			data.get()
		));
		if (mipMapFilter != MipMapFilter2D::None) {
			RETURN_ON_GL_ERROR(glGenerateMipmap(GL_TEXTURE_2D));
		}
		return EC::ErrorCode();
	}

	EC::ErrorCode Texture2D::bind(int unit) const {
		RETURN_ON_GL_ERROR(glActiveTexture(GL_TEXTURE0 + unit));
		RETURN_ON_GL_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
		return EC::ErrorCode();
	}

	EC::ErrorCode Texture2D::bind() const {
		RETURN_ON_GL_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
		return EC::ErrorCode();
	}

	void Texture2D::freeMem() {
		glDeleteTextures(1, &texture);
		const EC::ErrorCode err = checkGLError();
		assert(err.hasError() == false);
		texture = 0;
	}
}
