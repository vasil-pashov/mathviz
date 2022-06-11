#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "error_code.h"
#include "glutils.h"
#include "primitives.h"
#include "context.h"

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

int main() {
	MathViz::Context ctx;
	EXIT_ON_ERROR_CODE(ctx.init(800, 900));
	EXIT_ON_ERROR_CODE(ctx.mainLoop());
}