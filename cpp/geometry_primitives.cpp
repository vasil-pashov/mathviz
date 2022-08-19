#include "geometry_primitives.h"
#include "error_code.h"
#include "context.h"
#include "material.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>

extern 	MathViz::Context ctx;

namespace MathViz {

	Line::Line() :
		start{0.0f, 0.0f, 0.0f},
		end{0.0f, 0.0f, 0.0f},
		width(0)
	{ }

	Line::Line(Line&& other) noexcept :
		start(other.start),
		end(other.end),
		vertexBuffer(std::move(other.vertexBuffer)),
		vao(std::move(vao)),
		width(other.width) {

	}

	Line& Line::operator=(Line&& other) noexcept {
		vertexBuffer = std::move(other.vertexBuffer);
		vao = std::move(other.vao);
		width = other.width;
		start = other.start;
		end = other.end;
		return *this;
	}

	Line::~Line() {
		freeMem();
	}

	EC::ErrorCode Line::init(const glm::vec3& start, const glm::vec3& end, float width) {
		this->start = start;
		this->end = end;
		this->width = width;

		const std::array<float, 6> data = {
			start.x, start.y, start.z,
			end.x, end.y, end.z
		};

		GLUtils::BufferLayout layout;
		layout.addAttribute(GLUtils::VertexType::Float, 3);

		RETURN_ON_ERROR_CODE(vao.init());
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.init(sizeof(data), (void*)data.data()), layout);
		RETURN_ON_ERROR_CODE(vertexBuffer.setLayout(layout));
		RETURN_ON_ERROR_CODE(vao.unbind());
		return EC::ErrorCode();
	}

	EC::ErrorCode Line::draw() const {
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_GL_ERROR(glLineWidth(width));
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINES, 0, 2));
		RETURN_ON_ERROR_CODE(vao.unbind());
		return EC::ErrorCode();
	}

	void Line::freeMem() {
		vertexBuffer.freeMem();
		vao.freeMem();
	}

	Axes::Axes() :
		xRange{0, 0},
		yRange{0, 0},
		lineVertexCount{0}
	{
	}
	EC::ErrorCode Axes::init(
		const Range2D& xRangeIn,
		const Range2D& yRangeIn,
		float markDh
	) {
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
		RETURN_ON_ERROR_CODE(vao.init());
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.init(byteSize, lineVertices.data(), l));
		RETURN_ON_ERROR_CODE(vao.unbind());
		return EC::ErrorCode();
	}

	EC::ErrorCode Axes::draw() const {
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINES, 0, lineVertexCount));
		RETURN_ON_ERROR_CODE(vao.unbind());
		return EC::ErrorCode();
	}

	Plot2D::Plot2D() :
		xRange{0, 0},
		yRange{0, 0},
		lineWidth{1.0f},
		n{0} {}

	EC::ErrorCode Plot2D::upload() {
		RETURN_ON_ERROR_CODE(vertexBuffer.bind());
		void* mapped;
		RETURN_ON_ERROR_CODE(vertexBuffer.map(mapped, GLUtils::BufferAccessType::Write));
		
		float maxHeight = f(xRange.from);
		float minHeight = maxHeight;
		const float dh = std::abs(xRange.getLength()) / (n-1);
		for (int i = 0; i < n; ++i) {
			const float x = xRange.from + i * dh;
			const float y = f(x);
			maxHeight = std::max(maxHeight, y);
			minHeight = std::min(minHeight, y);
			const glm::vec3 point(xRange.from + i * dh, y, 0.0f);
			static_cast<glm::vec3*>(mapped)[i] = point;
		}
		RETURN_ON_ERROR_CODE(vertexBuffer.unmap());
		RETURN_ON_ERROR_CODE(vertexBuffer.unbind());
		return EC::ErrorCode();
	}

	EC::ErrorCode Plot2D::draw() const {
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_GL_ERROR(glLineWidth(lineWidth));
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINE_STRIP, 0, n));
		RETURN_ON_ERROR_CODE(vao.unbind());
		return EC::ErrorCode();
	}

	ReimanArea::ReimanArea() :
		vertexCount(0),
		barCount(0)
	{}

	EC::ErrorCode ReimanArea::draw() const {
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_GL_ERROR(glDrawArrays(GL_TRIANGLES, 0, vertexCount));
		RETURN_ON_ERROR_CODE(vao.unbind());
		return EC::ErrorCode();
	}

	EC::ErrorCode ReimanArea::outline(const IMaterial& m, const IMaterial& om) const {
		RETURN_ON_ERROR_CODE(vao.bind());
		
		RETURN_ON_GL_ERROR(glDrawArrays(GL_TRIANGLES, 0, vertexCount));
		
		RETURN_ON_GL_ERROR(glDisable(GL_DEPTH_TEST));
		
		const GLUtils::Program& p = om.getProgram();
		RETURN_ON_ERROR_CODE(p.bind());
		RETURN_ON_ERROR_CODE(om.setUniforms());
		
		const glm::mat4 ortho = glm::ortho(-5.f, 5.f, -5.f, 5.f, -5.f, 5.f);
		const glm::mat4 perspective = glm::perspective(glm::radians(60.f), float(800) / float(900), -1.f, 10.f);
		
		const glm::vec3 cameraPos(0.0f, 0.0f, -1.0f);
		const glm::vec3 lookAtPoint(0.0f, 0.0f, 0.0f);
		const glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
		const glm::mat4 view = glm::lookAt(cameraPos, lookAtPoint, cameraUp);
		RETURN_ON_ERROR_CODE(p.setUniform("projection", ortho, false));
		RETURN_ON_ERROR_CODE(p.setUniform("view", view, false));
		
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINES, vertexCount, barCount * 8));
		
		RETURN_ON_GL_ERROR(glEnable(GL_DEPTH_TEST));
		RETURN_ON_ERROR_CODE(vao.unbind());
		return EC::ErrorCode();
	}

	Canvas::Canvas() :
		lowLeft{0, 0, 0},
		upRight{0, 0, 0}
	{ }

	EC::ErrorCode Canvas::init(const glm::vec3& lowLeft, const glm::vec3& upRight) {
		this->lowLeft = lowLeft;
		this->upRight = upRight;

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

		GLUtils::BufferLayout layout;
		layout.addAttribute(GLUtils::VertexType::Float, 3);
		layout.addAttribute(GLUtils::VertexType::Float, 2);

		RETURN_ON_ERROR_CODE(vao.init());
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.init(sizeof(data), (void*)data, layout));
		RETURN_ON_ERROR_CODE(vao.unbind());

		return EC::ErrorCode();
	}

	EC::ErrorCode Canvas::draw() const {
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_GL_ERROR(glDrawArrays(GL_TRIANGLES, 0, 6));
		RETURN_ON_ERROR_CODE(vao.unbind());
		return EC::ErrorCode();
	}

	glm::vec3 circleEquation(float t) {
		t *= 2 * PI;
		return {std::cos(t), std::sin(t), 0.0f};
	}

	glm::vec3 squareEquation(float t) {
		float x = 0, y = 0;
		if (t < 0.25) {
			x = -0.5 + t * 4;
			y = -0.5;
		} else if (0.25 <= t && t < 0.5) {
			x = 0.5;
			y = -0.5 + (t - 0.25) * 4;
		} else if (0.5 <= t && t < 0.75) {
			x = 0.5 - (t - 0.5) * 4;
			y = 0.5;
		} else if (0.75f <= t && t <= 1) {
			x = -0.5;
			y = 0.5 - (t - 0.75f) * 4;
		} else {
			assert(false);
		}
		return {x, y, 0.0f};
	}

	Morph2D::Morph2D(Morph2D&& other) noexcept :
		vao(std::move(other.vao)),
		vertexBuffer(std::move(other.vertexBuffer)),
		vertexCount(other.vertexCount)
	{ }

	Morph2D& Morph2D::operator=(Morph2D&& other) noexcept {
		freeMem();
		vao = std::move(other.vao);
		vertexBuffer = std::move(other.vertexBuffer);
		vertexCount = other.vertexCount;
		return *this;
	}

	EC::ErrorCode Morph2D::init(const Morphable2D& start, const Morphable2D& end) {
		vertexCount = std::max(start.getVertexCount(), end.getVertexCount());
		std::vector<glm::vec3> data(vertexCount * 2);
		const int startVerts = start.getVertexCount();
		const int endVerts = end.getVertexCount();
		for (int i = 0, idx = 0; i < vertexCount; ++i, idx += 2) {
			data[idx] = start.getVertices()[i];
			data[idx + 1] = end.getVertices()[i];
		}


		GLUtils::BufferLayout layout;
		layout.addAttribute(GLUtils::VertexType::Float, 3);
		layout.addAttribute(GLUtils::VertexType::Float, 3);

		const int64_t dataByteSize = data.size() * sizeof(glm::vec3);

		RETURN_ON_ERROR_CODE(vao.init());
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_ERROR_CODE(vertexBuffer.init(dataByteSize, (void*)data.data(), layout));
		RETURN_ON_ERROR_CODE(vao.unbind());

		return EC::ErrorCode();
	}

	void Morph2D::freeMem() {
		vao.freeMem();
		vertexBuffer.freeMem();
	}

	EC::ErrorCode Morph2D::draw() const {
		RETURN_ON_ERROR_CODE(vao.bind());
		RETURN_ON_GL_ERROR(glLineWidth(2));
		RETURN_ON_GL_ERROR(glDrawArrays(GL_LINE_STRIP, 0, vertexCount));
		RETURN_ON_ERROR_CODE(vao.unbind());
		return EC::ErrorCode();
	}
}