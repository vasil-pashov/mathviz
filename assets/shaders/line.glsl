#shader vertex
#version 330 core
layout(location = 0) in vec3 position;
uniform vec3 color;
uniform mat4 view;
uniform mat4 projection;
out vec3 vertexColor;
void main() {
	gl_Position = projection * view * vec4(position, 1.0f);
	vertexColor = color;
}

#shader fragment
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;
void main() {
	FragColor = vec4(vertexColor, 1.0f);
}