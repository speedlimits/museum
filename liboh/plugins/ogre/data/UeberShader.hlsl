/*
#define NUM_LIGHTS 1
#define NUM_SHADOW_LIGHTS 1
#define MAX_LIGHTS 1
#define HAVE_TEN_TEXCOORD 0
#define HAVE_LOD 0
#define DERIVATIVE_SUPPORT 0
#define USE_SHADOWS 0
#define USE_HUMP 0
#define USE_NORMAL 1
#define USE_AMBIENT 1
#define USE_AMBIENT_OCCLUSION 1
#define USE_AMBIENT_ENVIRONMENT 0
#define USE_DIFFUSE 1
#define USE_SPECULAR 1
#define USE_GLOSSINESS 0
#define START_LIGHT 0
#define COMBINE_DIFFUSE_CRUFT combineOver
#define COMBINE_DIFFUSE_DIRT combineOver
#define COMBINE_SPECULAR_CRUFT combineOver
#define COMBINE_SPECULAR_DIRT combineOver
#define USE_DIFFUSE_DIRT 1
#define USE_DIFFUSE_CRUFT 1
#define USE_SPECULAR_DIRT 1
#define USE_SPECULAR_CRUFT 1
#define USE_GLOW 1
#define USE_GLOW_DIRT 0
#define USE_GLOW_CRUFT 0
#define USE_REFLECTION 0
#define USE_SPOTLIGHT 0
#define USE_SUBSURFACE 0
#define USE_SKINNING 0
#define NUM_BONES 3
#define DEBUG 0
#define DIFFUSE_UV 0
#define DIFFUSE_CRUFT_UV 0
#define DIFFUSE_DIRT_UV 0
#define DIRT_NORMAL_UV 0
#define SPECULAR_UV 0
#define SPECULAR_CRUFT_UV 0
#define SPECULAR_DIRT_UV 0
#define NORMAL_UV 0
#define BUMP_UV 0
#define GLOW_UV 0
#define GLOW_CRUFT_UV 0
#define GLOW_DIRT_UV 0


#if DIFFUSE_UV==1||DIFFUSE_CRUFT_UV==1||DIFFUSE_DIRT_UV==1||DIRT_NORMAL_UV==1||SPECULAR_UV==1||SPECULAR_CRUFT_UV==1||SPECULAR_DIRT_UV==1||BUMP_UV==1||GLOW_UV==1||GLOW_CRUFT_UV==1||GLOW_DIRT_UV==1||NORMAL_UV==1||AMBIENT_OCCLUSION_UV==1

#endif
#if DIFFUSE_UV==2||DIFFUSE_CRUFT_UV==2||DIFFUSE_DIRT_UV==2||DIRT_NORMAL_UV==2||SPECULAR_UV==2||SPECULAR_CRUFT_UV==2||SPECULAR_DIRT_UV==2||BUMP_UV==2||GLOW_UV==2||GLOW_CRUFT_UV==2||GLOW_DIRT_UV==2||NORMAL_UV==2||AMBIENT_OCCLUSION_UV==2

#endif
*/

#define USE_MAC 0

/* Offset mapping including a shadow element and multiple lights in one pass */
#if USE_GLSL!=0
#if (HAVE_LOD!=0 || DERIVATIVE_SUPPORT!=0)
#if USE_FRAGMENT_SHADER!=0
#extension GL_ATI_shader_texture_lod : enable 
#extension GL_ARB_shader_texture_lod : enable 
#endif
#endif
#define float4 vec4
#define float3 vec3
#define float2 vec2
#define float3x3 mat3
#define float4x4 mat4
#define uvBase gl_MultiTexCoord0
#define uvDirt gl_MultiTexCoord1
#define uvCruft gl_MultiTexCoord2
#define iUv oUv
#define iSurfacePosCruftU oSurfacePosCruftU
#define iTangent oTangent
#define iNormalCruftV oNormalCruftV
#define iShadowUV oShadowUV
#define iBinormal oBinormal
#define iEyeDirTanSpace oEyeDirTanSpace
#define ddx dFdx
#define ddy dFdy
#define tex2D texture2D
#define texCUBE textureCube
#define texCUBElod textureCubeLod
#define tex2Dlod texture2DLod
#define tex2Dproj texture2DProj
#define iNormal gl_Normal
#define iPosition gl_Vertex
#define oPosition gl_Position
#define oColour gl_FragColor
#define mul multipl
vec4 mul(mat4 trans, vec4 pos) {
  return trans*pos;
}
vec3 mul(mat3 trans, vec3 pos) {
  return trans*pos;
}
vec3 mulRotate(mat4 trans, vec3 pos){
  return (trans*vec4(pos.xyz,0)).xyz;
}
float saturate(float val) {
  return clamp(val,0.0,1.0);
}

#else
float3 mulRotate(float4x4 trans, float3 pos){
  return mul((float3x3)trans,pos);
}

#endif

float3 expand(float2 inp, float normalscale);

float3 combineAlpha(float3 source, float3 dest, float alpha, float aoalpha) {
    alpha *= aoalpha;

    // transform source_normal into the space of dest_normal    
    float3 tangent = float3(1.0,0.0,0.0);
    if (distance(tangent, dest) < 0.01)
        tangent = float3(0.0,0.0,-1.0);
    float3 binormal = cross(dest, float3(1.0,0.0,0.0));
    tangent = cross(binormal, dest);

    // scale source_normal by alpha
    source.xyz = float3(0.0,0.0,1.0)*(1.-alpha) + source.xyz*alpha;

    float3 normal = (source.xxx*tangent.xyz +
                     source.yyy*binormal.xyz +
                     source.zzz*dest.xyz);

    return normal;
}

