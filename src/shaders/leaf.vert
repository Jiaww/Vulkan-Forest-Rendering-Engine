#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
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

layout(location = 0) out vec3 vertColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 worldPosition;
layout(location = 3) out vec3 worldN;
layout(location = 4) out vec3 worldB;
layout(location = 5) out vec3 worldT;
layout(location = 7) out float vertAmbient;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	mat3 inv_trans_model = transpose(inverse(mat3(model)));

	worldPosition = vec3(model * vec4(inPosition, 1.0f));
	worldN = normalize(inv_trans_model * inNormal);
	worldB = normalize(inv_trans_model * inBitangent);
	worldT = normalize(inv_trans_model * inTangent);
	vertAmbient = inColor.a;

	mat4 scale = mat4(1.0);
	scale[0][0] = 0.01;
	scale[1][1] = 0.01;
	scale[2][2] = 0.01;
    gl_Position = camera.proj * camera.view * model * scale * vec4(inPosition, 1.0);

    vertColor = vec3(inColor);
    fragTexCoord = inTexCoord;
}
