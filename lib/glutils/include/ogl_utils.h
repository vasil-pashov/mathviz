#pragma once
#include <vector>
#include <cinttypes>
#include "glad/glad.h"
#include "glm/mat4x4.hpp"
#include "error_code.h"


namespace GLUtils {

	extern EC::ErrorCode checkGLError();

	#define RETURN_ON_ERROR(GLCall) \
	{ \
		GLCall; \
		const EC::ErrorCode err = checkGLError(); \
		if (err.hasError()) { \
			return err; \
		} \
	}

	using ShaderHandle = unsigned int;
	using ProgramHandle = unsigned int;
	using BufferHandle = unsigned int;

	/// Wrapper enum for buffer types. Different API have different
	/// kind of buffer types, but there is some intersection between
	/// all the types. Currently it will contain only the most common types
	enum class BufferType : short {
		Vertex,
		Index,
		Uniform,
		ShaderStorage
	};

	/// Each buffer vertex is composed of some number of elements
	/// (in opengl it is [1;4]), they all must be of the same type
	/// this enum wraps the vertex element type
	enum class VertexType : short {
		Int,
		Float
	};

	enum class BufferAccessType {
		Read,
		Write,
		ReadWrite
	};

	/// Each buffer can contain multiple type of vertices, this is the layout of the vertex.
	struct AttributeLayout {
		int64_t offset; ///< Where in the buffer is the first vertex of this type in bytes
		int stride; ///< The space between two consecutive vertices of this type in bytes
		int slot; ///< Which slot in the shader is this going to occupy. Corresponds to layout (location=slot) in glsl
		short componentSize; ///< How much elements does the vertex contain. (In opengl it is in range [1;4]
		VertexType type; ///< The type of the vertex components. (They are all of the same type)
	};

	/// Simple opengl buffer wrapper with ability to upload/free data and set layout for the shader
	class Buffer {
		using BufferLayout = std::vector <AttributeLayout>;
		Buffer() = default;
		~Buffer();
		Buffer(const Buffer&) = delete;
		Buffer(Buffer&&) noexcept;
		Buffer& operator=(const Buffer&) = delete;
		Buffer& operator=(Buffer&&) = delete;
		/// Initialize the handle to the buffer. Does not allocate GPU memory
		/// @param[in] type - Type of the buffer e.g. vertex, index, etc...
		EC::ErrorCode init(BufferType type);
		/// Upload data to the GPU
		/// @param[in] size - Size in bytes of the data to be uploaded
		/// @param[in] data - Pointer to the data to be uploaded
		EC::ErrorCode upload(int64_t size, const void* data);
		/// Binds the current buffer and sets the layout to the buffer
		template<typename T>
		EC::ErrorCode setLayout(const T& layout);

		template<typename T, typename ...Args>
		EC::ErrorCode setLayout(const T& layout, Args &...);

		/// Destroy the handle and free any data uploaded to this buffer
		void freeMem();
		/// return the opengl handle to the buffer
		BufferHandle getHandle() const;
		/// Binds the current buffer
		EC::ErrorCode bind() const;
		/// Binds the current buffer to a shader binding. Used for UBO and SSBO
		/// @param[in] bindingIndex The binding index to which the buffer will be bound. Currently must be specified in the shader
		EC::ErrorCode bind(const int bindingIndex);
		EC::ErrorCode unbind() const;
		EC::ErrorCode map(void*& map, BufferAccessType access) const;
		EC::ErrorCode unmap() const;
		EC::ErrorCode setLayout(const AttributeLayout& layout);
	private:
		unsigned int handle;
		unsigned int type;
	};

	inline EC::ErrorCode Buffer::unbind() const {
		RETURN_ON_ERROR(glBindBuffer(type, 0));
		return EC::ErrorCode();
	}

	/// Shader type program wrapper
	enum class ShaderType : short {
		Vertex,
		Fragment
	};

	/// Class used to compile shader source and store handle to it.
	/// Several shaders can be then linked into one shader program, which
	/// tells the gl pipeline what to do on each stage.
	class Shader {
	public:
		Shader() = default;
		~Shader();
		// Copy semantics
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;
		// Move semantics
		Shader(Shader&&) noexcept;
		Shader& operator=(Shader&&) noexcept;
		/// Create the shader given the source code
		/// @param[in] source - Source code for the shader
		/// @param[in] type - Type of the shader e.g. vertex, fragment, etc...
		EC::ErrorCode loadFromSource(const char* source, ShaderType type);
		/// Create the shader by reading its source code from the hard drive
		/// @param[in] path - the path to the file with the shader source code
		/// @param[in] type - Type of the shader e.g. vertex, fragment, etc...
		EC::ErrorCode loadFromFile(const char* path, ShaderType type);
		/// Return the api handle to the shader
		ShaderHandle getHandle() const;
	private:
		unsigned int handle;
		/// Call this in order to get compilation errors (if any) for the sahder
		EC::ErrorCode checkShaderCompilationError();
	};

	/// Class which will link different shaders into one program and hold the handle to it
	class Program {
		Program() = default;
		~Program();
		// Copy semantics
		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;
		// Move semantics
		Program(Program&&) noexcept;
		Program& operator=(Program&&) = default;
		EC::ErrorCode init(const char* vertexShaderPath, const char* indexShaderPath);
		EC::ErrorCode init(const Shader& vertexShader, const Shader& fragmentShader);
		/// Return the api handle to the program
		ProgramHandle getHandle() const;
		/// Set a uniform float mat4
		/// @param[in] name - Name of the uniform must match a uniform in the shaders
		/// @param[in] matrix - 4x4 matrix which will be transfered as uniform
		/// @param[in] transpose - if true the matrix transpose will be uploaded to the GPU
		EC::ErrorCode setUniform(const char* name, const glm::mat4& matrix, bool transpose) const;
		/// Set a uniform float vec3 for vector
		/// @param[in] name - Name of the uniform must match a uniform in the shaders
		/// @param[in] float - pointer to the first element of the matrix
		EC::ErrorCode setUniform(const char* name, const glm::vec3& vector) const;
		EC::ErrorCode bind() const;
		void unbind() const;
	private:
		unsigned int handle;
		/// Call this in order to get link errors (if any) for the sahder program
		EC::ErrorCode checkProgramLinkErrors() const;
	};

	/// VAO is special opengl feature. It "remembers" buffer layout.
	/// It can also remember which index buffer was bound
	struct VAO {
	public:
		VAO() = default;
		~VAO();
		// Copy semantics
		VAO(const VAO&) = delete;
		VAO& operator=(const VAO&) = delete;
		// Move semantics
		VAO(VAO&&) noexcept;
		VAO& operator=(VAO&&) = delete;
		/// Init the VAO handle
		EC::ErrorCode init();
		/// Bind the VAO, when bound the vao starts recording buffer layouts and index buffers
		EC::ErrorCode bind() const;
		/// Unbinds the current VAO by binding the default vao which is 0
		void unbind() const;
		/// unbind and delete the vao
		void freeMem();
	private:
		unsigned int handle;
	};
}