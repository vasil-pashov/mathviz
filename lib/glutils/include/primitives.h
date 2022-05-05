#pragma once
#include "glm/vec3.hpp"
#include "glutils.h"
#include "error_code.h"
#include <functional>

namespace EC {
	class ErrorCode;
}

namespace GLUtils {
	class Line {
	public:
		Line() : start{0.0f, 0.0f, 0.0f}, end{0.0f, 0.0f, 0.0f}, color{0.0f, 0.0f, 0.0f}, width(0) {}
		EC::ErrorCode init(glm::vec3 start, glm::vec3 end, glm::vec3 color, int width);
		EC::ErrorCode upload();
		EC::ErrorCode draw(const Program& p);
		glm::vec3 getColor() const;
	private:
		glm::vec3 start;
		glm::vec3 end;
		glm::vec3 color;
		Buffer vertexBuffer;
		VAO vao;
		int width;
	};

	class Plot2D {
	public:
		Plot2D() : color{0.0f, 0.0f, 0.0f}, from(0), to(0), n(0), width(0) {}
		template<typename FuncT>
		EC::ErrorCode init(FuncT&& f, glm::vec3 color, float from, float to, int n, int width = 1) {
			this->f = std::forward<FuncT>(f);
			this->color = color;
			this->from = from;
			this->to = to;
			this->n = n;
			this->width = width;

			GLUtils::BufferLayout l;
			l.addAttribute(GLUtils::VertexType::Float, 3);

			RETURN_ON_ERROR_CODE(vertexBuffer.init(GLUtils::BufferType::Vertex));
			RETURN_ON_ERROR_CODE(vao.init());
			RETURN_ON_ERROR_CODE(vao.bind());
			RETURN_ON_ERROR_CODE(vertexBuffer.bind());
			RETURN_ON_ERROR_CODE(vertexBuffer.setLayout(l));
			RETURN_ON_ERROR_CODE(vertexBuffer.unbind());
			vao.unbind();
			return EC::ErrorCode();
		}
		EC::ErrorCode upload();
		EC::ErrorCode draw(const Program& p);
	private:
		std::function<float(float)> f;
		glm::vec3 color;
		Buffer vertexBuffer;
		VAO vao;
		int width;
		float from;
		float to;
		int n;
	};
}