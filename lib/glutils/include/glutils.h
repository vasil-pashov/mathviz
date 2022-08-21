#pragma once
#include <vector>
#include <cinttypes>
#include <unordered_map>
#include "glad/glad.h"
#include "glm/mat4x4.hpp"
#include "error_code.h"


namespace GLUtils {

	extern EC::ErrorCode checkGLError();

	#define RETURN_ON_GL_ERROR(GLCall) \
	{ \
		GLCall; \
		const EC::ErrorCode err = GLUtils::checkGLError(); \
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
	enum class BufferType {
		Vertex,
		Index,
		Uniform,
		ShaderStorage
	};

	/// Each buffer vertex is composed of some number of elements
	/// (in opengl it is [1;4]), they all must be of the same type
	/// this enum wraps the vertex element type
	enum class VertexType {
		Int,
		Float
	};

	enum class BufferAccessType {
		Read,
		Write,
		ReadWrite
	};

	/// Each buffer can contain multiple type of vertices, this is the layout of the vertex.
	// struct AttributeLayout {
	// 	int64_t offset; ///< Where in the buffer is the first vertex of this type in bytes
	// 	int stride; ///< The space between two consecutive vertices of this type in bytes
	// 	int slot; ///< Which slot in the shader is this going to occupy. Corresponds to layout (location=slot) in glsl
	// 	short componentSize; ///< How much elements does the vertex contain. (In opengl it is in range [1;4]
	// 	VertexType type; ///< The type of the vertex components. (They are all of the same type)
	// 	bool normalized;
	// };

	struct AttributeLayout {
		int count;
		VertexType type;
		bool normalized;
	};

	class BufferLayout {
	public:
		BufferLayout() : stride(0) {}
		BufferLayout(int count) : layout(count), stride(0) {}
		BufferLayout(const BufferLayout&) = delete;
		BufferLayout& operator=(const BufferLayout&) = delete;
		BufferLayout(BufferLayout&&) = delete;
		BufferLayout& operator=(const BufferLayout&&) = delete;
		void addAttribute(VertexType type, int count, bool normalized = false);
		[[nodiscard]]
		const std::vector<AttributeLayout>& getAttributes() const {
			return layout;
		}
		[[nodiscard]]
		unsigned int getStride() const {
			return stride;
		}
	private:
		std::vector<AttributeLayout> layout;
		unsigned int stride;
	};

	/// Simple opengl buffer wrapper with ability to upload/free data and set layout for the shader
	class BufferBase {
	public:
		~BufferBase();
		/// Initialize the handle to the buffer. Does not allocate GPU memory
		[[nodiscard]]
		EC::ErrorCode init(int64_t size);
		EC::ErrorCode init(int64_t size, const void* source);
		EC::ErrorCode init(int64_t size, const void* source, const BufferLayout& layout);
		EC::ErrorCode upload(int64_t offset, int64_t size, const void* data);
		/// Upload data to the GPU
		/// @param[in] size - Size in bytes of the data to be uploaded
		/// @param[in] data - Pointer to the data to be uploaded
		[[nodiscard]]
		// EC::ErrorCode upload(int64_t size, const void* data);
		/// @brief Bind the buffer and set the attribute layout for it.
		[[nodiscard]]
		EC::ErrorCode setLayout(const BufferLayout& layout);
		/// Destroy the handle and free any data uploaded to this buffer
		void freeMem();
		/// return the opengl handle to the buffer
		[[nodiscard]]
		BufferHandle getHandle() const;
		/// Binds the current buffer
		[[nodiscard]]
		EC::ErrorCode bind() const;
		[[nodiscard]]
		EC::ErrorCode unbind() const;
		[[nodiscard]]
		EC::ErrorCode map(void*& map, BufferAccessType access) const;
		[[nodiscard]]
		EC::ErrorCode unmap() const;
	protected:
		[[nodiscard]]
		EC::ErrorCode setLayoutInternal(const BufferLayout& layout);
		/// @param[in] type - Type of the buffer e.g. vertex, index
		BufferBase(BufferType type);
		BufferBase(const BufferBase&) = delete;
		BufferBase(BufferBase&&) noexcept;
		BufferBase& operator=(const BufferBase&) = delete;
		BufferBase& operator=(BufferBase&&) noexcept;;
		unsigned int handle;
		unsigned int type;
	};

	class VertexBuffer : public BufferBase {
	public:
		VertexBuffer();
	};

	class IndexBuffer : public BufferBase {
	public:
		IndexBuffer();
	};

	class UniformBuffer : public BufferBase {
	public:
		UniformBuffer() noexcept;
		UniformBuffer(int bindingPosition) noexcept;
		UniformBuffer(UniformBuffer&&) noexcept;
		UniformBuffer& operator=(UniformBuffer&&) noexcept;;
		void setBindingPosition(unsigned int bindingPosition);
		EC::ErrorCode bind();
	private:
		int bindingPosition;
	};

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
		/// @param[in] source Source code for the shader
		/// @param[in] length The length of the shader (in characters) 
		/// @param[in] type Type of the shader e.g. vertex, fragment, etc...
		[[nodiscard]]
		EC::ErrorCode loadFromSource(const char* source, int length, ShaderType type);
		/// Create the shader given the source code
		/// @param[in] source - Source code for the shader
		/// @param[in] type - Type of the shader e.g. vertex, fragment, etc...
		[[nodiscard]]
		EC::ErrorCode loadFromSource(const char* source, ShaderType type);
		/// Create the shader by reading its source code from the hard drive
		/// @param[in] path - the path to the file with the shader source code
		/// @param[in] type - Type of the shader e.g. vertex, fragment, etc...
		[[nodiscard]]
		EC::ErrorCode loadFromFile(const char* path, ShaderType type);
		/// Return the api handle to the shader
		[[nodiscard]]
		ShaderHandle getHandle() const;
	private:
		unsigned int handle;
		/// Call this in order to get compilation errors (if any) for the sahder
		EC::ErrorCode checkShaderCompilationError();
	};

