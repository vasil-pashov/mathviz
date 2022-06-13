#include <memory>
namespace EC {
	class ErrorCode;
}

struct GLFWwindow;

namespace MathViz {
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
		std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window;
		std::vector<GLUtils::Program> shaderPrograms;
		int width;
		int height;
	};
}