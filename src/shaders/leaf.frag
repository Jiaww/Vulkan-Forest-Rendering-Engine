#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D noiseSampler;

layout(set = 2, binding = 0) uniform Time {
    vec2 TimeInfo;
	// 0: deltaTime 1: totalTime
};

layout(set = 3, binding = 0) uniform LODINFO{
	// 0: LOD0 1: LOD1 2: TreeHeight 3: NumTrees
	vec4 LODInfo;
};

layout(location = 0) in vec3 vertColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 worldPosition;
layout(location = 3) in vec3 worldN;
layout(location = 4) in vec3 worldB;
layout(location = 5) in vec3 worldT;
layout(location = 6) in float vertAmbient;
layout(location = 7) in float distanceLevel;
layout(location = 8) in vec2 noiseTexCoord;
layout(location = 9) in vec3 tintColor;

layout(location = 0) out vec4 outColor;

const vec3 lightDir = vec3(-1.0, 5.0, -3.0);
const vec3 lightColorDay = vec3(1.0, 1.0, 0.94);
const vec3 lightColorAfternoon = vec3(1.0, 0.9, 0.7);
const vec3 lightColorNight = vec3(0.95, 1.0, 1.0);

void main() {
	// LOD Morphing
	vec4 noiseColor = texture(noiseSampler, noiseTexCoord);
	float dis = (distanceLevel - LODInfo.y)/(LODInfo.x - LODInfo.y);
	if(dis >= noiseColor.x)
		discard;

	// Local normal, in tangent space
	vec3 TextureNormal_tangentspace;
	TextureNormal_tangentspace = (texture( normalSampler, fragTexCoord ).rgb*2.0f - 1.0f);
	TextureNormal_tangentspace.x *= 1.3f;
	TextureNormal_tangentspace.y *= 1.3f;
	vec3 TextureNormal_worldspace = normalize(worldT * TextureNormal_tangentspace.x + worldB * -TextureNormal_tangentspace.y + worldN * TextureNormal_tangentspace.z);

	vec4 diffuseColor = texture(texSampler, fragTexCoord);
	
	if(diffuseColor.a < 0.8f)
		discard;
	
	// Calculate the diffuse term for Lambert shading
	float diffuseTerm = clamp(dot(TextureNormal_worldspace, normalize(lightDir)), 0.15f, 1);
	// Avoid negative lighting values
	float ambientTerm = vertAmbient * 0.3f;

	// Day and Night Cycle
	float currentTime = TimeInfo[1] - (40 * floor(TimeInfo[1]/40));
	float lightIntensity = 1.0f;
	vec3 lightColor;
	if(currentTime < 10){
		lightColor = lightColorDay * (1.0 - (currentTime)/10.0) + lightColorAfternoon * (currentTime)/10.0; 
		lightIntensity = 1.1f * (1.0 - (currentTime)/10.0) + 0.9f * (currentTime)/10.0;
	}
	else if(currentTime < 20){
		lightColor = lightColorAfternoon * (1.0 - (currentTime-10)/10.0) + lightColorNight * (currentTime-10)/10.0; 
		lightIntensity = 0.9f * (1.0 - (currentTime-10)/10.0) + 0.4f * (currentTime-10)/10.0;
	}
	else{
		lightColor = lightColorNight * (1.0 - (currentTime-20)/20.0) + lightColorDay * (currentTime-20)/20.0; 
		lightIntensity = 0.4f * (1.0 - (currentTime-20)/20.0) + 1.1f * (currentTime-20)/20.0;
	}

	outColor = vec4(diffuseColor.rgb * tintColor * lightColor * lightIntensity * (diffuseTerm + ambientTerm), diffuseColor.a);
	//outColor = vec4(diffuseColor.a);
}