	class Pipeline {
	public:
		using It = std::unordered_map<ShaderType, Shader>::iterator;
		using ConstIt = std::unordered_map<ShaderType, Shader>::const_iterator;
		Pipeline() = default;
		Pipeline(const Pipeline&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;
		Pipeline(Pipeline&&) = default;
		Pipeline& operator=(Pipeline&&) = default;
		EC::ErrorCode init(const char* path);
		It begin();
		It end();
		ConstIt begin() const;
		ConstIt end() const;
	private:
		std::unordered_map<ShaderType, Shader> shaders;
	};

	/// Class which will link different shaders into one program and hold the handle to it
	class Program {
	public:
		Program() = default;
		~Program();
		// Copy semantics
		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;
		// Move semantics
		Program(Program&&) noexcept;
		Program& operator=(Program&&) noexcept;
		[[nodiscard]]
		EC::ErrorCode init(const Pipeline& pipeline);
		/// Return the api handle to the program
		[[nodiscard]]
		ProgramHandle getHandle() const;
		/// Set a uniform float mat4
		/// @param[in] name - Name of the uniform must match a uniform in the shaders
		/// @param[in] matrix - 4x4 matrix which will be transfered as uniform
		/// @param[in] transpose - if true the matrix transpose will be uploaded to the GPU
		[[nodiscard]]
		EC::ErrorCode setUniform(const char* name, const glm::mat4& matrix, bool transpose) const;
		/// Set a uniform float vec3 for vector
		/// @param[in] name - Name of the uniform must match a uniform in the shaders
		/// @param[in] float - pointer to the first element of the matrix
		[[nodiscard]]
		EC::ErrorCode setUniform(const char* name, const glm::vec3& vector) const;
		[[nodiscard]]
		EC::ErrorCode setUniform(const char* name, float value) const;
		[[nodiscard]]
		EC::ErrorCode bind() const;
		void unbind() const;
		void freeMem();
	private:
		unsigned int handle;
		/// Call this in order to get link errors (if any) for the sahder program
		EC::ErrorCode checkProgramLinkErrors() const;
	};

