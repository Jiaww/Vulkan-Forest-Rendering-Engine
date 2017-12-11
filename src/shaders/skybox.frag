#version 450
#extension GL_ARB_separate_shader_objects : enable


layout( set = 1, binding = 0 ) uniform samplerCube Cubemap_Day;
layout( set = 1, binding = 1 ) uniform samplerCube Cubemap_Afternoon;
layout( set = 1, binding = 2 ) uniform samplerCube Cubemap_Night;

layout(set = 2, binding = 0) uniform Time {
    vec2 TimeInfo;
	// 0: deltaTime 1: totalTime
};

layout(set = 3, binding = 0) uniform DayNightInfo{
	//0: Daylength, 1: Activate
	vec2 DayNightData;
};

layout(location = 0) in vec4 vert_texcoord;
layout(location = 0) out vec4 outColor;

void main() {
	vec3 texcoord=vert_texcoord.xyz;
	// Day and Night Cycle
	// 40s a day 
	// a mod b : a - (b * floor(a/b))
	float dayLength = DayNightData.x;
	float currentTime = TimeInfo[1] - (dayLength * floor(TimeInfo[1]/dayLength));
	vec4 blendColor;
	if(currentTime < (dayLength/4.0)){
		blendColor = texture( Cubemap_Day, texcoord ) * (1.0 - (currentTime)/(dayLength/4.0)) + texture( Cubemap_Afternoon, texcoord ) * (currentTime)/(dayLength/4.0); 
	}
	else if(currentTime < (dayLength/2.0)){
		blendColor = texture( Cubemap_Afternoon, texcoord ) * (1.0 - (currentTime-(dayLength/4.0))/(dayLength/4.0)) + texture( Cubemap_Night, texcoord ) * (currentTime-(dayLength/4.0))/(dayLength/4.0); 
	}
	else{
		blendColor = texture( Cubemap_Night, texcoord ) * (1.0 - (currentTime-(dayLength/2.0))/(dayLength/2.0)) + texture( Cubemap_Day, texcoord ) * (currentTime-(dayLength/2.0))/(dayLength/2.0); 
	}

	float dayNightAct = DayNightData.y;
	outColor = blendColor*dayNightAct + texture( Cubemap_Day, texcoord ) * (1.0f-dayNightAct);
}
