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

vec4 SmoothCurve( vec4 x ) {
  return x * x * ( vec4(3.0f,3.0f,3.0f,3.0f) - 2.0 * x ) ;
}
vec4 TriangleWave( vec4 x ) {
  return abs( fract( x + 0.5 ) * 2.0 - 1.0 );
}
vec4 SmoothTriangleWave( vec4 x ) {
  return SmoothCurve( TriangleWave( x ) );
}

void ApplyMainBending(inout vec3 vPos, vec2 vWind, float fBendScale){
	// Calculate the length from the ground, since we'll need it.
	float fLength = length(vPos);
	// Bend factor - Wind variation is done on the CPU.
	float fBF = vPos.y * fBendScale;
	// Smooth bending factor and increase its nearby height limit.
	fBF += 1.0;
	fBF *= fBF;
	fBF = fBF * fBF - fBF;
	fBF = fBF * fBF;
	// Displace position
	vec3 vNewPos = vPos;
	vNewPos.xz += vWind.xy * fBF;
	vPos.xyz = normalize(vNewPos.xyz)* fLength;
}

void ApplyDetailBending(
	inout vec3 vPos,		// The final world position of the vertex being modified
	vec2 Wind,
	vec3 vNormal,			// The world normal for this vertex
	vec3 objectPosition,	        // The world position of the plant instance (same for all vertices)
	float fDetailPhase,		// Optional phase for side-to-side. This is used to vary the phase for side-to-side motion
	float fBranchPhase,		// The green vertex channel per Crytek's convention
	float fTime,			// Ever-increasing time value (e.g. seconds ellapsed)
	float fEdgeAtten,		// "Leaf stiffness", red vertex channel per Crytek's convention
	float fBranchAtten,		// "Overall stiffness", *inverse* of blue channel per Crytek's convention
	float fBranchAmp,		// Controls how much up and down
	float fSpeed,			// Controls how quickly the leaf oscillates
	float fDetailFreq,		// Same thing as fSpeed (they could really be combined, but I suspect
		// this could be used to let you additionally control the speed per vertex).
	float fDetailAmp,		// Controls how much back and forth
	float fOccilateFreq,    // Ojasdhkas
	float fDetailShakeAmp
	)		
{
	float fObjPhase = dot(objectPosition.xyz, vec3(1.0f, 1.0f, 1.0f));  
	fBranchPhase += fObjPhase;
	float SIDE_TO_SIDE_FREQ1=1.975;
	float SIDE_TO_SIDE_FREQ2=0.793;
	float UP_AND_DOWN_FREQ1=0.375;
	float UP_AND_DOWN_FREQ2=0.193;
	float fVtxPhase = dot(vPos.xyz, vec3(fDetailPhase + fBranchPhase,fDetailPhase + fBranchPhase,fDetailPhase + fBranchPhase));  
	vec2 vWavesIn = vec2(fTime,fTime) + vec2(fVtxPhase, fBranchPhase );
	vec4 vWaves = (fract(fOccilateFreq * vWavesIn.xxyy *
			vec4(SIDE_TO_SIDE_FREQ1, SIDE_TO_SIDE_FREQ2, UP_AND_DOWN_FREQ1, UP_AND_DOWN_FREQ2)) *
			 2.0 - 1.0 );
	vec4 vWaves1 = SmoothTriangleWave( vWaves * fSpeed);
	vec2 vWavesSum1 = vWaves1.xz + vWaves1.yw;
	vPos.xyz += vWavesSum1.x * vec3(fEdgeAtten * fDetailAmp * vNormal.xyz);
	vPos.y += vWavesSum1.y * fBranchAtten * fBranchAmp;
	vec4 vWaves2 = SmoothTriangleWave( vWaves * fDetailFreq);
	vPos.xz = vPos.xz + Wind.xy * fBranchAtten * fDetailShakeAmp * (vWaves2.x + 1.0f);
}

void main() {
	mat3 inv_trans_model = transpose(inverse(mat3(model)));
	vec3 vPos=vec3(model * vec4(inPosition, 1.0f));
	vec3 normalDir=normalize(inv_trans_model * inNormal);
	worldN = normalDir;
	worldB = normalize(inv_trans_model * inBitangent);
	worldT = normalize(inv_trans_model * inTangent);
	vertAmbient = inColor.a;


	//Wind
	vec3 wind_dir = normalize(vec3(0.5, 0, 1));
    float wind_speed = 8.0;
    float wave_division_width = 5.0;
    float wave_info = (cos((dot(vec3(0, 0, 0), wind_dir) - wind_speed * totalTime) / wave_division_width) + 0.7);
	
	float wind_power = 15.0f;
    //vec3 w = wind_dir * wind_power * wave_info * fd * fr;
	vec3 w=wind_dir * wind_power * wave_info;
	vec2 Wind=vec2(w.x*0.05,w.z*0.05);



	vec3 objectPosition = vec3(0,0,0);
	vPos -= objectPosition;	// Reset the vertex to base-zero
	float BendScale=0.001;
	ApplyMainBending(vPos, Wind, BendScale);
	vPos += objectPosition;


	float BranchAmp=0.1;
	float DetailAmp=0.1;
	float DetailShakeAmp=0.1;
	vec2 WindDetail = vec2(w.x * 0.5,w.z*0.5);
	float windStrength = length(WindDetail);
	//ApplyDetailBending(
		//vPos,
		//WindDetail,
		//normalDirection,
		//objectPosition,
		//0,					// Leaf phase - not used in this scenario, but would allow for variation in side-to-side motion
		//inColor.g,		// Branch phase - should be the same for all verts in a leaf/branch.
		//uTime,
		//inColor.r,		// edge attenuation, leaf stiffness
		//1 - inColor.b,  // branch attenuation. High values close to stem, low values furthest from stem.
				// For some reason, Crysis uses solid blue for non-moving, and black for most movement.
				// So we invert the blue value here.
		//BranchAmp * windStrength, // branch amplitude. Play with this until it looks good.
		//Speed,					// Speed. Play with this until it looks good.
		//DetailFreq,					// Detail frequency. Keep this at 1 unless you want to have different per-leaf frequency
		//DetailAmp * windStrength,	// Detail amplitude. Play with this until it looks good.
		//0.002f,
		//DetailShakeAmp
		//);

	mat4 scale = mat4(1.0);
	scale[0][0] = 0.01;
	scale[1][1] = 0.01;
	scale[2][2] = 0.01;
	worldPosition = vPos;

    //gl_Position = camera.proj * camera.view * model * scale * vec4(inPosition, 1.0);
	gl_Position = camera.proj * camera.view  * scale * vec4(vPos, 1.0);
	
    vertColor = vec3(inColor);
    fragTexCoord = inTexCoord;
}
