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

namespace MathViz {

	static inline void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
		Context* ctx = static_cast<Context*>(glfwGetWindowUserPointer(window));
		ctx->onResize(width, height);
	}

	Context::Context() :
		window(nullptr, &glfwDestroyWindow),
		shaderPrograms(ShaderTable::Count),
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

		MathViz::ReimanArea r;
		RETURN_ON_ERROR_CODE(r.init(f, xRange, 0.1));

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
		const glm::mat4 ortho = glm::ortho(-10.f, 10.f, -10.f, 10.f, -10.f, 10.f);

		const glm::vec3 cameraPos(0.0f, 0.0f, -1.0f);
		const glm::vec3 lookAtPoint(0.0f, 0.0f, 0.0f);
		const glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
		const glm::mat4 view = glm::lookAt(cameraPos, lookAtPoint, cameraUp);

		while (!glfwWindowShouldClose(window.get())) {
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			RETURN_ON_ERROR_CODE(shaderPrograms[ShaderTable::Morph].bind());
			RETURN_ON_ERROR_CODE(shaderPrograms[ShaderTable::Morph].setUniform("projection", ortho, false));
			RETURN_ON_ERROR_CODE(shaderPrograms[ShaderTable::Morph].setUniform("view", view, false));
			RETURN_ON_ERROR_CODE(shaderPrograms[ShaderTable::Morph].setUniform("color", glm::vec3(0.5f, 0.5f, 0.f)));
			RETURN_ON_ERROR_CODE(shaderPrograms[ShaderTable::Morph].setUniform("lerpCoeff", std::min(float(glfwGetTime()) / 5.0f, 1.0f)));

			morph1.draw();
			morph2.draw();
			shaderPrograms[ShaderTable::FlatColor].unbind();

			glfwSwapBuffers(window.get());

			glfwPollEvents();
		}
		return EC::ErrorCode();
	}

	EC::ErrorCode Context::loadShaders() {
		for (int i = 0; i < ShaderTable::Count; ++i) {
			GLUtils::Pipeline pipeline;
			RETURN_ON_ERROR_CODE(pipeline.init(shaderPaths[i]));

			RETURN_ON_ERROR_CODE(shaderPrograms[i].init(pipeline));
		}
		return EC::ErrorCode();
	}
}