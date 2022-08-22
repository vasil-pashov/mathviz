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
		EC::ErrorCode initImgui();
		void onResize(int width, int height);
		void freeMem();
		[[nodiscard]]
		EC::ErrorCode mainLoop();
	private:
		enum ReservedUBOBindings {
			ProjectionView = 0
		};

		EC::ErrorCode loadShaders();
		EC::ErrorCode drawNode(const Node& node);
		EC::ErrorCode setMatrices(const GLUtils::Program& program);
		std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window;
		MaterialFactory materialFactory;
		int width;
		int height;

		// TODO: Create a camera class
		glm::mat4 view;
		glm::mat4 projection;
		/// ProjectionVew + Model transform
		GLUtils::UniformBuffer transforms;
	};
}