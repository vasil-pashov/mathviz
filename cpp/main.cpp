#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>
#include "error_code.h"
#include "glutils.h"
#include "primitives.h"

void logError(const char* error) {
	printf("[Error] %s\n", error);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}


const char* getLineVertexShader() {
	return
		"#version 330 core\n"
		"layout(location = 0) in vec3 position;\n"
		"uniform vec3 color;\n"
		"out vec3 vertexColor;\n"
		"void main() {\n"
		"	gl_Position = vec4(position, 1.0f);\n"
		"	vertexColor = color;\n"
		"}\n";
}

const char* getLineFragmentShader() {
	return
		"#version 330 core\n"
		"in vec3 vertexColor;\n"
		"out vec4 FragColor;\n"
		"void main() {\n"
		"	FragColor = vec4(vertexColor, 1.0f);\n"
		"}\n";
}

int main() {
	
	const int status = glfwInit();
	if (status != GLFW_TRUE) {
		const char* description;
		const int errorCode = glfwGetError(&description);
		logError(description);
		return status;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(800, 900, "mathviz", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		logError("Failed to initialize GLAD");
		return -1;
	}

	GLUtils::Program p;
	EC::ErrorCode err = p.initFromSources(getLineVertexShader(), getLineFragmentShader());
	if (err.hasError()) {
		logError(err.getMessage());
		return err.getStatus();
	}

	GLUtils::Plot2D plot;
	err = plot.init([](float x) {return std::sin(x); }, -1, 1, 100);
	if (err.hasError()) {
		logError(err.getMessage());
		return err.getStatus();
	}
	err = plot.upload();
	if (err.hasError()) {
		logError(err.getMessage());
		return err.getStatus();
	}

	glEnable(GL_LINE_SMOOTH);

	while (!glfwWindowShouldClose(window)) {
		/* Render here */

		plot.draw();
        // l.draw(p);
        // l2.draw();
		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
}