	/// VAO is special opengl feature. It "remembers" buffer layout.
	/// It can also remember which index buffer was bound
	class VAO {
	public:
		VAO() = default;
		~VAO();
		// Copy semantics
		VAO(const VAO&) = delete;
		VAO& operator=(const VAO&) = delete;
		// Move semantics
		VAO(VAO&&) noexcept;
		VAO& operator=(VAO&&) noexcept;
		/// Init the VAO handle
		[[nodiscard]]
		EC::ErrorCode init();
		/// Bind the VAO, when bound the vao starts recording buffer layouts and index buffers
		[[nodiscard]]
		EC::ErrorCode bind() const;
		/// Unbinds the current VAO by binding the default vao which is 0
		[[nodiscard]]
		EC::ErrorCode unbind() const;
		/// unbind and delete the vao
		void freeMem();
	private:
		unsigned int handle;
	};

	/// Defines what happens when texture coordinates go out of range [0;1]
	enum class TextureWrap2D {
		/// The texture starts repeating
		Repeat,
		/// Clamps all coordinates to [0;1] range. Thus coordinates higher than
		/// one will be sampled at 1, coords lower that 0 will be sampled at 0.
		Clamp,
	};


	/// Defines how the image will be sampled in cases where one screen coordinate
	/// (pixel) does not match to one image coordinate (texel).
	enum class TextureFilter2D {
		/// Selects the texel whose center is closest to the texture coordinate
		Nearest,
		/// takes an interpolated value from the texture coordinate's neighboring texels,
		/// approximating a color between the texels. The smaller the distance from the texture
		/// coordinate to a texel's center, the more that texel's color contributes to the sampled color
		Linear
	};

	/// Defines which mipmap layer to use
	enum class MipMapFilter2D {
		/// No mipmaps will be created
		None,
		/// When between two mipmaps sample the one that is closest
		Nearest,
		/// When between two mipmaps interpolate between both. The one
		/// which is closer will have more influence on the result.
		Linear
	};

	enum class TextureFormat2D {
		RGB
	};

	class Texture2D {
	public:
		Texture2D() : texture(0), width(0), height(0), channelsCount(0) {}

		~Texture2D() {
			freeMem();
		}

		/// Load texture from disk into video memory
		/// @param[in] path The path to the texture file
		/// @param[in] format Describes the image file contents e.g. the type of the channels
		/// (red/green/blue, red/green/blue/alpha, etc.) the count of the channels etc... The
		/// format of the created image will be mapped as close as possible to the original format.
		/// @param[in] wrap Defines what will happen when texture coordinate go out of range [0;1]
		/// in both u and v directions. The same option will be applied for both u and v.
		/// @param[in] Defines how the image will be sampled in cases where one screen coordinate
		/// (pixel) does not match to one image coordinate (texel).
		[[nodiscard]]
		EC::ErrorCode init(
			const char* path,
			TextureFormat2D format,
			TextureWrap2D wrap,
			TextureFilter2D filter
		);

		/// Load texture from disk into video memory
		/// @param[in] path The path to the texture file
		/// @param[in] format Describes the image file contents e.g. the type of the channels
		/// (red/green/blue, red/green/blue/alpha, etc.) the count of the channels etc... The
		/// format of the created image will be mapped as close as possible to the original format.
		/// @param[in] wrapU Defines what will happen when texture coordinate go out of range [0;1]
		/// in the u (horizontal) direction
		/// @param[in] wrapV Defines what will happen when texture coordinate go out of range [0;1]
		/// in the v (vertical) direction
		/// @param[in] minFilter Defines how the image will be sampled in cases where one screen coordinate
		/// (pixel) does not match to one image coordinate (texel) and one texel corresponds to less than a pixel.
		/// @param[in] minFilter Defines how the image will be sampled in cases where one screen coordinate
		/// (pixel) does not match to one image coordinate (texel) and one texel corresponds to multiple pixels
		/// @param[in] mipMapFilter Defines whether to use mipmaps and how to choose which mipmap to sample
		[[nodiscard]]
		EC::ErrorCode init(
			const char* path,
			TextureFormat2D format,
			TextureWrap2D wrapU,
			TextureWrap2D wrapV,
			TextureFilter2D minFilter,
			TextureFilter2D maxFilter,
			MipMapFilter2D mipMapFilter
		);

		[[nodiscard]]
		EC::ErrorCode bind(int unit) const;

		void freeMem();
	private:
		/// Some actions do not require setting the texture unit.
		/// All of those actions however should not be performed
		/// by the end user.
		[[nodiscard]]
		EC::ErrorCode bind() const;
		unsigned int texture;
		int width;
		int height;
		int channelsCount;
	};
}