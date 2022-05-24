#pragma once
#include "glm/vec3.hpp"
#include "glutils.h"
#include "error_code.h"
#include <functional>

namespace EC {
	class ErrorCode;
}

namespace GLUtils {
	/// @brief A straight line
	class Line {
	public:
		Line() : start{0.0f, 0.0f, 0.0f}, end{0.0f, 0.0f, 0.0f}, width(0) {}
		/// @brief Inititialize the line
		/// @param start The start of the line in world space
		/// @param end The end of the line in world space
		/// @param width The width of the line in pixel. Fractional values are supported for
		/// antialiased lines only. In case of fractional value without antialiasing the width
		/// will be rounded.
		EC::ErrorCode init(const glm::vec3& start, const glm::vec3& end, float width);
		/// @brief Upload the geometry to the GPU. Must be called after init.
		EC::ErrorCode upload();
		/// @brief Issue a draw call. Must be called only after init and upload are called. 
		EC::ErrorCode draw() const;
	private:
		/// The start of the line in world space
		glm::vec3 start;
		/// The end of the line in world space
		glm::vec3 end;
		Buffer vertexBuffer;
		VAO vao;
		float width;
	};

	/// @brief Create a curve following a 2D plot.
	/// Each x coordinate in world space will corelate to a y coordinate in world space.
	class Plot2D {
	public:
		Plot2D() : from(0), to(0), n(0), width(0) {}
		/// @brief Initialize the curve
		/// @tparam FuncT Type of the fuctor which will eval the function. It must accept one float and
		/// return a float.
		/// @param f The function which will be plotted.
		/// @param from The minimal x value of the function in world space.
		/// @param to The maximal x value of the function in world space.
		/// @param dx The curve is drawn by connecting linear pieces. The dx value dictates
		/// the space between two ends of the line (in world space).
		/// @param width The width of the line in pixel. Fractional values are supported for
		/// antialiased lines only. In case of fractional value without antialiasing the width
		/// will be rounded.
		template<typename FuncT>
		EC::ErrorCode init(FuncT&& f, float from, float to, float width, int n) {
			if (from > to) {
				std::swap(from, to);
			}
			this->f = std::forward<FuncT>(f);
			this->from = from;
			this->to = to;
			this->n = n;
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
		EC::ErrorCode upload();
		EC::ErrorCode draw() const;
	private:
		std::function<float(float)> f;
		Buffer vertexBuffer;
		VAO vao;
		float width;
		float from;
		float to;
		int n;
	};

	class Canvas {
	public:
		Canvas() : lowLeft{0, 0, 0}, upRight{0, 0, 0} {}
		EC::ErrorCode init(const glm::vec3& lowLeft, const glm::vec3& upRight);
		EC::ErrorCode upload();
		EC::ErrorCode draw() const;
	private:
		glm::vec3 lowLeft;
		glm::vec3 upRight;
		Buffer vertexBuffer;
		VAO vao;
	};

}