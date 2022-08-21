#pragma once
#include <unordered_map>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "glutils.h"
#include "error_code.h"
#include "shader_bindings.h"

namespace GLUtils {
	class Program;
}

namespace MathViz {

	class IMaterial {
	public:
		using ShaderId = int;
		IMaterial(const GLUtils::Program& p) : program {p} {}
		virtual ~IMaterial() {}
		virtual EC::ErrorCode setUniforms() const = 0;
		const GLUtils::Program& getProgram() const {
			return program;
		}
	protected:
		const GLUtils::Program& program;
	};

	class FlatColor : public IMaterial {
	public:
		explicit FlatColor(const GLUtils::Program& p);
		FlatColor(const GLUtils::Program& p, const glm::vec3& color);
		void setColor(const glm::vec3& color);
		glm::vec3 getColor() const;
		EC::ErrorCode setUniforms() const override;
	private:
		glm::vec3 color;
	};

	class Gradient2D : public IMaterial {
	public:
		explicit Gradient2D(const GLUtils::Program& p);
		Gradient2D(
			const GLUtils::Program& p,
			const glm::vec3& start,
			const glm::vec3& end,
			const glm::vec3& col0rStart,
			const glm::vec3& col0rEnd
		);
		EC::ErrorCode setUniforms() const override;
	private:
		glm::vec3 start;
		glm::vec3 end;
		glm::vec3 colorStart;
		glm::vec3 colorEnd;
	};

	class MaterialFactory {
	public:
		MaterialFactory() = default;
		EC::ErrorCode init() {
			for (int i = 0; i < int(ShaderTable::Count); ++i) {
				GLUtils::Pipeline pipeline;
				RETURN_ON_ERROR_CODE(pipeline.init(shaderPaths[i]));
				RETURN_ON_ERROR_CODE(programs[i].init(pipeline));
			}
			return EC::ErrorCode();
		}

		template<typename T, typename... Ts>
		T create(Ts... args) {
			const GLUtils::Program& p = programs[shaderIndex<T>()];
			return T(p, args...);
		}

		void freeMem() {
			for (auto& p : programs) {
				p.freeMem();
			}
		}
	private:
		template<typename T>
		constexpr int shaderIndex() const;

		template<>
		constexpr int shaderIndex<FlatColor>() const {
			return int(ShaderTable::FlatColor);
		}
		 
		template<>
		constexpr int shaderIndex<Gradient2D>() const {
			return int(ShaderTable::Gradient2D);
		}

		std::array<GLUtils::Program, int(ShaderTable::Count)> programs;
	};
}