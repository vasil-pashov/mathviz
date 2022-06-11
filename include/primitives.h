#pragma once
#include "glm/vec3.hpp"
#include "glutils.h"
#include "error_code.h"
#include <array>
#include <functional>

namespace EC {
	class ErrorCode;
}

namespace MathViz {

	struct Range2D {
		Range2D() : from(0), to(0) {}
		Range2D(float from, float to) : from(from), to(to) {
			assert(from <= to);
		}
		float getLength() const {
			return to - from;
		}
		bool contains(float x) const {
			return x >= from && x <= to;
		}
		float getMid() const {
			return (to - from) / 2;
		}
		float from;
		float to;
	};

	constexpr float PI = 3.141592653589793f;

	/// @brief A straight line
	class Line {
	public:
		Line() : start{0.0f, 0.0f, 0.0f}, end{0.0f, 0.0f, 0.0f}, width(0) {}
		Line(Line&& other) noexcept :
			start(other.start),
			end(other.end),
			vertexBuffer(std::move(other.vertexBuffer)),
			vao(std::move(vao)),
			width(other.width)
		{

		}

		Line& operator=(Line&& other) noexcept {
			vertexBuffer = std::move(other.vertexBuffer);
			vao = std::move(other.vao);
			width = other.width;
			start = other.start;
			end = other.end;
			return *this;
		}
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
		void freeMem() {
			vertexBuffer.freeMem();
			vao.freeMem();
		}
	private:
		/// The start of the line in world space
		glm::vec3 start;
		/// The end of the line in world space
		glm::vec3 end;
		GLUtils::Buffer vertexBuffer;
		GLUtils::VAO vao;
		float width;
	};

	class Axes {
	public:
		Axes() : xRange{0, 0}, yRange{0, 0} {}
		EC::ErrorCode init(const Range2D& xRangeIn, const Range2D& yRangeIn, float markDh) {
			xRange = xRangeIn;
			yRange = yRangeIn;
			const int axisCount = 2;
			const int xMarksCount = int(xRange.getLength() / markDh);
			const int yMarksCount = int(yRange.getLength() / markDh);
			const int totalLinesCount = xMarksCount + yMarksCount + axisCount;
			const int totalVertices = totalLinesCount * 2;
			const float z = 0.0f;
			std::vector<glm::vec3> lineVertices;
			lineVertices.reserve(totalVertices);
			// x axis
			const float xAxisYCoordinate = yRange.contains(0.0f) ? 0.0f : yRange.getMid();
			lineVertices.emplace_back(xRange.from, xAxisYCoordinate, z);
			lineVertices.emplace_back(xRange.to, xAxisYCoordinate, z);
			// y axis
			const float yAxisXCoordinate = xRange.contains(0.0f) ? 0.0f : xRange.getMid();
			lineVertices.emplace_back(yAxisXCoordinate, yRange.from, z);
			lineVertices.emplace_back(yAxisXCoordinate, yRange.to, z);

			const float markHalfLength = 0.1f;
			// x axis marks
			const float xMarkStart = int(xRange.from / markDh) * markDh;
			for (float xMarkPos = xMarkStart; xMarkPos < xRange.to; xMarkPos += markDh) {
				lineVertices.emplace_back(xMarkPos, xAxisYCoordinate - markHalfLength, z);
				lineVertices.emplace_back(xMarkPos, xAxisYCoordinate + markHalfLength, z);
			}
			// y axis marks
			const float yMarkStart = int(yRange.from / markDh) * markDh;
			for (float yMarkPos = yMarkStart; yMarkPos < yRange.to; yMarkPos += markDh) {
				lineVertices.emplace_back(yAxisXCoordinate - markHalfLength, yMarkPos, z);
				lineVertices.emplace_back(yAxisXCoordinate + markHalfLength, yMarkPos, z);
			}
			assert(lineVertexCount == lineVertices.size());
			lineVertexCount = lineVertices.size();
			const int64_t byteSize = lineVertices.size() * sizeof(lineVertices[0]);
			GLUtils::BufferLayout l;
			l.addAttribute(GLUtils::VertexType::Float, 3);
			RETURN_ON_ERROR_CODE(vertexBuffer.init(GLUtils::BufferType::Vertex));
			RETURN_ON_ERROR_CODE(vertexBuffer.bind());
			RETURN_ON_ERROR_CODE(vao.init());
			RETURN_ON_ERROR_CODE(vao.bind());
			RETURN_ON_ERROR_CODE(vertexBuffer.setLayout(l));
			RETURN_ON_ERROR_CODE(vertexBuffer.upload(byteSize, lineVertices.data()));
			RETURN_ON_ERROR_CODE(vertexBuffer.unbind());
			RETURN_ON_ERROR_CODE(vao.unbind());
			return EC::ErrorCode();
		}
		EC::ErrorCode draw() const {
			RETURN_ON_ERROR_CODE(vao.bind());
			RETURN_ON_GL_ERROR(glDrawArrays(GL_LINES, 0, lineVertexCount));
			RETURN_ON_ERROR_CODE(vao.unbind());
			return EC::ErrorCode();
		}
	private:
		Range2D xRange;
		Range2D yRange;
		GLUtils::VAO vao;
		GLUtils::Buffer vertexBuffer;
		/// Used by the draw call. The number of vertices (not lines)
		/// which will be used to draw the lines
		int lineVertexCount;
	};

