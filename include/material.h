#pragma once
#include <unordered_map>
#include <glm/glm.hpp>

namespace MathViz {
	class IMaterial {
	public:
		using ShaderId = int;
		virtual ~IMaterial() {
		}
		virtual ShaderId getShaderId() const = 0;
	};

	class FlatColor : public IMaterial {
	public:
		FlatColor();
		explicit FlatColor(const glm::vec3& color);
		void setColor(const glm::vec3& color);
		glm::vec3 getColor() const;
		ShaderId getShaderId() const override;
	private:
		glm::vec3 color;
	};
}