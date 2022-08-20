#shader vertex
#version 330 core
layout(location = 0) in vec3 startPos;
layout(location = 1) in vec3 endPos;

layout(std140, binding = 0) uniform ProjectionView
{
	mat4 projectionView;
	mat4 model;
};

uniform float lerpCoeff;

void main() {
	vec3 mixedPos = mix(startPos, endPos, lerpCoeff);
	gl_Position = projectionView * model * vec4(mixedPos, 1.0f);
}

#shader fragment
#version 330 core
out vec4 FragColor;
void main() {
	FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}