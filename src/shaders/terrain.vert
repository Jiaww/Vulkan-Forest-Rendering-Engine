#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
	vec4 camPos;
	vec4 camDir;
} camera;

layout(set = 1, binding = 0) uniform ModelBufferObject {
    mat4 model;
};

layout(set = 2, binding = 0) uniform Time {
    float deltaTime;
    float totalTime;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 worldPosition;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	mat4 modelMatrix = model;
	vec3 vPos=vec3(modelMatrix * vec4(inPosition, 1.0f));
	worldPosition = vPos;

	gl_Position = camera.proj * camera.view * vec4(vPos, 1.0);
	
    fragTexCoord = inTexCoord;
}