	/// @brief Create a curve following a 2D plot.
	/// Each x coordinate in world space will corelate to a y coordinate in world space.
	class Plot2D {
	public:
		Plot2D() : xRange{ 0, 0 }, yRange{ 0, 0 }, lineWidth{ 1.0f }, n{ 0 } {}
		/// @brief Initialize the curve
		/// @tparam FuncT Type of the fuctor which will eval the function. It must accept one float and
		/// return a float.
		/// @param f The function which will be plotted.
		/// @param from The minimal x value of the function in world space.
		/// @param to The maximal x value of the function in world space.
		/// @param lineWidth The width of the line in pixel. Fractional values are supported for
		/// antialiased lines only. In case of fractional value without antialiasing the width
		/// will be rounded.
		/// @param n Number of points where the plot will be evaluated. The larger this value
		/// the smoother the plot
		template<typename FuncT>
		EC::ErrorCode init(
			FuncT&& f,
			const Range2D& xRange,
			const Range2D& yRange,
			float lineWidth,
			int n
		) {
			this->f = std::forward<FuncT>(f);
			this->xRange = xRange;
			this->yRange = yRange;
			this->lineWidth = lineWidth;
			this->n = n;

			GLUtils::BufferLayout layout;
			layout.addAttribute(GLUtils::VertexType::Float, 3);

			RETURN_ON_ERROR_CODE(plotVertexBuffer.init(GLUtils::BufferType::Vertex));
			RETURN_ON_ERROR_CODE(plotVAO.init());
			RETURN_ON_ERROR_CODE(plotVAO.bind());
			RETURN_ON_ERROR_CODE(plotVertexBuffer.bind());
			RETURN_ON_ERROR_CODE(plotVertexBuffer.setLayout(layout));
			RETURN_ON_ERROR_CODE(plotVertexBuffer.unbind());
			RETURN_ON_ERROR_CODE(plotVAO.unbind());

			return EC::ErrorCode();
		}
		EC::ErrorCode upload();
		EC::ErrorCode draw() const;
	private:
		std::function<float(float)> f;
		GLUtils::Buffer plotVertexBuffer;
		GLUtils::VAO plotVAO;
		/// min and max x coordinate to show on the plot
		/// points which are outside of the range will not be computed
		Range2D xRange;
		/// min and max y coordinate to show on the plot
		Range2D yRange;
		float lineWidth;
		int n;
	};

