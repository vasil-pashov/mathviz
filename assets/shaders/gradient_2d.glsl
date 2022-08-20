#shader vertex
#version 330 core
layout(location = 0) in vec3 position;

layout(std140, binding = 0) uniform ProjectionView
{
	mat4 projectionView;
	mat4 model;
};

uniform vec3 start;
uniform vec3 end;
uniform vec3 colorStart;
uniform vec3 colorEnd;

out vec3 vertexColor;

void main() {
	float coeff = length(position - start);
	gl_Position = projectionView * model * vec4(position, 1.0f);
	vertexColor = colorStart + coeff * (colorEnd - colorStart);
}

#shader fragment
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;
void main() {
	FragColor = vec4(vertexColor, 1.0f);
}