float3 combineAdd(float3 source, float3 dest, float alpha, float aoalpha) {
    return combineAlpha(source,dest,1.0,1.0);
}
float3 combineAddAlpha(float3 source, float3 dest, float alpha, float aoalpha) {
    return combineAlpha(source,dest,1.0,aoalpha);
}
float4 combineOver(float4 under, float4 over, float aoalpha) {
 // return float4(under.xyz*(1.0-aoalpha*over.w),1.0);
  return float4(over.xyz*aoalpha*over.w+under.xyz*(1.0-aoalpha*over.w),1.0);
  return float4(over.xyz*aoalpha+under.xyz*(1.0-aoalpha*over.w),under.w+(1.0-under.w)*over.w*aoalpha);
}
float4 combineMultiplyAlpha(float4 under, float4 over, float aoalpha) {
  return under*(1.0-aoalpha+aoalpha*over);
}
float4 combineOverlayAlpha(float4 under, float4 over, float aoalpha) {
  over=.5*(1.0-aoalpha)+aoalpha*over;//average with .5;
#if USE_GLSL!=0
  //return under<.5?2*under*over:(1-2*(1-under)*(1-over));
  return mix(2.*under*over,(1.-2.*(1.-under)*(1.-over)),vec4(lessThan(under,vec4(.5))));
#else
  return under<.5?2*under*over:(1-2*(1-under)*(1-over));
#endif
}
float4 combineMultiply(float4 under, float4 over, float aoalpha) {
  return float4(combineMultiplyAlpha(under,over,aoalpha).xyz,under.a);
}
float4 combineOverlay(float4 under, float4 over, float aoalpha) {
  return float4(combineOverlayAlpha(under,over,aoalpha).xyz,under.a);
}
#if USE_GLSL!=0
varying vec4 oUv;
varying vec4 oSurfacePosCruftU;
varying vec4 oTangent;
varying vec4 oShadowUV
#if NUM_SHADOW_LIGHTS>1
                        [ NUM_SHADOW_LIGHTS]
#endif
;
varying vec4 oNormalCruftV;
#if (NUM_SHADOW_LIGHTS<4 || USE_SHADOWS==0 || (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<6)) && USE_HUMP!=0
varying vec3 oEyeDirTanSpace;
#endif
#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0|| (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5))||(NUM_LIGHTS==3&&USE_HUMP==0)
varying vec3 oBinormal;
#endif
#endif

//---------- Vertex shader ---------

#if USE_VERTEX_SHADER!=0 || USE_GLSL==0

#if USE_GLSL!=0
uniform mat4 worldViewProj;
#if USE_SKINNING==0
uniform mat4 worldMatrix;
#endif
#if NUM_SHADOW_LIGHTS>1
uniform mat4 texViewProjArray[NUM_SHADOW_LIGHTS];
#else
uniform mat4 texViewProj;
#endif
attribute vec4 tangent;
#if USE_SKINNING!=0
attribute vec4 blendIndices;
attribute vec4 blendWeights;
uniform vec4 worldMatrix3x4Array[132];
#endif
void main_vp()
#else
void main_vp(float4 iPosition   : POSITION,
        float3 iNormal      : NORMAL,
        float2 uvBase         : TEXCOORD0,
        float2 uvDirt     : TEXCOORD1,
        float2 uvCruft    : TEXCOORD2,
        float4 tangent     : TANGENT0,
#if USE_SKINNING!=0
        float4 blendIndices: BLENDINDICES,
        float4 blendWeights: BLENDWEIGHT,
#endif
              // outputs
        out float4 oPosition    : POSITION,
	//hide dirtmap uv in UV's zw values
        out float4 oUv          : TEXCOORD0,
	//hide cruftmap u coord in w
	out float4 oSurfacePosCruftU: TEXCOORD1,
	out float4 oTangent :TEXCOORD2,
	//hide cruftmap v coord in w
	out float4 oNormalCruftV :TEXCOORD3,
#if USE_SHADOWS!=0
	out float4 oShadowUV
#if NUM_SHADOW_LIGHTS>1
                             [NUM_SHADOW_LIGHTS]
#endif
                                                  : TEXCOORD4,
#endif
#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0 || (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5))||(NUM_LIGHTS==3&&USE_HUMP==0)
        out float3 oBinormal:
#if (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5)
                TEXCOORD9,
#else
#if NUM_LIGHTS==3&&USE_HUMP==0
		TEXCOORD7,
#else
		TEXCOORD6,
#endif
#endif
#endif
#if (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<6) && USE_HUMP!=0
        out float3 oEyeDirTanSpace:TEXCOORD8,
#else
#if (NUM_SHADOW_LIGHTS<4 || USE_SHADOWS==0) && USE_HUMP!=0
        out float3 oEyeDirTanSpace:TEXCOORD7,
#endif
#endif
              // parameters
    uniform float3 spotlight_direction
#if NUM_LIGHTS>1
[MAX_LIGHTS] // object space
#endif
        ,uniform float4x4 worldViewProj
#if USE_SKINNING==0
	,uniform float4x4 worldMatrix
#else
#if USE_MAC==0
        ,uniform float4 worldMatrix3x4Array[132]
#else
        ,uniform float3x4 worldMatrix3x4Array[44]
#endif
#endif
#if NUM_SHADOW_LIGHTS>1
	,uniform float4x4 texViewProjArray[NUM_SHADOW_LIGHTS]
#else
	,uniform float4x4 texViewProj
#endif
        )
#endif

