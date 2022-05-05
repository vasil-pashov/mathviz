#include "primitives.h"
#include "error_code.h"
#include <array>

namespace GLUtils {
	Line::Line(glm::vec3 start, glm::vec3 end, glm::vec3 color, int width) :
		start(start), end(end), color(color), width(width)
	{ }

	EC::ErrorCode Line::upload() {
		const std::array<float, 12> data = {
			start.x, start.y, start.z,
			color.x, color.y, color.z,
			end.x, end.y, end.z,
			color.x, color.y, color.z
		};
		vertexBuffer.init(GLUtils::BufferType::Vertex);
		vertexBuffer.upload(sizeof(data), (void*)data.data());

		GLUtils::BufferLayout l;
		l.addAttribute(GLUtils::VertexType::Float, 3);
		l.addAttribute(GLUtils::VertexType::Float, 3);

		vao.init();
		vao.bind();
		vertexBuffer.bind();
		vertexBuffer.setLayout(l);
		vertexBuffer.unbind();
		vao.unbind();
		return EC::ErrorCode();
	}

	EC::ErrorCode Line::draw() {
		vao.bind();
		RETURN_ON_GL_ERROR(glLineWidth(width));
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINES, 0, 2));
		return EC::ErrorCode();
	}
}