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

	struct IMaterial;

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

	class IGeometry {
	public:
		virtual ~IGeometry() {}
		virtual EC::ErrorCode draw() const = 0;
		virtual EC::ErrorCode outline(const IMaterial& objectMaterial, const IMaterial& outlineMaterial) const {
			return EC::ErrorCode("Not implemented");
		}
	};

	/// @brief A straight line
	class Line : public IGeometry {
	public:
		Line();
		Line(Line&& other) noexcept;
		Line& operator=(Line&& other) noexcept;
		~Line();
		/// @brief Inititialize the line
		/// @param start The start of the line in world space
		/// @param end The end of the line in world space
		/// @param width The width of the line in pixel. Fractional values are supported for
		/// antialiased lines only. In case of fractional value without antialiasing the width
		/// will be rounded.
		EC::ErrorCode init(const glm::vec3& start, const glm::vec3& end, float width);
		/// @brief Issue a draw call. Must be called only after init and upload are called. 
		EC::ErrorCode draw() const override;
		void freeMem();
	private:
		/// The start of the line in world space
		glm::vec3 start;
		/// The end of the line in world space
		glm::vec3 end;
		GLUtils::VertexBuffer vertexBuffer;
		GLUtils::VAO vao;
		float width;
	};

	class Axes : public IGeometry {
	public:
		Axes();
		EC::ErrorCode init(
			const Range2D& xRangeIn,
			const Range2D& yRangeIn,
			float markDh
		);
		EC::ErrorCode draw() const override;
	private:
		Range2D xRange;
		Range2D yRange;
		GLUtils::VAO vao;
		GLUtils::VertexBuffer vertexBuffer;
		/// Used by the draw call. The number of vertices (not lines)
		/// which will be used to draw the lines
		int lineVertexCount;
	};

	/// @brief Create a curve following a 2D plot.
	/// Each x coordinate in world space will corelate to a y coordinate in world space.
	class Plot2D : public IGeometry {
	public:
		Plot2D();
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

			RETURN_ON_ERROR_CODE(vao.init());
			RETURN_ON_ERROR_CODE(vao.bind());
			RETURN_ON_ERROR_CODE(vertexBuffer.init(n * sizeof(glm::vec3), nullptr, layout));
			RETURN_ON_ERROR_CODE(vao.unbind());

			RETURN_ON_ERROR_CODE(upload());

			return EC::ErrorCode();
		}
		EC::ErrorCode draw() const override;
		template<typename FuncT>
		EC::ErrorCode reset(FuncT&& f) {
			this->f = std::forward<FuncT>(f);
			// TODO:
			// this->xRange = xRange;
			// this->yRange = yRange;
			// this->lineWidth = lineWidth;
			// this->n = n;
			RETURN_ON_ERROR_CODE(upload());
			return EC::ErrorCode();
		}
		void setLineWidth(const float lineWidth);
	private:
		EC::ErrorCode upload();

		std::function<float(float)> f;
		GLUtils::VertexBuffer vertexBuffer;
		GLUtils::VAO vao;
		/// min and max x coordinate to show on the plot
		/// points which are outside of the range will not be computed
		Range2D xRange;
		/// min and max y coordinate to show on the plot
		Range2D yRange;
		float lineWidth;
		int n;
	};

	class ReimanArea : public IGeometry {
	public:
		ReimanArea();
		template<typename FuncT>
		EC::ErrorCode init(FuncT&& f, const Range2D& xRange, float dh) {
			barCount = int(xRange.getLength() / dh);
			const float z = 1.0f;
			std::vector<glm::vec3> vertices(barCount * 14);
			// Each bar is represented by 2 triangles each triangle has 3 verts
			// First 6 * barCount verts are the presented Reiman area
			// Second 6 * barCount verts are slightly scaled and used for outlining
			float barStart = xRange.from;
			int i = 0, j = 6 * barCount;
			for (int bar = 0; bar < barCount; ++bar) {
				const float barMid = barStart + dh / 2;
				const float barEnd = barStart + dh;
				const float fAtBarCenter = f(barMid);
				const float barBottom = fAtBarCenter > 0 ? 0.0f : fAtBarCenter;
				const float barTop = fAtBarCenter > 0 ? fAtBarCenter : 0.0f;

				vertices[i++] = glm::vec3(barEnd, barTop, z); // up right
				vertices[i++] = glm::vec3(barStart, barTop, z); // up left
				vertices[i++] = glm::vec3(barStart, barBottom, z); // bottom left

				vertices[i++] = glm::vec3(barEnd, barTop, z); // up right
				vertices[i++] = glm::vec3(barStart, barBottom, z); // bottom left
				vertices[i++] = glm::vec3(barEnd, barBottom, z); // bottom right

				// ===========================================================
				// ====================== OUTLINE ============================
				// ===========================================================
				// const float scale = dh * 0.1;
				// vertices[j++] = glm::vec3(barEnd + scale, barTop + scale, z); // up right
				// vertices[j++] = glm::vec3(barStart - scale, barTop + scale, z); // up left
				// vertices[j++] = glm::vec3(barStart - scale, barBottom - scale, z); // bottom left
				// 
				// vertices[j++] = glm::vec3(barEnd + scale, barTop + scale, z); // up right
				// vertices[j++] = glm::vec3(barStart - scale, barBottom - scale, z); // bottom left
				// vertices[j++] = glm::vec3(barEnd + scale, barBottom - scale, z); // bottom right


				vertices[j++] = glm::vec3(barEnd, barTop, z); // up right
				vertices[j++] = glm::vec3(barStart, barTop, z); // up left

				vertices[j++] = glm::vec3(barStart, barTop, z); // up left
				vertices[j++] = glm::vec3(barStart, barBottom, z); // bottom left

				vertices[j++] = glm::vec3(barStart, barBottom, z); // bottom left
				vertices[j++] = glm::vec3(barEnd, barBottom, z); // bottom right

				vertices[j++] = glm::vec3(barEnd, barBottom, z); // bottom right
				vertices[j++] = glm::vec3(barEnd, barTop, z); // up right

				barStart += dh;
			}
			vertexCount = barCount * 6;
			GLUtils::BufferLayout l;
			l.addAttribute(GLUtils::VertexType::Float, 3);
			const int64_t byteSize = vertices.size() * sizeof(vertices[0]);
			RETURN_ON_ERROR_CODE(vao.init());
			RETURN_ON_ERROR_CODE(vao.bind());
			RETURN_ON_ERROR_CODE(vertexBuffer.init(byteSize, vertices.data(), l));
			RETURN_ON_ERROR_CODE(vao.unbind());
			return EC::ErrorCode();
		}

		EC::ErrorCode draw() const override;
		EC::ErrorCode outline(const IMaterial& m, const IMaterial& om) const override;
	private:
		GLUtils::VAO vao;
		GLUtils::VertexBuffer vertexBuffer;
		int vertexCount;
		int barCount;
	};

	class Canvas : public IGeometry {
	public:
		Canvas();
		EC::ErrorCode init(const glm::vec3& lowLeft, const glm::vec3& upRight);
		EC::ErrorCode draw() const override;
	private:
		glm::vec3 lowLeft;
		glm::vec3 upRight;
		GLUtils::VertexBuffer vertexBuffer;
		GLUtils::VAO vao;
	};

	class Morphable2D {
	public:
		virtual int getVertexCount() const = 0;
		virtual const glm::vec3* getVertices() const = 0;
	};

	class Curve : public Morphable2D {
	public:
		Curve();
		enum CurveFlags {
			// The beginning of the curve should match the ending i.e.
			// the first and the last vertex will be the same.
			IsClosed = 1
		};
		template<typename FuncT>
		void init(FuncT&& f, int vertexCountIn, CurveFlags flags) {
			assert(vertexCountIn > 1);
			const bool isClosed = flags & CurveFlags::IsClosed;
			this->vertexCount = vertexCountIn + isClosed;
			vertices.clear();
			vertices.reserve(vertexCount);
			const float dh = 1.0f / vertexCountIn;
			float t = 0.0f;
			for (int i = 0; i < vertexCountIn; ++i, t += dh) {
				vertices.push_back(f(t));
			}
			if (isClosed) {
				vertices.push_back(vertices[0]);
			}
		}

		int getVertexCount() const override {
			return vertexCount;
		}

		const glm::vec3* getVertices() const override {
			return vertices.data();
		}
	private:
		int vertexCount;
		std::vector<glm::vec3> vertices;
	};

	glm::vec3 circleEquation(float t);
	glm::vec3 squareEquation(float t);

	class Morph2D : public IGeometry {
	public:
		Morph2D();
		Morph2D(const Morph2D&) = delete;
		Morph2D& operator=(const Morph2D&) = delete;
		Morph2D(Morph2D&& other) noexcept;
		Morph2D& operator=(Morph2D&& other) noexcept;
		EC::ErrorCode init(const Morphable2D& start, const Morphable2D& end);
		void freeMem();
		EC::ErrorCode draw() const override;
	private:
		GLUtils::VAO vao;
		GLUtils::VertexBuffer vertexBuffer;
		int vertexCount;
	};
}