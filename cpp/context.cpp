#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glutils.h"
#include "context.h"
#include "error_code.h"
#include "geometry_primitives.h"
#include "shader_bindings.h"
#include "material.h"

namespace MathViz {

	struct Node {
		enum Flags {
			None = 0,
			Outline = 1
		};
		IMaterial* material;
		IMaterial* outlineMaterial;
		IGeometry* geometry;
		unsigned int flags;
	};

	static inline void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
		Context* ctx = static_cast<Context*>(glfwGetWindowUserPointer(window));
		ctx->onResize(width, height);
	}

	Context::Context() :
		window(nullptr, &glfwDestroyWindow),
		shaderPrograms(int(ShaderTable::Count)),
		width(0),
		height(0)
	{ }

	Context::~Context() {
		freeMem();
	}

	EC::ErrorCode Context::init(int widthIn, int heightIn) {
		width = widthIn;
		height = heightIn;
		const int status = glfwInit();
		if (status != GLFW_TRUE) {
			const char* description;
			const int errorCode = glfwGetError(&description);
			return EC::ErrorCode(status, "%s", description);
		}
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		window.reset(glfwCreateWindow(width, height, "mathviz", NULL, NULL));
		glfwSetWindowUserPointer(window.get(), this);
		glfwMakeContextCurrent(window.get());
		glfwSetFramebufferSizeCallback(window.get(), framebufferSizeCallback);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			return EC::ErrorCode("Failed to initialize GLAD");
		}

		RETURN_ON_ERROR_CODE(loadShaders());

		RETURN_ON_GL_ERROR(glEnable(GL_LINE_SMOOTH));
		RETURN_ON_GL_ERROR(glHint(GL_LINE_SMOOTH_HINT, GL_NICEST));

		RETURN_ON_GL_ERROR(glEnable(GL_DEPTH_TEST));
		return EC::ErrorCode();
	}

	void Context::onResize(int width, int height) {
		// make sure the viewport matches the new window dimensions; note that width and
		// height will be significantly larger than specified on retina displays.
		this->width = width;
		this->height = height;
		glViewport(0, 0, width, height);
	}

	void Context::freeMem() {
		shaderPrograms.clear();
		window.reset();
		glfwTerminate();
	}

	EC::ErrorCode Context::mainLoop() {
		MathViz::Plot2D plot;
		const MathViz::Range2D xRange(-5, 5);
		const MathViz::Range2D yRange(-1, 1);
		const auto f = [](float x) -> float {
			return std::sin(x);
		};
		plot.init(f, xRange, yRange, 1.0f, 100);
		plot.upload();

		FlatColor red({1.0f, 0.0f, 0.0f});
		FlatColor blue({0.0f, 0.0f, 1.0f});
		Gradient2D grad(
			{-5.f, -1.f, 0.f},
			{5.f, 1.f, 0.f},
			{0.1f, 0.4f, 0.7f},
			{0.f, 0.5f, 0.8}
		);

		Node plotNode;
		plotNode.material = &red;
		plotNode.geometry = &plot;

		MathViz::ReimanArea r;
		RETURN_ON_ERROR_CODE(r.init(f, xRange, 0.1));
		Node reimanNode;
		reimanNode.material = &grad;
		reimanNode.geometry = &r;

		const int morphVerts = 100;

		MathViz::Curve circle;
		circle.init(MathViz::circleEquation, morphVerts, MathViz::Curve::IsClosed);

		MathViz::Curve rect;
		rect.init(MathViz::squareEquation, morphVerts, MathViz::Curve::IsClosed);

		MathViz::Morph2D morph1, morph2;
		morph1.init(circle, rect);
		morph2.init(rect, circle);

		const float fov = 45.0f;

		const glm::mat4 perspective = glm::perspective(glm::radians(fov), (float)800 / (float)900, 0.1f, 100.0f);


		while (!glfwWindowShouldClose(window.get())) {
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			drawNode(plotNode);
			drawNode(reimanNode);

			glfwSwapBuffers(window.get());

			glfwPollEvents();
		}
		return EC::ErrorCode();
	}

	EC::ErrorCode Context::loadShaders() {
		for (int i = 0; i < int(ShaderTable::Count); ++i) {
			GLUtils::Pipeline pipeline;
			RETURN_ON_ERROR_CODE(pipeline.init(shaderPaths[i]));

			RETURN_ON_ERROR_CODE(shaderPrograms[i].init(pipeline));
		}
		return EC::ErrorCode();
	}

	EC::ErrorCode Context::drawNode(const Node& node) {
		IMaterial::ShaderId shaderId = node.material->getShaderId();
		const GLUtils::Program& shader = shaderPrograms[shaderId];
		RETURN_ON_ERROR_CODE(shader.bind());
		RETURN_ON_ERROR_CODE(node.material->setUniforms(shader));
		RETURN_ON_ERROR_CODE(node.geometry->draw());
		RETURN_ON_ERROR_CODE(setMatrices(shader));
		shader.unbind();
		return EC::ErrorCode();
	}

	EC::ErrorCode Context::setMatrices(const GLUtils::Program& program) {
		const glm::mat4 ortho = glm::ortho(-5.f, 5.f, -1.f, 1.f, 0.f, 10.f);

		const glm::vec3 cameraPos(0.0f, 0.0f, -1.0f);
		const glm::vec3 lookAtPoint(0.0f, 0.0f, 0.0f);
		const glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
		const glm::mat4 view = glm::lookAt(cameraPos, lookAtPoint, cameraUp);
		RETURN_ON_ERROR_CODE(program.setUniform("projection", ortho, false));
		RETURN_ON_ERROR_CODE(program.setUniform("view", view, false));
		return EC::ErrorCode();
	}
}