{
#if USE_SKINNING==0
   float4 worldPosition = mul(worldMatrix, iPosition);
#if USE_GLSL!=0
   float3x3 worldRotMatrix = float3x3(worldMatrix[0].xyz, worldMatrix[1].xyz, worldMatrix[2].xyz);
#else
   float3x3 worldRotMatrix = (float3x3)worldMatrix;
#endif
   float3 worldNormal=normalize(mul(worldRotMatrix,iNormal));
   float3 worldTangent=normalize(mul(worldRotMatrix,tangent.xyz));
   oPosition=mul(worldViewProj,iPosition);
#else
#if USE_GLSL!=0
   float4 worldPosition=float4(0,0,0,1);
   float3 worldNormal=float3(0,0,0);
   float3 worldTangent=float3(0,0,0);
   {
      for (int bone = 0; bone < NUM_BONES; ++bone) {
         // perform matrix multiplication manually since no 3x4 matrices
         // ATI GLSL compiler can't handle indexing an array within an array so calculate the inner index first
         int idx = int(blendIndices[bone]) * 3;
         // ATI GLSL has better performance when mat4 is used rather than using individual dot product
         // There is a bug in ATI mat4 constructor (Cat 7.2) when indexed uniform array elements are used as vec4 parameter so manually assign
         float4x4 worldMatrix;
         worldMatrix[0] = worldMatrix3x4Array[idx];
         worldMatrix[1] = worldMatrix3x4Array[idx + 1];
         worldMatrix[2] = worldMatrix3x4Array[idx + 2];
         worldMatrix[3] = float4(0,0,0,0);
         // now weight this into final 
         float weight = blendWeights[bone];
         float3 worldPosTmp=float3 (dot(worldMatrix3x4Array[idx],iPosition),
                                        dot(worldMatrix3x4Array[idx+1],iPosition),
                                        dot(worldMatrix3x4Array[idx+2],iPosition));
         worldPosition.xyz += worldPosTmp*weight;
         float3 worldNormalTmp=float3(dot(worldMatrix3x4Array[idx].xyz,iNormal.xyz),
                                        dot(worldMatrix3x4Array[idx+1].xyz,iNormal.xyz),
                                        dot(worldMatrix3x4Array[idx+2].xyz,iNormal.xyz))*weight;
         worldNormal += worldNormalTmp * weight;
         float3 worldTangentTmp=float3(dot(worldMatrix3x4Array[idx].xyz,tangent.xyz),
                                        dot(worldMatrix3x4Array[idx+1].xyz,tangent.xyz),
                                        dot(worldMatrix3x4Array[idx+2].xyz,tangent.xyz))*weight;
         worldTangent += worldTangentTmp * weight;
      }
      // apply view / projection to position
    }
#else
#if USE_MAC==0
   float4 worldPosition=float4(0,0,0,0);
   float3 worldNormal=float3(0,0,0);
   float3 worldTangent=float3(0,0,0);
   {
      for (int bone = 0; bone < NUM_BONES; ++bone) {
         // perform matrix multiplication manually since no 3x4 matrices
         // ATI GLSL compiler can't handle indexing an array within an array so calculate the inner index first
         int idx = int(blendIndices[bone]) * 3;
         // ATI GLSL has better performance when mat4 is used rather than using individual dot product
         // There is a bug in ATI mat4 constructor (Cat 7.2) when indexed uniform array elements are used as vec4 parameter so manually assign
         float4x4 worldMatrix;
         worldMatrix[0] = worldMatrix3x4Array[idx];
         worldMatrix[1] = worldMatrix3x4Array[idx + 1];
         worldMatrix[2] = worldMatrix3x4Array[idx + 2];
         worldMatrix[3] = float4(0,0,0,0);
         // now weight this into final 
         float weight = blendWeights[bone];
         worldPosition += float4(mul( worldMatrix,iPosition).xyz,1.0) * weight;
         float3x3 worldRotMatrix = float3x3(worldMatrix[0].xyz, worldMatrix[1].xyz, worldMatrix[2].xyz);
         worldNormal += mul(worldRotMatrix,iNormal) * weight;
         worldTangent += mul(worldRotMatrix,tangent.xyz) * weight;
      }
      // apply view / projection to position
    }
#else
   float4 worldPosition=float4(0,0,0,0);
   float3 worldNormal=float3(0,0,0),worldTangent=float3(0,0,0);
       worldPosition += float4(mul(worldMatrix3x4Array[blendIndices[0]],iPosition),1) * blendWeights[0];
       worldNormal += mul((float3x3)worldMatrix3x4Array[blendIndices[0]],iNormal) * blendWeights[0];
       worldTangent += mul((float3x3)worldMatrix3x4Array[blendIndices[0]],tangent.xyz) * blendWeights[0];
       float bw1=blendWeights[1];
       worldPosition += float4(mul(worldMatrix3x4Array[blendIndices[1]],iPosition),1) * bw1;//blendWeights[1];
       worldNormal += mul((float3x3)worldMatrix3x4Array[blendIndices[1]],iNormal) * bw1;//blendWeights[1];
       worldTangent += mul((float3x3)worldMatrix3x4Array[blendIndices[1]],tangent.xyz) * bw1;//blendWeights[1];

       float bw=1.0-blendWeights[0]-blendWeights[1];
       worldPosition += float4(mul(worldMatrix3x4Array[blendIndices[2]],iPosition),1) * bw;
       worldNormal += mul((float3x3)worldMatrix3x4Array[blendIndices[2]],iNormal) * bw;
       worldTangent += mul((float3x3)worldMatrix3x4Array[blendIndices[2]],tangent.xyz) * bw;
#endif
#endif
   oPosition = mul(worldViewProj, worldPosition);
#endif
   //save position in world space relative to camera center
   oSurfacePosCruftU.xyz=worldPosition.xyz/worldPosition.w;
   // pass the main uvs straight through unchanged
   oUv = float4(uvBase.xy,uvDirt.xy);
   oSurfacePosCruftU.w=uvCruft.x;
   oNormalCruftV.w=uvCruft.y;
#if DEBUG!=0
   oSurfacePosCruftU.w=uvBase.x*.5;
   oNormalCruftV.w=uvBase.y*.5;
   oUv.zw=uvBase.xy;
#endif
   //Find normal and tangent in world space to let us do reflection mapping,
   //and avoid light transforms to object or tangent space
   oNormalCruftV.xyz=worldNormal;
   oTangent.xyz=worldTangent;
   oTangent.w=tangent.w;
   {
      // Compute world binormal for purpose of transforming tangent space eye direction
      // (NB we assume both normal and tangent are already normalised)
      // NB looks like nvidia cross params are BACKWARDS to what you'd expect
      // this equates to NxT, not TxN
      float3 worldBinormal= cross(worldTangent.xyz, worldNormal);
      if (tangent.w<0.0) worldBinormal=-worldBinormal;
#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0|| (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5))||(NUM_LIGHTS==3&&USE_HUMP==0)
     oBinormal.xyz=worldBinormal.xyz;
#endif
   }
#if USE_GLSL!=0
   int index;
#else
   float index;
#endif
#if USE_SHADOWS!=0
   for (index=0;index<NUM_SHADOW_LIGHTS;++index) {
     //per-light projective texture transform in Vertex program
#if NUM_SHADOW_LIGHTS>1
     oShadowUV[index] = mul(texViewProjArray[index], worldPosition);
#else
     oShadowUV = mul(texViewProj, worldPosition);

#endif
   }
#endif

    // Normalize outputs
    oTangent.xyz = normalize(oTangent.xyz);
    oNormalCruftV.xyz = normalize(oNormalCruftV.xyz);
#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0 || (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5))||(NUM_LIGHTS==3&&USE_HUMP==0)
    oBinormal = normalize(oBinormal);
#endif
}
#endif

