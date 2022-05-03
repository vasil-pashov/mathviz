#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "error_code.h"

void logError(const char* error) {
	printf("[Error] %s\n", error);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
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

	while (!glfwWindowShouldClose(window)) {
		/* Render here */
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
}