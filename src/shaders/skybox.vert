#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
	vec4 camPos;
	vec4 camDir;
} camera;


layout(location = 0) in vec4 inPosition;

layout(location = 0) out vec4 vert_texcoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	vec4 pos = vec4(inPosition.x+camera.camPos.x,
					inPosition.y+camera.camPos.y,
					inPosition.z+camera.camPos.z,1);
	pos=camera.proj * camera.view*pos;
	gl_Position=pos.xyww;
	vert_texcoord=inPosition;
}
