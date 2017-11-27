#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
    mat4 view;
	mat4 proj;
	vec4 camPos;
	vec4 camDir;
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

//instancing buffer
layout(location = 6) in vec3 inTransformPos;
layout(location = 7) in float inScale;
layout(location = 8) in float inTheta;
layout(location = 9) in vec3 inTintColor;

layout(location = 0) out vec3 vertColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 worldPosition;
layout(location = 3) out vec3 worldN;
layout(location = 4) out vec3 worldB;
layout(location = 5) out vec3 worldT;
layout(location = 6) out float vertAmbient;
layout(location = 7) out float distanceLevel;
layout(location = 8) out vec2 noiseTexCoord;
layout(location = 9) out vec3 tintColor;

out gl_PerVertex {
    vec4 gl_Position;
};

// The suggested frequencies from the Crytek paper
// The side-to-side motion has a much higher frequency than the up-and-down.
#define SIDE_TO_SIDE_FREQ1 1.475
#define SIDE_TO_SIDE_FREQ2 0.593
#define UP_AND_DOWN_FREQ1 0.275
#define UP_AND_DOWN_FREQ2 0.123

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
	float fDetailAmp		// Controls how much back and forth
	)		
{
	// Phases (object, vertex, branch)
	// fObjPhase: This ensures phase is different for different plant instances, but it should be
	// the same value for all vertices of the same plant.
	float fObjPhase = dot(objectPosition.xyz, vec3(1));  
	fBranchPhase += fObjPhase;
	float fVtxPhase = dot(vPos.xyz, vec3(fDetailPhase + fBranchPhase));  

	vec2 vWavesIn = vec2(fTime) + vec2(fVtxPhase, fBranchPhase );
	vec4 vWaves = (fract( vWavesIn.xxyy *
					   vec4(SIDE_TO_SIDE_FREQ1, SIDE_TO_SIDE_FREQ2, UP_AND_DOWN_FREQ1, UP_AND_DOWN_FREQ2) ) *
					   2.0 - 1.0 ) * fSpeed * fDetailFreq;
	vWaves = SmoothTriangleWave( vWaves );
	vec2 vWavesSum = vWaves.xz + vWaves.yw;  

	// -fBranchAtten is how restricted this vertex of the leaf/branch is. e.g. close to the stem
	//  it should be 0 (maximum stiffness). At the far outer edge it might be 1.
	//  In this sample, this is controlled by the blue vertex color.
	// -fEdgeAtten controls movement in the plane of the leaf/branch. It is controlled by the
	//  red vertex color in this sample. It is supposed to represent "leaf stiffness". Generally, it
	//  should be 0 in the middle of the leaf (maximum stiffness), and 1 on the outer edges.
	// -Note that this is different from the Crytek code, in that we use vPos.xzy instead of vPos.xyz,
	//  because I treat y as the up-and-down direction.
    // vPos.xyz += vWavesSum.x * vec3(fEdgeAtten * fDetailAmp * vNormal.xyz);
    // vPos.y += vWavesSum.y * fBranchAtten * fBranchAmp;
    vPos.xyz +=  vWavesSum.xxy * vec3(fEdgeAtten * fDetailAmp *
                            vNormal.xy, fBranchAtten * fBranchAmp);
}

mat4 rotateMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void main() {
	mat4 scale = mat4(1.0);
	mat4 rotate = rotateMatrix(vec3(0,1,0), inTheta);
	mat4 translate=mat4(1.0);
	vec3 objectPosition =vec3(inTransformPos.x, inTransformPos.y, inTransformPos.z);
	translate[3][0]=inTransformPos.x;
	translate[3][1]=inTransformPos.y;
	translate[3][2]=inTransformPos.z;

	scale[0][0] = 0.015 * inScale;
	scale[1][1] = 0.015 * inScale;
	scale[2][2] = 0.015 * inScale;
	mat4 modelMatrix = model * translate * rotate * scale;

	mat3 inv_trans_model = transpose(inverse(mat3(modelMatrix)));
	vec3 vPos=vec3(modelMatrix * vec4(inPosition, 1.0f));

	vec3 normalDir=normalize(inv_trans_model * inNormal);
	worldN = normalDir;
	worldB = normalize(inv_trans_model * inBitangent);
	worldT = normalize(inv_trans_model * inTangent);
	vertAmbient = inColor.a;


	//Wind
	vec3 wind_dir = normalize(vec3(0.5, 0, 1));
    float wind_speed = 12.0;
    float wave_division_width = 15.0;
    float wave_info = (cos((dot(objectPosition, wind_dir) - wind_speed * totalTime) / wave_division_width) + 0.7);
	
	float wind_power = 15.0f;
    //vec3 w = wind_dir * wind_power * wave_info * fd * fr;
	vec3 w=wind_dir * wind_power * wave_info;
	vec2 Wind=vec2(w.x*0.05,w.z*0.05);

	vPos -= objectPosition;	// Reset the vertex to base-zero
	float BendScale=0.024;
	ApplyMainBending(vPos, Wind, BendScale);
	vPos += objectPosition;

	float BranchAmp=0.2;
	float DetailAmp=0.1;
	
	vec2 WindDetail = vec2(w.x * 0.5,w.z*0.5);
	float windStrength = length(WindDetail);
	ApplyDetailBending(
		vPos,
		WindDetail,
		normalDir,
		objectPosition,
		0,					// Leaf phase - not used in this scenario, but would allow for variation in side-to-side motion
		inColor.g,		// Branch phase - should be the same for all verts in a leaf/branch.
		totalTime,
		inColor.r,		// edge attenuation, leaf stiffness
		1 - inColor.b,  // branch attenuation. High values close to stem, low values furthest from stem.
				// For some reason, Crysis uses solid blue for non-moving, and black for most movement.
				// So we invert the blue value here.
		BranchAmp * windStrength, // branch amplitude. Play with this until it looks good.
		1.0f,					// Speed. Play with this until it looks good.
		0.13f,					// Detail frequency. Keep this at 1 unless you want to have different per-leaf frequency
		DetailAmp * windStrength	// Detail amplitude. Play with this until it looks good.
		);

	worldPosition = vPos;

    //gl_Position = camera.proj * camera.view * model * vec4(inPosition, 1.0);
	gl_Position = camera.proj * camera.view  * vec4(vPos, 1.0);
	
    vertColor = vec3(inColor);
    fragTexCoord = inTexCoord;

//LOD Effect
	noiseTexCoord.x = (vPos.x - 0.0) / 10.5f + 0.5f;
	noiseTexCoord.y = (vPos.y - 0.0) / 20.2f;
	distanceLevel = length(vec2(camera.camPos.x, camera.camPos.z)) / (1000.0f);
	
// Tint Color
	tintColor = inTintColor;
}
