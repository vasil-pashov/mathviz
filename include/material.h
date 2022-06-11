#pragma once
#include <unordered_map>
#include "glutils.h"
namespace MathViz {
	class ShaderManager {
	private:
		std::unordered_map<std::string, GLUtils::Program>
	};

	class Material {
		friend class Context;
	private:
		static ShaderManager* shanderManager;
	};

	class FlatColorMaterial {
	private:
		glm::vec3 color;
	};
}