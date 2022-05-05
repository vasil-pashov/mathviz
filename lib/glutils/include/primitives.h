#pragma once
#include "glm/vec3.hpp"
#include "glutils.h"
#include "error_code.h"

namespace EC {
	class ErrorCode;
}

namespace GLUtils {
	class Line {
	public:
		Line(glm::vec3 start, glm::vec3 end, glm::vec3 color, int width);
		EC::ErrorCode upload();
		EC::ErrorCode draw();
	private:
		glm::vec3 start;
		glm::vec3 end;
		glm::vec3 color;
		Buffer vertexBuffer;
		VAO vao;
		int width;
	};
}