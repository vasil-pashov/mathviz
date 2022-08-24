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
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "expression.h"

namespace MathViz {

	struct Node {
		Node() :
			transform(1),
			material(nullptr),
			outlineMaterial(nullptr),
			geometry(nullptr),
			flags(0)
		{ }
		enum Flags {
			None = 0,
			Outline = 1
		};
		glm::mat4 transform;
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
		width(0),
		height(0),
		transforms(ReservedUBOBindings::ProjectionView)
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
			return EC::ErrorCode(errorCode, "%s", description);
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

		{
			projection = glm::ortho(-5.f, 5.f, -5.f, 5.f, -5.f, 5.f);
			const glm::vec3 cameraPos(0.0f, 0.0f, 1.0f);
			const glm::vec3 lookAtPoint(0.0f, 0.0f, 0.0f);
			const glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
			view = glm::lookAt(cameraPos, lookAtPoint, cameraUp);
			const glm::mat4 pv = projection * view;
			RETURN_ON_ERROR_CODE(transforms.init(2 * sizeof(glm::mat4), (void*)glm::value_ptr(pv)));
		}
		
		RETURN_ON_ERROR_CODE(initImgui());

		return EC::ErrorCode();
	}

	EC::ErrorCode Context::initImgui() {
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
		ImGui_ImplOpenGL3_Init("#version 150");
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

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		materialFactory.freeMem();
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

		FlatColor red = materialFactory.create<FlatColor>(glm::vec3(1.0f, 0.0f, 0.0f));
		FlatColor blue = materialFactory.create<FlatColor>(glm::vec3{0.0f, 0.0f, 1.0f});
		Gradient2D grad = materialFactory.create<Gradient2D>(
			glm::vec3(-5.f, -1.f, 0.f),
			glm::vec3(5.f, 1.f, 0.f),
			glm::vec3(0.1f, 0.4f, 0.7f),
			glm::vec3(0.f, 0.5f, 0.8)
		);

		Node plotNode;
		plotNode.material = &red;
		plotNode.geometry = &plot;

		MathViz::ReimanArea r;
		RETURN_ON_ERROR_CODE(r.init(f, xRange, 0.1));
		Node reimanNode;
		reimanNode.material = &grad;
		reimanNode.geometry = &r;
		reimanNode.outlineMaterial = &red;
		reimanNode.flags |= Node::Flags::Outline;

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


		transforms.bind();


		// ImGui state
		ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
		std::string expressionText;

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		while (!glfwWindowShouldClose(window.get())) {
			glfwPollEvents();

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			{
				const bool collapsed = ImGui::Begin("MathViz!");

				ImGui::ColorEdit3("clear color", (float*)&clear_color);

				ImGui::Text(
					"Application average %.3f ms/frame (%.1f FPS)",
					1000.0f / ImGui::GetIO().Framerate,
					ImGui::GetIO().Framerate
				);
				ImGui::InputText("Function", &expressionText);

				if (ImGui::Button("Plot")) {
					MathViz::Expression expr;
					const EC::ErrorCode& err = expr.init(expressionText.c_str());
					assert(err.hasError() == false);
					plot.reset([&expr](float x) {
						std::unordered_map<char, float> vals;
						vals['x'] = x;
						float res;
						expr.evaluate(&vals, res);
						return res;
					});
				}

				ImGui::End();
			}

			ImGui::Render();

			glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			drawNode(plotNode);
			// drawNode(reimanNode);

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(window.get());
		}
		return EC::ErrorCode();
	}

	EC::ErrorCode Context::loadShaders() {
		RETURN_ON_ERROR_CODE(materialFactory.init());
		return EC::ErrorCode();
	}

	EC::ErrorCode Context::drawNode(const Node& node) {
		const GLUtils::Program& p = node.material->getProgram();
		RETURN_ON_ERROR_CODE(p.bind());
		RETURN_ON_ERROR_CODE(transforms.upload(sizeof(glm::mat4), sizeof(glm::mat4), (void*)glm::value_ptr(node.transform)));
		RETURN_ON_ERROR_CODE(node.material->setUniforms());
		if (node.flags & Node::Flags::Outline) {
			RETURN_ON_ERROR_CODE(node.geometry->outline(*node.material, *node.outlineMaterial));
		} else {
			RETURN_ON_ERROR_CODE(node.geometry->draw());
		}
		p.unbind();
		return EC::ErrorCode();
	}
}