//------------ Fragment shader
// Expand a range-compressed vector
float3 expand(float2 inp, float normalscale) {
  float3 retval;
  retval.xy=inp.xy*2.0*normalscale-normalscale;
  retval.z=0.0;
  float val=dot(retval.xy,retval.xy);
  float isOk=val<1.0?1.0:0.0;
  if (isOk!=0.0)
    val=1.0-val;
  val=sqrt(val);
  if (isOk!=0.0)
    retval.z=val;
  else
    retval.xy/=val;
  return retval;
}

//column major r0c0r1c0r0c1r1c1
float4 texToDirtSpace(float4 dDirt, float4 dTex, float determinant) {
  return (dDirt.xyzw*dTex.wwxx-dDirt.zwxy*dTex.yyzz)/determinant;
}


#if USE_FRAGMENT_SHADER!=0 || USE_GLSL==0
float selfshadowStep(float VNdotL) {
#if USE_GLSL!=0||NUM_LIGHTS>1||HAVE_TEN_TEXCOORDS==1
 return smoothstep(0.0,0.25,VNdotL); 
#else
 if (VNdotL>.125) return 1.0;
 return 0.0;
#endif
} // costly but soft and nice selfshadow function
float shininessMap(float shininess, float4 specmap) { return specmap.a*shininess; } // alpha-based shininess modulation
//float shininessMap(float shininess, float4 specmap) { return clamp(dot(specmap.rgb,vec3(1.0/3.0))*shininess,1.0,256.0); } // luma-based shininess modulation
float shininess2Lod(float shininess) {
  return max(0.0,9.0-log2(shininess+1.0));//+3.0*(1.0+.5/*FIXME assuming envColor global is .5 ask Klauss??envColor.a*/);
}

//texture_viewproj_matrix_array
//texture_worldviewproj_matrix_array
float3 lightCalculation(float3 surfacePosition,
                        float3 normalWorld,
                        float3 vertexNormal,
                        float  ambientShadow,
                        float3 reflectedEye,
                        float4 light_position,
                        float4 derived_light_diffuse_color,
                        float4 derived_light_specular_color,
                        float4 light_attenuation,
#if USE_SPOTLIGHT!=0
                        float3 spotlight_direction,
                        float4 spotlight_params,
#endif
                        float3 diffuse,
                        float  shininess,
                        float4 specular,
                        float  bumpShadow
) {
    float3 lightVector=light_position.xyz-surfacePosition*light_position.w;
    float lightVectorLenSqr=dot(lightVector,lightVector);
    float lightVectorLen=sqrt(lightVectorLenSqr);
    float3 lightDir=lightVector/lightVectorLen;
    
    float attenuation=dot(float3(1,lightVectorLen,lightVectorLenSqr),light_attenuation.yzw)+.0001;
    float NdotL = dot(normalWorld,lightDir);
    float VNdotL= dot(vertexNormal,lightDir);
    float RdotL = dot(reflectedEye,lightDir);
    float selfshadow=selfshadowStep(VNdotL);

    // Diffuse lighting
    float3 lightcolor=clamp(NdotL,0.0,1.0)*derived_light_diffuse_color.xyz*diffuse

    // Specular lighting    
                      +pow(clamp(RdotL,0.0,1.0),
#if USE_GLOSSINESS!=0
                          shininessMap(shininess,specular)
#else
                          shininess
#endif
                          )*derived_light_specular_color.xyz*specular.xyz;
    
#if USE_SPOTLIGHT!=0
    {
       float cosLightAngle=saturate(dot(-spotlight_direction,lightDir));
       float factor=/*pow((*/saturate((cosLightAngle-spotlight_params.y)/(spotlight_params.x-spotlight_params.y));//,spotlight_params.z);
       if (spotlight_params.z>0.0) 
          lightcolor*=pow(factor,spotlight_params.z);
    }  
#endif

    // Self-shadow from parallax mapping
    lightcolor *= bumpShadow;

    // Shadow lighting
    float brightness = max(max(derived_light_diffuse_color.x,
                               derived_light_diffuse_color.y),
                               derived_light_diffuse_color.z);

    float3 shadowColor = brightness * float3(0.31, 0.27, 0.37);
    float3 shadow = saturate(-NdotL) * shadowColor * diffuse * derived_light_specular_color.w;

    // Ambient lighting
    float3 ambient=derived_light_diffuse_color.xyz*diffuse*derived_light_diffuse_color.w;
    
    return (lightcolor*selfshadow + shadow + ambient*ambientShadow)/attenuation;
}

#if USE_HUMP != 0

// myTex2D()
// Calls the appropriate texture lookup function depending on shader language.
// (reason for this is that GLSL's texture2DGrad() isn't fully supported yet)
float4 myTex2D(sampler2D    sampler_,
               float2       UV,
               float2       dx,
               float2       dy
              )
{
#if USE_GLSL != 0
    return tex2Dlod(sampler_, UV, 0.0);
#else
    return tex2D(sampler_, UV, dx, dy);
#endif
}

// parallaxMap()
// Computes a new texture coordinate to simulate parallax on a height map.
float2 parallaxMap(float2       stepUV,                 // texture coordinate
                   sampler2D    bumpMap,                // bump map
                   float3       eyeDir,                 // eye vector in tangent space
                   float4       bumpScaleBiasNormScale, // bump map scale
                   float2       dtexdx,                 // texture coordinate derivatives in the x
                   float2       dtexdy,                 // and y directions
                   float4       numParallaxSteps        // number of steps for parallax and self-shadow
                  )
{
    eyeDir = normalize(eyeDir);

    int maxSteps = int(numParallaxSteps.x * bumpScaleBiasNormScale.y);
    int minSteps = int(float(maxSteps) / 4.0);
    
    // sample rate determined by viewing angle
    // (cast to int for loop variable, then cast back to float)
    int numSteps = int(float(maxSteps) - eyeDir.z * float(maxSteps - minSteps));
    if (numSteps < minSteps) numSteps = minSteps;
    if (numSteps > maxSteps) numSteps = maxSteps;
    float numStepsA = float(numSteps);

    float height = 1.0;
    float step = height / numStepsA;

    // calculate distance moved each step using eye vector
    float2 delta = (-eyeDir.xy * bumpScaleBiasNormScale.x) / (eyeDir.z * numStepsA);
    
    // start at negative height (0.5 * scale)
    stepUV += delta * numStepsA * -0.5;
    
    float mapHeight = myTex2D(bumpMap, stepUV, dtexdx, dtexdy).x;

    // find crossing of eye vector with height map
    int stepped = 0;

    for (int i = 0; i < numSteps; i++) {
        if (mapHeight >= height) break;
        height -= step;
        stepUV += delta;
        mapHeight = myTex2D(bumpMap, stepUV, dtexdx, dtexdy).x;
        stepped = 1;
    }

    // approximate previous segment of height map as a line and find intersection
    if (stepped == 1) {
        float2 lastUV = stepUV - delta;
        float lastHeight = myTex2D(bumpMap, lastUV, dtexdx, dtexdy).x;
        float diff1 = (height + step) - lastHeight;
        float diff2 = (mapHeight - height);

        float fraction = diff1 / (diff1 + diff2);
        if (fraction >= 0.0 && fraction <= 1.0)
            stepUV = lastUV + delta * fraction;
    }
    
    return stepUV;
}

