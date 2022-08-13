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

	EC::ErrorCode FlatColor::setUniforms(const GLUtils::Program& p) const {
		RETURN_ON_ERROR_CODE(p.setUniform("color", color));
		return EC::ErrorCode();
	}

	Gradient2D::Gradient2D() :
		start(0.0f, 0.0f, 0.0f),
		end(0.0f, 0.0f, 0.0f),
		colorStart(0.0f, 0.0f, 0.0f),
		colorEnd(0.0f, 0.0f, 0.0f)
	{ }

	Gradient2D::Gradient2D(
		const glm::vec3& start,
		const glm::vec3& end,
		const glm::vec3& colorStart,
		const glm::vec3& colorEnd
	) :
		start(start),
		end(end),
		colorStart(colorStart),
		colorEnd(colorEnd)
	{ }

	Gradient2D::ShaderId Gradient2D::getShaderId() const {
		return ShaderId(ShaderTable::Gradient2D);
	}

	EC::ErrorCode Gradient2D::setUniforms(const GLUtils::Program& p) const {
		RETURN_ON_ERROR_CODE(p.setUniform("start", start));
		RETURN_ON_ERROR_CODE(p.setUniform("end", end));
		RETURN_ON_ERROR_CODE(p.setUniform("colorStart", colorStart));
		RETURN_ON_ERROR_CODE(p.setUniform("colorEnd", colorEnd));
		return EC::ErrorCode();
	}
}