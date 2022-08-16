#include <memory>
#include "GLFW/glfw3.h"
#include "material.h"
namespace EC {
	class ErrorCode;
}

namespace GLUtils {
	class Program;
}

struct GLFWwindow;

namespace MathViz {
	class Node;

	class Context {
	public:
		Context();
		Context(const Context&) = delete;
		Context& operator=(const Context&) = delete;
		~Context();
		[[nodiscard]]
		EC::ErrorCode init(int width, int height);
		void onResize(int width, int height);
		void freeMem();
		[[nodiscard]]
		EC::ErrorCode mainLoop();
	private:
		EC::ErrorCode loadShaders();
		EC::ErrorCode drawNode(const Node& node);
		EC::ErrorCode setMatrices(const GLUtils::Program& program);
		std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window;
		MaterialFactory materialFactory;
		int width;
		int height;
	};
}