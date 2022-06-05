#shader vertex
#version 330 core
layout(location = 0) in vec3 startPos;
layout(location = 1) in vec3 endPos;
uniform float lerpCoeff;
uniform mat4 projection;
uniform mat4 view;
void main() {
	vec3 mixedPos = mix(startPos, endPos, lerpCoeff);
	gl_Position = projection * view * vec4(mixedPos, 1.0f);
}

#shader fragment
#version 330 core
out vec4 FragColor;
void main() {
	FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}