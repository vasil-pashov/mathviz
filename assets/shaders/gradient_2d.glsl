#shader vertex
#version 330 core
layout(location = 0) in vec3 position;
uniform vec3 start;
uniform vec3 end;
uniform vec3 colorStart;
uniform vec3 colorEnd;
uniform mat4 projection;
uniform mat4 view;
out vec3 vertexColor;

void main() {
	float coeff = length(position - start);
	gl_Position = projection * view * vec4(position, 1.0f);
	vertexColor = colorStart + coeff * (colorEnd - colorStart);
}

#shader fragment
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;
void main() {
	FragColor = vec4(vertexColor, 1.0f);
}