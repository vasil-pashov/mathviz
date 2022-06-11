#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glutils.h"
#include "context.h"
#include "error_code.h"
#include "primitives.h"

namespace MathViz {
	static inline void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
		Context* ctx = static_cast<Context*>(glfwGetWindowUserPointer(window));
		ctx->onResize(width, height);
	}

	Context::Context() :
		window(nullptr, &glfwDestroyWindow),
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
		window.reset();
		glfwTerminate();
	}

	EC::ErrorCode Context::mainLoop() {
		GLUtils::Pipeline morphPipeline, plotPipeline;
		GLUtils::Program morphProgram, plotProgram;
		RETURN_ON_ERROR_CODE(morphPipeline.init("\\assets\\shaders\\line_morph.glsl"));
		RETURN_ON_ERROR_CODE(plotPipeline.init("\\assets\\shaders\\flat_color.glsl"));
		RETURN_ON_ERROR_CODE(morphProgram.init(morphPipeline));
		RETURN_ON_ERROR_CODE(plotProgram.init(plotPipeline));

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

		MathViz::Circle c(100, 5);
		MathViz::Rectangle rect(glm::vec3(-5.f, -5.f, 1.0f), glm::vec3(5.f, 5.f, 1.0f));
		MathViz::Morph2D morph;
		morph.init(rect, c);

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

			RETURN_ON_ERROR_CODE(plotProgram.bind());
			RETURN_ON_ERROR_CODE(plotProgram.setUniform("projection", ortho, false));
			RETURN_ON_ERROR_CODE(plotProgram.setUniform("view", view, false));
			RETURN_ON_ERROR_CODE(plotProgram.setUniform("color", glm::vec3(0.5f, 0.5f, 0.f)));
			RETURN_ON_ERROR_CODE(plot.draw());

			RETURN_ON_ERROR_CODE(plotProgram.setUniform("color", glm::vec3(0.2f, 0.7f, 3.f)));
			RETURN_ON_ERROR_CODE(r.draw());
			plotProgram.unbind();

			glfwSwapBuffers(window.get());

			glfwPollEvents();
		}
		return EC::ErrorCode();
	}
}