// selfShadow()
// Determines whether a pixel is self-shadowed in a height map. Returns a float
// to be multiplied by lighting result.
float selfShadow(float2     shadowUV,               // texture coordinate
                 float3     pixelPos,               // pixel position in world space
                 float3     lightPosWS,             // light position in world space
                 sampler2D  bumpMap,                // bump map
                 float4     bumpScaleBiasNormScale, // bump map scale
                 float4     iTangent,
                 float3     iBinormal,
                 float3     vertexNormal,
                 float2     dtexdx,                 // texture coordinate derivatives in the x
                 float2     dtexdy,                 // and y directions
                 float4	    numParallaxSteps        // number of steps for parallax and self-shadow
                )
{
    float shadowCoef = 1.0;
    
    // calculate light vector in tangent space
    float3 lightDirWS = normalize(lightPosWS - pixelPos);

    float3 lightDirTS = normalize(float3(
                            dot(iTangent.xyz,       lightDirWS),
                            dot(iBinormal,          lightDirWS),
                            dot(vertexNormal.xyz,  lightDirWS)));

    // only calculate if light is facing the polygon
    if (lightDirTS.z > 0.0) 
    {
        shadowCoef = 0.0;
        
        float softening = 0.7;
        float height = myTex2D(bumpMap, shadowUV, dtexdx, dtexdy).x;
        float2 lightRay = lightDirTS.xy * bumpScaleBiasNormScale.x;
        
        int numSteps = int(numParallaxSteps.y);
        float step = 1.0 / float(numSteps);
        
        // sample points along the light ray
        for (int i = 1; i < numSteps; i++) {
            float offset = step * float(i);
            float sh = (myTex2D(bumpMap, shadowUV + lightRay * offset, dtexdx, dtexdy).x - height
                        - offset) * softening * 14.0 * (1.0 - offset);
            
            shadowCoef = max(shadowCoef, sh);
        }
        
        shadowCoef = 1.0 - shadowCoef;
        
        if (shadowCoef > 1.0) shadowCoef = 1.0;
        if (shadowCoef < 0.0) shadowCoef = 0.0;
        
        shadowCoef = shadowCoef * 0.6 + 0.4;
    }
    
    return shadowCoef;
}
#endif


#if USE_GLSL!=0

uniform sampler2D ambientOcclusionMap;
uniform sampler2D diffuseMap;
uniform sampler2D diffuseDirtMap;
#if USE_AMBIENT_ENVIRONMENT==0
uniform sampler2D diffuseCruftMap;
#else
uniform samplerCube diffuseEnvironmentMap;
#endif
#if USE_DIRT_NORMAL&&((USE_AMBIENT_ENVIRONMENT==0&&USE_DIFFUSE_CRUFT==0)||(USE_SPECULAR_CRUFT==0&&USE_GLOW==0)||(USE_SPECULAR_CRUFT==0&&USE_REFLECTION==0)||(USE_GLOW==0&&USE_REFLECTION==0))
uniform sampler2D dirtNormalMap;
#endif
uniform sampler2D bumpMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;
uniform sampler2D specularDirtMap;
#if USE_REFLECTION==0||USE_GLOW==0
uniform sampler2D specularCruftMap;
#endif

uniform samplerCube reflectionMap;
uniform sampler2D glowMap;
#if NUM_SHADOW_LIGHTS>1
uniform sampler2D shadowMap[NUM_SHADOW_LIGHTS];
#else
uniform sampler2D shadowMap;
#endif
uniform float3 derived_ambient_color;
uniform vec4 colorVariance;
#if USE_HUMP!=0
uniform vec4 numParallaxSteps;
#endif
#if NUM_LIGHTS<=1
uniform float4 derived_light_diffuse_color;
uniform float4 derived_light_specular_color;
uniform float4 light_position;
uniform float4 light_attenuation;
#else
uniform float4 derived_light_diffuse_color [MAX_LIGHTS];
uniform float4 derived_light_specular_color[MAX_LIGHTS];
uniform float4 light_position[MAX_LIGHTS];
uniform float4 light_attenuation[MAX_LIGHTS];
#endif

uniform float3 derived_ambient_light_color;
  //world space light positions
uniform float4 surfaceSpecularEmissive;
uniform float4 bumpScaleBiasNormScale;
#if USE_SUBSURFACE!=0
uniform float4 bloodTransmissionFactor;
#endif
#if USE_SPOTLIGHT!=0
#if NUM_LIGHTS<=1
uniform float4 light_spotlight_params;
uniform float3 spotlight_direction;
#else
uniform float4 light_spotlight_params[MAX_LIGHTS];
uniform float3 spotlight_direction[MAX_LIGHTS];
#endif
#endif
uniform vec4 eyePosition;   // object space


