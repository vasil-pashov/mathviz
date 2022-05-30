#include "primitives.h"
#include "error_code.h"
#include <array>

namespace GLUtils {
	EC::ErrorCode Line::init(const glm::vec3& start, const glm::vec3& end, float width) {
		this->start = start;
		this->end = end;
		this->width = width;

		GLUtils::BufferLayout layout;
		layout.addAttribute(GLUtils::VertexType::Float, 3);

		RETURN_ON_ERROR_CODE(vertexBuffer.init(GLUtils::BufferType::Vertex));
		RETURN_ON_ERROR_CODE(vao.init());
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.setLayout(layout));
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

	EC::ErrorCode Line::draw() const {
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_GL_ERROR(glLineWidth(width));
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINES, 0, 2));
		vao.unbind();
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

	EC::ErrorCode Plot2D::draw() const {
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_GL_ERROR(glLineWidth(width));
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINE_STRIP, 0, n + 1));
		vao.unbind();
		return EC::ErrorCode();
	}

	EC::ErrorCode Canvas::init(const glm::vec3& lowLeft, const glm::vec3& upRight) {
		this->lowLeft = lowLeft;
		this->upRight = upRight;

		GLUtils::BufferLayout layout;
		layout.addAttribute(GLUtils::VertexType::Float, 3);
		layout.addAttribute(GLUtils::VertexType::Float, 2);

		RETURN_ON_ERROR_CODE(vertexBuffer.init(GLUtils::BufferType::Vertex));
		RETURN_ON_ERROR_CODE(vao.init());
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.setLayout(layout));
		RETURN_ON_ERROR_CODE(vertexBuffer.unbind());

		return EC::ErrorCode();
	}

	EC::ErrorCode Canvas::upload() {
		// (x1, y1)       (x0, y0)
		// (0, 1)         (1, 1)
		//      ***********
		//      *         *
		//      *         *
		//      ***********
		// (x2, y2)       (x3, y3)
		// (0, 0)         (1, 0)
		const glm::vec3 upLeft(lowLeft.x, upRight.y, upRight.z);
		const glm::vec3 lowRight(upRight.x, lowLeft.y, upRight.z);
		const float data[] = {
			upRight.x, upRight.y, upRight.z, 1, 1,
			upLeft.x, upLeft.y, upLeft.z, 0, 1,
			lowLeft.x, lowLeft.y, lowLeft.z, 0, 0,

			upRight.x, upRight.y, upRight.z, 1, 1,
			lowLeft.x, lowLeft.y, lowLeft.z, 0, 0,
			lowRight.x, lowRight.y, lowRight.z, 1, 0
		};
		RETURN_ON_ERROR_CODE(vertexBuffer.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.upload(sizeof(data), (void*)data));
		RETURN_ON_ERROR_CODE(vertexBuffer.unbind());
		return EC::ErrorCode();
	}

	EC::ErrorCode Canvas::draw() const {
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_GL_ERROR(glDrawArrays(GL_TRIANGLES, 0, 6));
		vao.unbind();
		return EC::ErrorCode();
	}
}