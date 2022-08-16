#pragma once
#include <unordered_map>
#include "glutils.h"
#include "material.h"
#include "shader_bindings.h"

namespace MathViz {
	FlatColor::FlatColor(const GLUtils::Program& p) :
		FlatColor(p, {0.0f, 0.0f ,0.0f})
	{ }

	FlatColor::FlatColor(const GLUtils::Program& p, const glm::vec3& col) :
		IMaterial(p),
		color{col}
	{ }

	void FlatColor::setColor(const glm::vec3& col) {
		this->color = col;
	}

	glm::vec3 FlatColor::getColor() const {
		return color;
	}

	EC::ErrorCode FlatColor::setUniforms() const {
		RETURN_ON_ERROR_CODE(program.setUniform("color", color));
		return EC::ErrorCode();
	}

	Gradient2D::Gradient2D(const GLUtils::Program& p) :
		Gradient2D(
			p,
			{0.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 0.0f}
		)
	{ }

	Gradient2D::Gradient2D(
		const GLUtils::Program& p,
		const glm::vec3& start,
		const glm::vec3& end,
		const glm::vec3& colorStart,
		const glm::vec3& colorEnd
	) :
		IMaterial(p),
		start(start),
		end(end),
		colorStart(colorStart),
		colorEnd(colorEnd)
	{ }

	EC::ErrorCode Gradient2D::setUniforms() const {
		RETURN_ON_ERROR_CODE(program.setUniform("start", start));
		RETURN_ON_ERROR_CODE(program.setUniform("end", end));
		RETURN_ON_ERROR_CODE(program.setUniform("colorStart", colorStart));
		RETURN_ON_ERROR_CODE(program.setUniform("colorEnd", colorEnd));
		return EC::ErrorCode();
	}
}