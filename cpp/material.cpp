#pragma once
#include <unordered_map>
#include "glutils.h"
#include "material.h"
#include "shader_bindings.h"

namespace MathViz {
	FlatColor::FlatColor() : 
		color{0.0f, 0.0f ,0.0f}
	{ }

	FlatColor::FlatColor(const glm::vec3& col) : 
		color{col}
	{ }

	void FlatColor::setColor(const glm::vec3& col) {
		this->color = col;
	}

	glm::vec3 FlatColor::getColor() const {
		return color;
	}

	FlatColor::ShaderId FlatColor::getShaderId() const {
		return ShaderId(ShaderTable::FlatColor);
	}

	EC::ErrorCode FlatColor::apply(const GLUtils::Program& p) const {
		RETURN_ON_ERROR_CODE(p.setUniform("color", color));
		return EC::ErrorCode();
	}
}