void main_fp() 
#else
void main_fp(
  float4 iUv : TEXCOORD0,
  float4 iSurfacePosCruftU : TEXCOORD1,
  float4 iTangent : TEXCOORD2
  ,float4 iNormalCruftV : TEXCOORD3
#if USE_SHADOWS!=0
  ,float4 iShadowUV 
#if NUM_SHADOW_LIGHTS>1
                   [NUM_SHADOW_LIGHTS]
#endif
                      : TEXCOORD4
#endif

#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0||(HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5)) || (NUM_LIGHTS==3&&USE_HUMP==0)
  ,float3 iBinormal:
#if (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5)
                TEXCOORD9
#else
#if NUM_LIGHTS==3&&USE_HUMP==0
		TEXCOORD7
#else
		TEXCOORD6
#endif
#endif
#endif
#if (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<6) && USE_HUMP!=0
  ,float3 iEyeDirTanSpace:TEXCOORD8
#else
#if (NUM_SHADOW_LIGHTS<4 || USE_SHADOWS==0) && USE_HUMP!=0
  ,float3 iEyeDirTanSpace:TEXCOORD7
#endif
#endif
  ,uniform float4 derived_light_diffuse_color
#if NUM_LIGHTS>1
                                            [MAX_LIGHTS]
#endif
  ,uniform float4 derived_light_specular_color
#if NUM_LIGHTS>1
                                            [MAX_LIGHTS]
#endif
  //world space light positions
  ,uniform float4 light_position
#if NUM_LIGHTS>1
                                            [MAX_LIGHTS]
#endif
  ,uniform float4 light_attenuation
#if NUM_LIGHTS>1
                                            [MAX_LIGHTS]
#endif
  ,uniform float4 surfaceSpecularEmissive,
#if USE_SUBSURFACE!=0
  uniform float4 bloodTransmissionFactor,
#endif
  uniform float3 derived_ambient_color,
  uniform float4 bumpScaleBiasNormScale
#if USE_SPOTLIGHT!=0
  ,uniform float3 spotlight_direction
#if NUM_LIGHTS>1
                                            [MAX_LIGHTS]
#endif
  ,uniform float4 light_spotlight_params
#if NUM_LIGHTS>1
                                            [MAX_LIGHTS]
#endif
#endif
  ,uniform float4 eyePosition,   // object space
  uniform float4 colorVariance,
#if USE_HUMP!=0
  uniform float4 numParallaxSteps,
#endif
  uniform sampler2D ambientOcclusionMap: register(s0),
  uniform sampler2D diffuseMap : register(s1),  
  uniform sampler2D diffuseDirtMap : register(s2),  
#if USE_AMBIENT_ENVIRONMENT==0
#if (USE_AMBIENT_ENVIRONMENT==0&&USE_DIFFUSE_CRUFT==0&&USE_DIRT_NORMAL!=0)
  uniform sampler2D dirtNormalMap: register(s3),
#else
  uniform sampler2D diffuseCruftMap : register(s3),  
#endif
#else
  uniform samplerCUBE diffuseEnvironmentMap : register(s3),  
#endif
  uniform sampler2D bumpMap : register(s4),
  uniform sampler2D normalMap : register(s5),
  uniform sampler2D specularMap :register(s6),
  uniform sampler2D specularDirtMap :register(s7),
#if USE_REFLECTION==0 || USE_GLOW==0
#if USE_DIRT_NORMAL!=0&&USE_SPECULAR_CRUFT==0&&(USE_AMBIENT_ENVIRONMENT!=0||USE_DIFFUSE_CRUFT!=0)
  uniform sampler2D dirtNormalMap :register(s8),
#else
  uniform sampler2D specularCruftMap :register(s8),
#endif
#else
  uniform sampler2D glowMap :register(s8),
#endif
#if USE_REFLECTION!=0
  uniform samplerCUBE reflectionMap: register(s9),
#else
#if USE_GLOW==0&&USE_DIRT_NORMAL!=0&&(USE_AMBIENT_ENVIRONMENT!=0||USE_DIFFUSE_CRUFT!=0)&&USE_SPECULAR_CRUFT!=0
  uniform sampler2D dirtNormalMap:register(s9),
#else
  uniform sampler2D glowMap:register(s9),
#endif
#endif
#if USE_SHADOWS!=0 && NUM_SHADOW_LIGHTS>1
  uniform sampler2D shadowMap[NUM_SHADOW_LIGHTS] : register(s10),
#else
  uniform sampler2D shadowMap : register(s10),
#endif
  out float4 oColour : COLOR)

