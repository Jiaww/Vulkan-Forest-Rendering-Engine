#version 450
#extension GL_ARB_separate_shader_objects : enable


layout( set = 1, binding = 0 ) uniform samplerCube Cubemap;

layout(location = 0) in vec4 vert_texcoord;
layout(location = 0) out vec4 outColor;

void main() {
	vec3 texcoord=vert_texcoord.xyz;
	outColor = texture( Cubemap, texcoord );
}
