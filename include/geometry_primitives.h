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
		virtual EC::ErrorCode outline(const IMaterial& objectMaterial, const IMaterial& outlineMaterial) {
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
		/// @brief Upload the geometry to the GPU. Must be called after init.
		EC::ErrorCode upload();
		/// @brief Issue a draw call. Must be called only after init and upload are called. 
		EC::ErrorCode draw() const override;
		void freeMem();
	private:
		/// The start of the line in world space
		glm::vec3 start;
		/// The end of the line in world space
		glm::vec3 end;
		GLUtils::Buffer vertexBuffer;
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
		GLUtils::Buffer vertexBuffer;
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
		EC::ErrorCode draw() const override;
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

	class ReimanArea : public IGeometry {
	public:
		ReimanArea();
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

		EC::ErrorCode draw() const override;
	private:
		GLUtils::VAO vao;
		GLUtils::Buffer vertexBuffer;
		int vertexCount;
	};

	class Canvas : public IGeometry {
	public:
		Canvas();
		EC::ErrorCode init(const glm::vec3& lowLeft, const glm::vec3& upRight);
		EC::ErrorCode upload();
		EC::ErrorCode draw() const override;
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
	};

	class Curve : public Morphable2D {
	public:
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
		Morph2D() = default;
		Morph2D(const Morph2D&) = delete;
		Morph2D& operator=(const Morph2D&) = delete;
		Morph2D(Morph2D&& other) noexcept;
		Morph2D& operator=(Morph2D&& other) noexcept;
		EC::ErrorCode init(const Morphable2D& start, const Morphable2D& end);
		void freeMem();
		EC::ErrorCode draw() const override;
	private:
		GLUtils::VAO vao;
		GLUtils::Buffer vertexBuffer;
		int vertexCount;
	};
}