#endif
{

    int i, j;
   float3 eyePos=iSurfacePosCruftU.xyz-eyePosition.xyz;
   vec3 vertexNormal=iNormalCruftV.xyz;
//#if USE_TWO_SIDED!=0
   if (dot(vertexNormal,eyePos)>0.0) {
      vertexNormal=-vertexNormal;
   }
//#endif

#if !((NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0||(HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5)) || (NUM_LIGHTS==3&&USE_HUMP==0))
  //recompute binormal if not passed in. Duplicates potentially #ifdef'd vp calc
  float3 iBinormal=cross(iTangent.xyz,vertexNormal.xyz);
  if (iTangent.w<0.0) iBinormal=-iBinormal;
#endif

float2 dirtOffset   = float2(0.0, 0.0);
float2 cruftOffset  = float2(0.0, 0.0);
float2 offset       = float2(0.0, 0.0);

// Calculate eye direction (in tangent space) for parallax mapping
#if USE_HUMP != 0
   float3 eyeDir=normalize(float3(dot(iTangent.xyz,-eyePos),
                                  dot(iBinormal,-eyePos),
                                  dot(vertexNormal.xyz,-eyePos)));
#endif

// Set texture coordinate for parallax mapping
// BUMP_UV should always be 0
#if BUMP_UV==0
    float2 origUV = iUv.xy;
    float2 mappedUV = origUV;
#endif

// Parallax mapping
#if USE_HUMP!=0
    float2 dtexdx = ddx(origUV);
    float2 dtexdy = ddy(origUV);
    
    if (eyeDir.z > 0.05)
        mappedUV = parallaxMap(origUV, bumpMap, eyeDir, bumpScaleBiasNormScale,
                               dtexdx, dtexdy, numParallaxSteps);
#endif

// Transform parallax offset to other textures
#if USE_HUMP!=0&&DERIVATIVE_SUPPORT!=0&&(DIFFUSE_UV==1||DIFFUSE_CRUFT_UV==1||DIFFUSE_DIRT_UV==1||DIRT_NORMAL_UV==1||SPECULAR_UV==1||SPECULAR_CRUFT_UV==1||SPECULAR_DIRT_UV==1||BUMP_UV==1||GLOW_UV==1||GLOW_CRUFT_UV==1||GLOW_DIRT_UV==1||NORMAL_UV==1||AMBIENT_OCCLUSION_UV==1||DIFFUSE_UV==2||DIFFUSE_CRUFT_UV==2||DIFFUSE_DIRT_UV==2||DIRT_NORMAL_UV==2||SPECULAR_UV==2||SPECULAR_CRUFT_UV==2||SPECULAR_DIRT_UV==2||BUMP_UV==2||GLOW_UV==2||GLOW_CRUFT_UV==2||GLOW_DIRT_UV==2||NORMAL_UV==2||AMBIENT_OCCLUSION_UV==2)

// Texture coordinate derivatives
float4 dtexdirtdx=ddx(iUv);
float4 dtexdirtdy=ddy(iUv);
// Cruft
float2 dcruftdx=ddx(float2(iSurfacePosCruftU.w,iNormalCruftV.w));
float2 dcruftdy=ddy(float2(iSurfacePosCruftU.w,iNormalCruftV.w));

if (eyeDir.z > 0.05)
{
    offset = mappedUV - origUV;
    
    // linearly diminish parallax strength as eyeDir.z approaches 0.05 from 0.1
    if (eyeDir.z < 0.1)
        offset = offset * (eyeDir.z - 0.05) / 0.05;        
    
    // Texture 0 derivatives (iUv.xy is texture 0)
    float4 dTex=float4(dtexdirtdx.xy,dtexdirtdy.xy);
 
    float determinant = dTex.x*dTex.w - dTex.y*dTex.z;
    
    // Dirt offset
    #if DIFFUSE_UV==1||DIFFUSE_CRUFT_UV==1||DIFFUSE_DIRT_UV==1||DIRT_NORMAL_UV==1||SPECULAR_UV==1||SPECULAR_CRUFT_UV==1||SPECULAR_DIRT_UV==1||BUMP_UV==1||GLOW_UV==1||GLOW_CRUFT_UV==1||GLOW_DIRT_UV==1||NORMAL_UV==1||AMBIENT_OCCLUSION_UV==1
        float4 dDirt=float4(dtexdirtdx.zw,dtexdirtdy.zw);
        float4 dirtTransform=texToDirtSpace(dDirt,dTex,determinant);
        
        dirtOffset = dirtTransform.xy*offset.xx + dirtTransform.zw*offset.yy;
    #endif
    
    // Cruft offset
    #if DIFFUSE_UV==2||DIFFUSE_CRUFT_UV==2||DIFFUSE_DIRT_UV==2||DIRT_NORMAL_UV==2||SPECULAR_UV==2||SPECULAR_CRUFT_UV==2||SPECULAR_DIRT_UV==2||BUMP_UV==2||GLOW_UV==2||GLOW_CRUFT_UV==2||GLOW_DIRT_UV==2||NORMAL_UV==2||AMBIENT_OCCLUSION_UV==2
        float4 dCruft=float4(dcruftdx,dcruftdy);
        float4 cruftTransform=texToDirtSpace(dCruft,dTex,determinant);
        
        cruftOffset = cruftTransform.xy*offset.xx + cruftTransform.zw*offset.yy;
    #endif
}
#endif

    float2 texCoords[3];
    texCoords[0] = offset + origUV;
    texCoords[1] = dirtOffset + iUv.zw;
    texCoords[2] = cruftOffset + float2(iSurfacePosCruftU.w,iNormalCruftV.w);

    float2 heightalpha = tex2D(bumpMap, mappedUV).xw;

  // get the new normal and diffuse values
  float4 dirt_diffuse_value=float4(0,0,0,0);
#if USE_AMBIENT_OCCLUSION!=0
  float4 ao=tex2D(ambientOcclusionMap, texCoords[AMBIENT_OCCLUSION_UV]);
#else
  float4 ao=float4(1.0,1.0,1.0,1.0);
#endif
#if USE_DIFFUSE!=0
  float4 diffuse =tex2D(diffuseMap, texCoords[DIFFUSE_UV]);
#else
  float4 diffuse=float4(0,0,0,1);
#endif

#if USE_DIFFUSE_DIRT!=0
  dirt_diffuse_value=tex2D(diffuseDirtMap,texCoords[DIFFUSE_DIRT_UV]);
  diffuse=COMBINE_DIFFUSE_DIRT(diffuse,dirt_diffuse_value,ao.w);
#endif

#if USE_DIFFUSE_CRUFT!=0&&USE_AMBIENT_ENVIRONMENT==0
  float4 cruft_diffuse_value=tex2D(diffuseCruftMap,texCoords[DIFFUSE_CRUFT_UV]);
 diffuse=COMBINE_DIFFUSE_CRUFT(diffuse,cruft_diffuse_value,1.0);
#endif

  diffuse.xyz*=ao.xyz;
#if USE_GLOW!=0
  float4 glow=tex2D(glowMap,texCoords[GLOW_UV]);
#if USE_DIFFUSE==0
  diffuse.w=glow.w;
#endif
#endif

#if USE_SPECULAR!=0
  float4 specular = tex2D(specularMap,texCoords[SPECULAR_UV]);
#else
  float4 specular=float4(1,1,1,1);
#endif

#if USE_SPECULAR_DIRT!=0
#if USE_DIFFUSE_DIRT!=0
  specular=COMBINE_SPECULAR_DIRT(specular,float4(tex2D(specularDirtMap,texCoords[SPECULAR_DIRT_UV]).xyz,dirt_diffuse_value.w),ao.w);
#else
  specular=COMBINE_SPECULAR_DIRT(specular,tex2D(specularDirtMap,texCoords[SPECULAR_DIRT_UV]),ao.w);
#endif
#endif

#if USE_SPECULAR_CRUFT!=0 && (USE_REFLECTION==0||USE_GLOW==0)
#if USE_DIFFUSE_CRUFT!=0
  specular=COMBINE_SPECULAR_CRUFT(specular,float4(tex2D(specularCruftMap,texCoords[SPECULAR_CRUFT_UV]).xyz,cruft_diffuse_value.w),1.0);
#else
  specular=COMBINE_SPECULAR_CRUFT(specular,tex2D(specularCruftMap,texCoords[SPECULAR_CRUFT_UV]),1.0);
#endif
#endif

  specular.xyz*=surfaceSpecularEmissive.xxx;

#if USE_NORMAL!=0
  float3 normal = expand(tex2D(normalMap, texCoords[NORMAL_UV]).wy,bumpScaleBiasNormScale.z);  
#if USE_DIRT_NORMAL&&((USE_AMBIENT_ENVIRONMENT==0&&USE_DIFFUSE_CRUFT==0)||(USE_SPECULAR_CRUFT==0&&USE_GLOW==0)||(USE_SPECULAR_CRUFT==0&&USE_REFLECTION==0)||(USE_GLOW==0&&USE_REFLECTION==0))
float3 base_normal;
float3 dirt_normal;
  {
      base_normal = expand(tex2D(normalMap, texCoords[NORMAL_UV]).wy,
          bumpScaleBiasNormScale.z);
      dirt_normal = expand(tex2D(dirtNormalMap, texCoords[DIRT_NORMAL_UV]).wy,
          bumpScaleBiasNormScale.w);

    normal = COMBINE_DIRT_NORMAL(dirt_normal,base_normal,dirt_diffuse_value.w,ao.w);
  }
#endif
  float3 normalWorld=(normal.xxx*iTangent.xyz+normal.yyy*iBinormal.xyz+normal.zzz*vertexNormal.xyz);
#else
  float3 normal = float3(0,0,1);
  float3 normalWorld=vertexNormal.xyz;
#endif

  float3 reflectedEye=normalize(reflect(eyePos.xyz,normalWorld));
  oColour=float4(0,0,0,diffuse.w*heightalpha.y);
#if USE_AMBIENT!=0
#if USE_AMBIENT_ENVIRONMENT!=0
  oColour.xyz=diffuse.xyz*texCUBE(diffuseEnvironmentMap,normalWorld).xyz;
#else
  oColour.xyz=diffuse.xyz*derived_ambient_color.xyz;
#endif
#endif
  float shininess = surfaceSpecularEmissive.y;

#if USE_REFLECTION!=0
#if USE_GLOSSINESS!=0
#if HAVE_LOD!=0
  float3 reflection=texCUBElod(reflectionMap,
#if USE_GLSL==0
            float4(
#endif
                               reflectedEye,
                               shininess2Lod(shininessMap(shininess,specular))
#if USE_GLSL==0
                               )
#endif
                               ).xyz;
#else
  float3 reflection=texCUBE(reflectionMap,
                               	reflectedEye
                                ).xyz;//,.5*shininess2Lod(shininessMap(shininess,specular))).xyz;
#endif
#else
#if HAVE_LOD!=0
  float3 reflection=texCUBElod(reflectionMap,
#if USE_GLSL==0
            float4(
#endif
                   reflectedEye,	
                   shininess2Lod(shininess)
#if USE_GLSL==0
                               )
#endif
                    ).xyz;
#else
  float3 reflection=texCUBE(reflectionMap,reflectedEye).xyz;//,.5*shininess2Lod(shininess);
#endif
#endif
  oColour.xyz+=reflection*specular.xyz;
#endif
#if USE_GLOW!=0
  oColour.xyz+=glow.xyz*surfaceSpecularEmissive.w;
#endif
  float3 surfacePosition=iSurfacePosCruftU.xyz;
  float ambientShadow=clamp(dot(vertexNormal,normalWorld),.25,1.0);
#if USE_SHADOWS!=0
  for (i=START_LIGHT;i<NUM_SHADOW_LIGHTS;++i) {
    
    oColour.xyz+=lightCalculation(surfacePosition
                              ,normalWorld
                              ,vertexNormal.xyz
                              ,ambientShadow
                              ,reflectedEye
                              ,light_position
#if NUM_LIGHTS>1
                                                          [i]
#endif
                              ,derived_light_diffuse_color
#if NUM_LIGHTS>1
                                                          [i]
#endif
                              ,derived_light_specular_color
#if NUM_LIGHTS>1
                                                           [i]
#endif
                              ,light_attenuation
#if NUM_LIGHTS>1
                                                 [i]
#endif
#if USE_SPOTLIGHT!=0
                              ,spotlight_direction
#if NUM_LIGHTS>1
                                                  [i]
#endif
                              ,light_spotlight_params
#if NUM_LIGHTS>1
                                                     [i]
#endif
#endif
                              ,diffuse.xyz
                              ,shininess
                              ,specular
#if USE_HUMP!=0
                              ,selfShadow(mappedUV, surfacePosition, light_position[i].xyz,
                                          bumpMap,bumpScaleBiasNormScale, iTangent,
                                          iBinormal, vertexNormal, dtexdx, dtexdy, numParallaxSteps)
#else
                              ,1.0
#endif
                               )*tex2Dproj(shadowMap
#if NUM_SHADOW_LIGHTS>1
                                                     [i]
#endif
                                           , iShadowUV
#if NUM_SHADOW_LIGHTS>1
                                                     [i]
#endif
                                           ).xyz
;
  }
#endif

#if NUM_LIGHTS!=0
#if USE_SHADOWS!=0
  for (i=NUM_SHADOW_LIGHTS;i<NUM_LIGHTS;++i) 
#else
  for (i=START_LIGHT;i<NUM_LIGHTS;++i) 
#endif
  {
    oColour.xyz+=lightCalculation(surfacePosition
                              ,normalWorld
                              ,vertexNormal.xyz
                              ,ambientShadow
                              ,reflectedEye
                              ,light_position
#if NUM_LIGHTS>1
                                               [i]
#endif
                              ,derived_light_diffuse_color
#if NUM_LIGHTS>1
                                               [i]
#endif

                              ,derived_light_specular_color
#if NUM_LIGHTS>1
                                               [i]
#endif
                              ,light_attenuation
#if NUM_LIGHTS>1
                                               [i]
#endif

#if USE_SPOTLIGHT!=0
                              ,spotlight_direction
#if NUM_LIGHTS>1
                                                  [i]
#endif

                              ,light_spotlight_params
#if NUM_LIGHTS>1
                                                      [i]
#endif

#endif
                              ,diffuse.xyz
                              ,shininess
                              ,specular
#if USE_HUMP!=0
                              ,selfShadow(mappedUV, surfacePosition, light_position[i].xyz,
                                          bumpMap,bumpScaleBiasNormScale, iTangent,
                                          iBinormal, vertexNormal, dtexdx, dtexdy, numParallaxSteps)
#else
                              ,1.0
#endif
                              )
;
 }

#endif  
  oColour*=colorVariance;

}
#endif
