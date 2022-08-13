#pragma once
#include <unordered_map>
#include <glm/glm.hpp>

namespace GLUtils {
	class Program;
}

namespace MathViz {
	class IMaterial {
	public:
		using ShaderId = int;
		virtual ~IMaterial() {
		}
		virtual ShaderId getShaderId() const = 0;
		virtual EC::ErrorCode setUniforms(const GLUtils::Program&) const = 0;
	};

	class FlatColor : public IMaterial {
	public:
		FlatColor();
		explicit FlatColor(const glm::vec3& color);
		void setColor(const glm::vec3& color);
		glm::vec3 getColor() const;
		ShaderId getShaderId() const override;
		EC::ErrorCode setUniforms(const GLUtils::Program&) const override;
	private:
		glm::vec3 color;
	};

	class Gradient2D : public IMaterial {
	public:
		Gradient2D();
		Gradient2D(
			const glm::vec3& start,
			const glm::vec3& end,
			const glm::vec3& col0rStart,
			const glm::vec3& col0rEnd
		);
		ShaderId getShaderId() const override;
		EC::ErrorCode setUniforms(const GLUtils::Program&) const override;
	private:
		glm::vec3 start;
		glm::vec3 end;
		glm::vec3 colorStart;
		glm::vec3 colorEnd;
	};
}