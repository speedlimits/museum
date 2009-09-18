// Example GLSL program for skinning with two bone weights per vertex

attribute vec4 blendIndices;
attribute vec4 blendWeights;

// 3x4 matrix, passed as vec4's for compatibility with GL 2.0
// GL 2.0 supports 3x4 matrices
// Support 24 bones ie 24*3, but use 72 since our parser can pick that out for sizing
uniform vec4 worldMatrix3x4Array[105];
uniform mat4 worldViewProj;
uniform vec4 lightPos;
uniform vec4 eyePosition;
uniform vec4 lightDiffuseColour;

void main()
{
	vec3 blendPos = vec3(0,0,0);
	vec3 blendNorm = vec3(0,0,0);
	
	for (int bone = 0; bone < 2; ++bone)
	{
		// perform matrix multiplication manually since no 3x4 matrices
        // ATI GLSL compiler can't handle indexing an array within an array so calculate the inner index first
	    int idx = int(blendIndices[bone]) * 3;
        // ATI GLSL compiler can't handle unrolling the loop so do it manually
        // ATI GLSL has better performance when mat4 is used rather than using individual dot product
        // There is a bug in ATI mat4 constructor (Cat 7.2) when indexed uniform array elements are used as vec4 parameter so manually assign
		mat4 worldMatrix;
		worldMatrix[0] = worldMatrix3x4Array[idx];
		worldMatrix[1] = worldMatrix3x4Array[idx + 1];
		worldMatrix[2] = worldMatrix3x4Array[idx + 2];
		worldMatrix[3] = vec4(0);
		// now weight this into final 
	    float weight = blendWeights[bone];
		blendPos += (gl_Vertex * worldMatrix).xyz * weight;
		
		mat3 worldRotMatrix = mat3(worldMatrix[0].xyz, worldMatrix[1].xyz, worldMatrix[2].xyz);
		blendNorm += (gl_Normal * worldRotMatrix) * weight;

	}

	// apply view / projection to position
	gl_Position = worldViewProj * vec4(blendPos, 1);

	// simple vertex lighting model
	vec3 lightDir0 = normalize(
		lightPos.xyz -  (blendPos.xyz * lightPos.w));
		
	gl_FrontSecondaryColor = vec4(0);
	gl_FrontColor = vec4(0.5, 0.5, 0.5, 1.0) *0.0
		+ clamp(dot(lightDir0, blendNorm), 0.0, 1.0) * lightDiffuseColour;

	gl_TexCoord[0] = gl_MultiTexCoord0;
        vec3 oNormal=normalize(blendNorm);
	gl_TexCoord[1].xyz = oNormal;
        gl_TexCoord[1].w = 0.0;
        vec3 oEyeDirObjectSpace= eyePosition.xyz-blendPos*eyePosition.w;
        gl_TexCoord[2].xyz =oEyeDirObjectSpace;
        gl_TexCoord[2].w = 0.0;
        gl_TexCoord[3] = vec4(0);//nada
        vec3 oBinormal = normalize(cross(oEyeDirObjectSpace,oNormal));//oBinormal
        gl_TexCoord[5].xyz = oBinormal;
        gl_TexCoord[5].w = 0.0;
        gl_TexCoord[4].xyz = normalize(cross(oNormal,oBinormal));//oTangent
        gl_TexCoord[4].w = 0.0;
        gl_TexCoord[6].xyz = lightPos.xyz- (blendPos.xyz * lightPos.w);//oLightVector
        gl_TexCoord[6].w = 0.0;
        gl_TexCoord[7]=vec4(0);//blood transmission factor
        
}
