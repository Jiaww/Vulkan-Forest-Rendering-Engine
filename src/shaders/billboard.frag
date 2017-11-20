#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
	vec3 camPos;
} camera;
layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D noiseSampler;

layout(location = 0) in vec3 vertColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 worldPosition;
layout(location = 3) in vec3 worldN;
layout(location = 4) in vec3 worldB;
layout(location = 5) in vec3 worldT;
layout(location = 7) in float vertAmbient;

layout(location = 0) out vec4 outColor;

void main() {
	// Local normal, in tangent space
	vec3 TextureNormal_tangentspace;
	TextureNormal_tangentspace = (texture( normalSampler, fragTexCoord ).rgb*2.0f - 1.0f);
	TextureNormal_tangentspace.x *= 1.7f;
	TextureNormal_tangentspace.y *= 1.7f;
	vec3 TextureNormal_worldspace = normalize(worldT * TextureNormal_tangentspace.x + worldB * -TextureNormal_tangentspace.y + worldN * TextureNormal_tangentspace.z);

    vec4 diffuseColor = texture(texSampler, fragTexCoord);
	// Calculate the diffuse term for Lambert shading
	vec3 lightDir = normalize(vec3(-1.0, 5.0, -3.0));
	float diffuseTerm = clamp(dot(TextureNormal_worldspace, normalize(lightDir)), 0.15f, 1);
	// Avoid negative lighting values
	float ambientTerm = vertAmbient * 0.15f;

    outColor = vec4(diffuseColor.rgb * diffuseTerm + diffuseColor.rgb * ambientTerm, diffuseColor.a);
	//outColor=vec4(vertColor,diffuseColor.a);
}
