#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D noiseSampler;

layout(location = 0) in vec3 vertColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 worldPosition;
layout(location = 3) in vec3 worldN;
layout(location = 4) in vec3 worldB;
layout(location = 5) in vec3 worldT;
layout(location = 6) in float vertAmbient;
layout(location = 7) in float distanceLevel;
layout(location = 8) in vec2 noiseTexCoord;
layout(location = 9) in vec3 test;

layout(location = 0) out vec4 outColor;

void main() {
	// LOD Morphing
	vec4 noiseColor = texture(noiseSampler, noiseTexCoord);
	float dis = (distanceLevel - 0.3f)/(0.5f - 0.3f);
	if(dis < noiseColor.x)
		discard;

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
	float ambientTerm = vertAmbient * 0.25f;

	if(diffuseColor.a < 0.9f)
		discard;
    //outColor = vec4(diffuseColor.rgb * diffuseTerm + diffuseColor.rgb * ambientTerm, 1.0f);
	outColor=vec4(0.0f, abs(test[2]), 0.0f,1.0);
}