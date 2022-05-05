#include "primitives.h"
#include "error_code.h"
#include <array>

namespace GLUtils {
	EC::ErrorCode Line::init(glm::vec3 start, glm::vec3 end, glm::vec3 color, int width = 1) {
		this->start = start;
		this->end = end;
		this->color = color;
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

	EC::ErrorCode Line::upload() {
		const std::array<float, 6> data = {
			start.x, start.y, start.z,
			end.x, end.y, end.z
		};
		RETURN_ON_ERROR_CODE(vertexBuffer.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.upload(sizeof(data), (void*)data.data()));
		RETURN_ON_ERROR_CODE(vertexBuffer.unbind());
		return EC::ErrorCode();
	}

	glm::vec3 Line::getColor() const {
		return color;
	}

	EC::ErrorCode Line::draw(const Program& p) {
		p.bind();
		p.setUniform("color", color);
		vao.bind();
		RETURN_ON_GL_ERROR(glLineWidth(width));
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINES, 0, 2));
		return EC::ErrorCode();
	}

	EC::ErrorCode Plot2D::upload() {
		RETURN_ON_ERROR_CODE(vertexBuffer.upload(sizeof(glm::vec3) * n, nullptr));
		void* mapped;
		RETURN_ON_ERROR_CODE(vertexBuffer.map(mapped, BufferAccessType::Write));
		
		const float dh = (to - from) / (n-1);
		for (int i = 0; i < n; ++i) {
			const glm::vec3 point(from + i * dh, f(from + i * dh), 0.0f);
			static_cast<glm::vec3*>(mapped)[i] = point;
		}
		RETURN_ON_ERROR_CODE(vertexBuffer.unmap());
		RETURN_ON_ERROR_CODE(vertexBuffer.unbind());
		return EC::ErrorCode();
	}

	EC::ErrorCode Plot2D::draw(const Program& p) {
		p.bind();
		p.setUniform("color", color);
		vao.bind();
		RETURN_ON_GL_ERROR(glLineWidth(width));
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINE_STRIP, 0, n));
		vao.unbind();
		p.unbind();
		return EC::ErrorCode();
	}
}