#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(set = 1, binding = 1) uniform sampler2D texSampler;
//layout(set = 1, binding = 2) uniform sampler2D normalSampler;

//layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
	outColor=fragColor;
}
