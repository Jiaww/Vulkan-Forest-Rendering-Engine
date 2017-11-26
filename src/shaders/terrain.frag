#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	// Local normal, in tangent space
	vec3 TextureNormal_tangentspace;
	TextureNormal_tangentspace = (texture( normalSampler, fragTexCoord ).rgb*2.0f - 1.0f);
	vec3 TextureNormal_worldspace;
	TextureNormal_worldspace.y = TextureNormal_tangentspace.z;
	TextureNormal_worldspace.z = -TextureNormal_tangentspace.y;
	TextureNormal_worldspace.x = TextureNormal_tangentspace.x;
	
	// Calculate the diffuse term for Lambert shading
	vec3 lightDir = normalize(vec3(-1.0, 5.0, -3.0));
	// Calculate the diffuse term for Lambert shading
	float diffuseTerm = clamp(dot(normalize(TextureNormal_worldspace.xyz), normalize(lightDir)), 0.15f, 1.0f);
    vec4 diffuseColor = texture(texSampler, fragTexCoord);
	// Avoid negative lighting values
	float ambientTerm = 0.05f;

    outColor = vec4(diffuseColor.rgb * diffuseTerm + diffuseColor.rgb * ambientTerm, 1.0f);
}
