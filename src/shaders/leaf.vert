#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
} camera;

layout(set = 1, binding = 0) uniform ModelBufferObject {
    mat4 model;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	mat4 scale = mat4(1.0);
	scale[0][0] = 0.01;
	scale[1][1] = 0.01;
	scale[2][2] = 0.01;
    gl_Position = camera.proj * camera.view * model * scale * vec4(inPosition, 1.0);
    fragColor = vec3(inColor);
    fragTexCoord = inTexCoord;
}