	class ReimanArea {
	public:
		ReimanArea() : vertexCount(0) {}
		template<typename FuncT>
		EC::ErrorCode init(FuncT&& f, const Range2D& xRange, float dh) {
			const int barsCount = int(xRange.getLength() / dh);
			const float z = 0.0f;
			std::vector<glm::vec3> vertices;
			// Each bar is represented by 2 triangles
			// each triangle has 3 verts
			vertices.reserve(barsCount * 6);
			float barStart = xRange.from;
			for (int i = 0; i < barsCount; ++i) {
				const float barMid = barStart + dh / 2;
				const float barEnd = barStart + dh;
				const float fAtBarCenter = f(barMid);
				const float barBottom = fAtBarCenter > 0 ? 0.0f : fAtBarCenter;
				const float barTop = fAtBarCenter > 0 ? fAtBarCenter : 0.0f;

				vertices.emplace_back(barEnd, barTop, z);
				vertices.emplace_back(barStart, barTop, z);
				vertices.emplace_back(barStart, barBottom, z);

				vertices.emplace_back(barEnd, barTop, z);
				vertices.emplace_back(barStart, barBottom, z);
				vertices.emplace_back(barEnd, barBottom, z);

				barStart += dh;
			}
			vertexCount = vertices.size();
			GLUtils::BufferLayout l;
			l.addAttribute(GLUtils::VertexType::Float, 3);
			const int64_t byteSize = vertices.size() * sizeof(vertices[0]);
			RETURN_ON_ERROR_CODE(vertexBuffer.init(GLUtils::BufferType::Vertex));
			RETURN_ON_ERROR_CODE(vertexBuffer.bind());
			RETURN_ON_ERROR_CODE(vao.init());
			RETURN_ON_ERROR_CODE(vao.bind());
			RETURN_ON_ERROR_CODE(vertexBuffer.setLayout(l));
			RETURN_ON_ERROR_CODE(vertexBuffer.upload(byteSize, vertices.data()));
			RETURN_ON_ERROR_CODE(vertexBuffer.unbind());
			RETURN_ON_ERROR_CODE(vao.unbind());
			return EC::ErrorCode();
		}

		EC::ErrorCode draw() const {
			RETURN_ON_ERROR_CODE(vao.bind());
			RETURN_ON_GL_ERROR(glDrawArrays(GL_TRIANGLES, 0, vertexCount));
			RETURN_ON_ERROR_CODE(vao.unbind());
			return EC::ErrorCode();
		}
	private:
		GLUtils::VAO vao;
		GLUtils::Buffer vertexBuffer;
		int vertexCount;
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
		GLUtils::Buffer vertexBuffer;
		GLUtils::VAO vao;
	};

	class Morphable2D {
	public:
		virtual int getVertexCount() const = 0;
		virtual const glm::vec3* getVertices() const = 0;
		virtual glm::vec3 getClosestVertex(const glm::vec3&) const = 0;
	};

	class Morph2D {
	public:
		Morph2D() = default;
		Morph2D(const Morph2D&) = delete;
		Morph2D& operator=(const Morph2D&) = delete;
		Morph2D(Morph2D&& other) noexcept : 
			vao(std::move(other.vao)),
			vertexBuffer(std::move(other.vertexBuffer)),
			vertexCount(other.vertexCount)
		{ }
		Morph2D& operator=(Morph2D&& other) noexcept {
			freeMem();
			vao = std::move(other.vao);
			vertexBuffer = std::move(other.vertexBuffer);
			vertexCount = other.vertexCount;
		}

