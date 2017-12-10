#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;

layout(set = 2, binding = 0) uniform Time {
    vec2 TimeInfo;
	// 0: deltaTime 1: totalTime
};

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const vec3 lightDir = vec3(-1.0, 5.0, -3.0);
const vec3 lightColorDay = vec3(1.0, 1.0, 0.94);
const vec3 lightColorAfternoon = vec3(1.0, 0.9, 0.7);
const vec3 lightColorNight = vec3(0.95, 1.0, 1.0);

void main() {
	// Local normal, in tangent space
	vec3 TextureNormal_tangentspace;
	TextureNormal_tangentspace = normalize(texture( normalSampler, fragTexCoord ).rgb*2.0f - 1.0f);
	vec3 TextureNormal_worldspace;
	TextureNormal_worldspace.y = TextureNormal_tangentspace.z;
	TextureNormal_worldspace.z = -TextureNormal_tangentspace.y;
	TextureNormal_worldspace.x = TextureNormal_tangentspace.x;
	
	// Calculate the diffuse term for Lambert shading
	// Calculate the diffuse term for Lambert shading
	float diffuseTerm = clamp(dot(normalize(TextureNormal_worldspace.xyz), normalize(lightDir)), 0.15f, 1.0f);
    vec4 diffuseColor = texture(texSampler, fragTexCoord);
	// Avoid negative lighting values
	float ambientTerm = 0.05f;

	// Day and Night Cycle
	float dayLength = 30;
	float currentTime = TimeInfo[1] - (dayLength * floor(TimeInfo[1]/dayLength));
	float lightIntensity = 1.0f;
	vec3 lightColor;
	if(currentTime < (dayLength/4.0)){
		lightColor = lightColorDay * (1.0 - (currentTime)/(dayLength/4.0)) + lightColorAfternoon * (currentTime)/(dayLength/4.0); 
		lightIntensity = 1.1f * (1.0 - (currentTime)/(dayLength/4.0)) + 0.9f * (currentTime)/(dayLength/4.0);
	}
	else if(currentTime < (dayLength/2.0)){
		lightColor = lightColorAfternoon * (1.0 - (currentTime-(dayLength/4.0))/(dayLength/4.0)) + lightColorNight * (currentTime-(dayLength/4.0))/(dayLength/4.0); 
		lightIntensity = 0.9f * (1.0 - (currentTime-(dayLength/4.0))/(dayLength/4.0)) + 0.4f * (currentTime-(dayLength/4.0))/(dayLength/4.0);
	}
	else{
		lightColor = lightColorNight * (1.0 - (currentTime-(dayLength/2.0))/(dayLength/2.0)) + lightColorDay * (currentTime-(dayLength/2.0))/(dayLength/2.0); 
		lightIntensity = 0.4f * (1.0 - (currentTime-(dayLength/2.0))/(dayLength/2.0)) + 1.1f * (currentTime-(dayLength/2.0))/(dayLength/2.0);
	}

    outColor = vec4(diffuseColor.rgb * lightColor * lightIntensity * (diffuseTerm +  ambientTerm), 1.0f);
}
