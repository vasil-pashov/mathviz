#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>s
#include "error_code.h"
#include "glutils.h"
#include "primitives.h"

#define EXIT_ON_ERROR_CODE(_Err) \
{ \
	const EC::ErrorCode& err = _Err; \
	if(err.hasError()) { \
		logError(err.getMessage(), __LINE__); \
		std::exit(err.getStatus()); \
	} \
}

void logError(const char* error, int line) {
	printf("[Error] %d %s\n", line, error);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

EC::ErrorCode initOpenGL(GLFWwindow** window) {
	const int status = glfwInit();
	if (status != GLFW_TRUE) {
		const char* description;
		const int errorCode = glfwGetError(&description);
		return EC::ErrorCode(status, "%s", description);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	*window = glfwCreateWindow(800, 900, "mathviz", NULL, NULL);
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, framebufferSizeCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		return EC::ErrorCode("Failed to initialize GLAD");
	}
	glEnable(GL_LINE_SMOOTH);
	return EC::ErrorCode();
}

int main() {
	
	GLFWwindow* window;
	EXIT_ON_ERROR_CODE(initOpenGL(&window));
	GLUtils::Program morphProgram, plotProgram;
	EXIT_ON_ERROR_CODE(morphProgram.initFromMegaShader("\\assets\\shaders\\line_morph.glsl"));
	EXIT_ON_ERROR_CODE(plotProgram.initFromMegaShader("\\assets\\shaders\\flat_color.glsl"));

	GLUtils::Plot2D plot;
	const GLUtils::Range2D xRange(-5, 5);
	const GLUtils::Range2D yRange(-1, 1);
	const auto f = [](float x) -> float {
		return std::sin(x);
	};
	plot.init(f, xRange, yRange, 1.0f, 100);
	plot.upload();

	GLUtils::ReimanArea r;
	EXIT_ON_ERROR_CODE(r.init(f, xRange, 0.1));

	GLUtils::Circle c(100, 5);
	GLUtils::Rectangle rect(glm::vec3(-5.f, -5.f, 1.0f), glm::vec3(5.f, 5.f, 1.0f));
	GLUtils::Morph2D morph;
	morph.init(rect, c);

	const float fov = 45.0f;

	const glm::mat4 perspective = glm::perspective(glm::radians(fov), (float)800 / (float)900, 0.1f, 100.0f);
	const glm::mat4 ortho = glm::ortho(-10.f, 10.f, -10.f, 10.f, -10.f, 10.f);
	
	const glm::vec3 cameraPos(0.0f, 0.0f, -1.0f);
	const glm::vec3 lookAtPoint(0.0f, 0.0f, 0.0f);
	const glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
	const glm::mat4 view = glm::lookAt(cameraPos, lookAtPoint, cameraUp);





	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		EXIT_ON_ERROR_CODE(plotProgram.bind());
		EXIT_ON_ERROR_CODE(plotProgram.setUniform("projection", ortho, false));
		EXIT_ON_ERROR_CODE(plotProgram.setUniform("view", view, false));
		EXIT_ON_ERROR_CODE(plotProgram.setUniform("color", glm::vec3(0.5f, 0.5f, 0.f)));
		EXIT_ON_ERROR_CODE(plot.draw());

		EXIT_ON_ERROR_CODE(plotProgram.setUniform("color", glm::vec3(0.2f, 0.7f, 3.f)));
		EXIT_ON_ERROR_CODE(r.draw());
		plotProgram.unbind();

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	glfwTerminate();
}