		EC::ErrorCode init(const Morphable2D& start, const Morphable2D& end) {
			vertexCount = std::max(start.getVertexCount(), end.getVertexCount());
			std::vector<glm::vec3> data(vertexCount * 2);
			const int startVerts = start.getVertexCount();
			const int endVerts = end.getVertexCount();
			for (int i = 0, idx = 0; i < vertexCount; ++i, idx += 2) {
				if (startVerts > endVerts) {
					const glm::vec3& s = start.getVertices()[i];
					data[idx] = s;
					data[idx + 1] = end.getClosestVertex(s);
				} else if (endVerts > startVerts) {
					const glm::vec3& currentVert = end.getVertices()[i];
					data[idx] = start.getClosestVertex(currentVert);
					data[idx + 1] = currentVert;
				} else {
					data[idx] = start.getVertices()[i];
					data[idx + 1] = end.getVertices()[i];
				}
			}


			GLUtils::BufferLayout layout;
			layout.addAttribute(GLUtils::VertexType::Float, 3);
			layout.addAttribute(GLUtils::VertexType::Float, 3);

			RETURN_ON_ERROR_CODE(vertexBuffer.init(GLUtils::BufferType::Vertex));
			RETURN_ON_ERROR_CODE(vao.init());
			RETURN_ON_ERROR_CODE(vao.bind());
			RETURN_ON_ERROR_CODE(vertexBuffer.bind());
			RETURN_ON_ERROR_CODE(vertexBuffer.setLayout(layout));
			RETURN_ON_ERROR_CODE(vertexBuffer.unbind());
			RETURN_ON_ERROR_CODE(vao.unbind());
			RETURN_ON_ERROR_CODE(vertexBuffer.bind());
			const int64_t dataByteSize = data.size() * sizeof(glm::vec3);
			RETURN_ON_ERROR_CODE(vertexBuffer.upload(dataByteSize, (void*)data.data()));
			RETURN_ON_ERROR_CODE(vertexBuffer.unbind());

			return EC::ErrorCode();
		}

		void freeMem() {
			vao.freeMem();
			vertexBuffer.freeMem();
		}

		EC::ErrorCode draw() const {
			RETURN_ON_ERROR_CODE(vao.bind());
			RETURN_ON_GL_ERROR(glLineWidth(2));
			RETURN_ON_GL_ERROR(glDrawArrays(GL_LINE_STRIP, 0, vertexCount));
			RETURN_ON_ERROR_CODE(vao.unbind());
			return EC::ErrorCode();
		}
	private:
		GLUtils::VAO vao;
		GLUtils::Buffer vertexBuffer;
		int vertexCount;
	};

	class Circle : public Morphable2D {
	public:
		Circle(int numVerts, float radius) : vertices(numVerts + 1), radius(radius) {
			const float dt = 2 * PI / numVerts;
			float t = 0;
			for (int i = 0; i < numVerts; ++i) {
				vertices[i] = glm::vec3(radius * std::cos(t), radius * std::sin(t), 1);
				t += dt;
			}
			vertices[numVerts] = vertices[0];
		}
		int getVertexCount() const override {
			return vertices.size();
		}

		glm::vec3 getClosestVertex(const glm::vec3& point) const override {
			int closestIndex = 0;
			float minDist = glm::distance(point, vertices[0]);
			for (int i = 1; i < vertices.size(); ++i) {
				const float dist = glm::distance(point, vertices[i]);
				if (dist < minDist) {
					minDist = dist;
					closestIndex = i;
				}
			}
			return vertices[closestIndex];
		}

		const glm::vec3* getVertices() const override {
			return vertices.data();
		}
	private:
		std::vector<glm::vec3> vertices;
		float radius;
	};

	class Rectangle : public Morphable2D {
	public:
		Rectangle(const glm::vec3& lowLeft, const glm::vec3& upRight) {
			assert(lowLeft.z == upRight.z);
			vertices[0] = lowLeft;
			vertices[1] = glm::vec3(upRight.x, lowLeft.y, lowLeft.z);
			vertices[2] = upRight;
			vertices[3] = glm::vec3(lowLeft.x, upRight.y, upRight.z);
		}

		int getVertexCount() const override {
			return vertices.size();
		}

		glm::vec3 getClosestVertex(const glm::vec3& point) const override {
			int closestIndex = 0;
			float minDist = glm::distance(point, vertices[0]);
			for (int i = 1; i < vertices.size(); ++i) {
				const float dist = glm::distance(point, vertices[i]);
				if (dist < minDist) {
					minDist = dist;
					closestIndex = i;
				}
			}
			return vertices[closestIndex];
		}

		const glm::vec3* getVertices() const override {
			return vertices.data();
		}
	private:
		std::array<glm::vec3, 4> vertices;
	};

}