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
#define iEyeToPosCruftU oEyeToPosCruftU
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

float3 expand(float2 inp) {
  return float3(inp.x,inp.y,.75)*2.0-1.0;
}

#if USE_GLSL!=0
varying vec4 oUv;
varying vec4 oEyeToPosCruftU;
varying vec4 oTangent;
varying vec4 oShadowUV
#if NUM_SHADOW_LIGHTS>1
                        [ NUM_SHADOW_LIGHTS]
#endif
;
varying vec4 oNormalCruftV;
#if (NUM_SHADOW_LIGHTS<4 || USE_SHADOWS==0 || (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<6)) && USE_BUMP!=0
varying vec3 oEyeDirTanSpace;
#endif
#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0|| (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5))||(NUM_LIGHTS==3&&USE_BUMP==0)
varying vec3 oBinormal;
#endif
#endif

//---------- Vertex shader ---------

#if USE_VERTEX_SHADER!=0 || USE_GLSL==0

#if USE_GLSL!=0
uniform vec3 eyePosition;   // object space
uniform mat4 worldViewProj;
uniform mat4 worldMatrix;
#if NUM_SHADOW_LIGHTS>1
uniform mat4 texViewProjArray[NUM_SHADOW_LIGHTS];
#else
uniform mat4 texViewProj;
#endif
void main_vp()
#else
void main_vp(float4 iPosition   : POSITION,
        float3 iNormal      : NORMAL,
        float2 uvBase         : TEXCOORD0,
              // outputs
        out float4 oPosition    : POSITION,
	//hide dirtmap uv in UV's zw values
        out float4 oUv          : TEXCOORD0,
	//hide cruftmap u coord in w
	out float4 oEyeToPosCruftU: TEXCOORD1,
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
#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0 || (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5))||(NUM_LIGHTS==3&&USE_BUMP==0)
        out float3 oBinormal:
#if (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5)
                TEXCOORD9,
#else
#if NUM_LIGHTS==3&&USE_BUMP==0
		TEXCOORD7,
#else
		TEXCOORD6,
#endif
#endif
#endif
#if (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<6) && USE_BUMP!=0
        out float3 oEyeDirTanSpace:TEXCOORD8,
#else
#if (NUM_SHADOW_LIGHTS<4 || USE_SHADOWS==0) && USE_BUMP!=0
        out float3 oEyeDirTanSpace:TEXCOORD7,
#endif
#endif
              // parameters
        uniform float3 eyePosition   // object space
 	,uniform float3 spotlight_direction
#if NUM_LIGHTS>1
[MAX_LIGHTS] // object space
#endif
    ,uniform float4x4 worldViewProj
	,uniform float4x4 worldMatrix
#if NUM_SHADOW_LIGHTS>1
	,uniform float4x4 texViewProjArray[NUM_SHADOW_LIGHTS]
#else
	,uniform float4x4 texViewProj
#endif
        )
#endif

{
   float4 worldPosition = mul(worldMatrix, iPosition);
#if USE_GLSL!=0
   float3x3 worldRotMatrix = float3x3(worldMatrix[0].xyz, worldMatrix[1].xyz, worldMatrix[2].xyz);
#else
   float3x3 worldRotMatrix = (float3x3)worldMatrix;
#endif
   float3 worldNormal=normalize(mul(worldRotMatrix,iNormal));
   float3 worldTangent = float3(1.0,0.0,0.0);
   oPosition=mul(worldViewProj,iPosition);

   //save position in world space relative to camera center
   oEyeToPosCruftU.xyz=worldPosition.xyz/worldPosition.w-eyePosition;
#if USE_TWO_SIDED!=0
   if (dot(oEyeToPosCruftU.xyz,worldNormal)>0)
      worldNormal=-worldNormal;
#endif
   // pass the main uvs straight through unchanged
   //oUv = float4(uvBase.xy,uvDirt.xy);
   oUv.xy = worldPosition.xz / 10.0;
   oUv.zw = worldPosition.xz / 10.0;
   oEyeToPosCruftU.w = worldPosition.x / 10.0;
   oNormalCruftV.w = worldPosition.z / 10.0;
#if DEBUG!=0
   oEyeToPosCruftU.w=uvBase.x*.5;
   oNormalCruftV.w=uvBase.y*.5;
   oUv.zw=uvBase.xy;
#endif
   //Find normal and tangent in world space to let us do reflection mapping,
   //and avoid light transforms to object or tangent space
   oNormalCruftV.xyz = worldNormal;
   oTangent.xyz = worldTangent;
   oTangent.w = 1.0;
   {
      // Compute world binormal for purpose of transforming tangent space eye direction
      // (NB we assume both normal and tangent are already normalised)
      // NB looks like nvidia cross params are BACKWARDS to what you'd expect
      // this equates to NxT, not TxN
      float3 worldBinormal= cross(worldTangent.xyz, worldNormal);
#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0|| (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5))||(NUM_LIGHTS==3&&USE_BUMP==0)
     oBinormal.xyz=worldBinormal.xyz;
#endif
   }
   
   oTangent.xyz = cross(oBinormal.xyz, worldNormal);

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
#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0 || (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5))||(NUM_LIGHTS==3&&USE_BUMP==0)
    oBinormal = normalize(oBinormal);
#endif
}
#endif

//------------ Fragment shader

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
                        float  specular
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
                      + pow(clamp(RdotL,0.0,1.0),shininess)
                      * derived_light_specular_color.xyz * specular;
    
#if USE_SPOTLIGHT!=0
    {
       float cosLightAngle=saturate(dot(-spotlight_direction,lightDir));
       float factor=/*pow((*/saturate((cosLightAngle-spotlight_params.y)/(spotlight_params.x-spotlight_params.y));//,spotlight_params.z);
       if (spotlight_params.z>0.0) 
          lightcolor*=pow(factor,spotlight_params.z);
    }  
#endif

    // Shadow lighting
    float brightness = max(max(derived_light_diffuse_color.x,
                               derived_light_diffuse_color.y),
                               derived_light_diffuse_color.z);

    float3 shadowColor = brightness * float3(0.31, 0.27, 0.37);
    float3 shadow = saturate(-NdotL) * shadowColor * diffuse * derived_light_specular_color.w;

    // Ambient lighting
    float3 ambient=derived_light_diffuse_color.xyz*diffuse*derived_light_diffuse_color.w;
    
    return (lightcolor*selfshadow + shadow + ambient)/attenuation;
}


#if USE_GLSL!=0

uniform sampler2D coverageMap;
uniform sampler2D diffuseMap0;
uniform sampler2D diffuseMap1;
uniform sampler2D diffuseMap2;
uniform sampler2D diffuseMap3; 
uniform sampler2D normalMap0;
uniform sampler2D normalMap1;
uniform sampler2D normalMap2;
uniform sampler2D normalMap3;
#if NUM_SHADOW_LIGHTS>1
uniform sampler2D shadowMap[NUM_SHADOW_LIGHTS];
#else
uniform sampler2D shadowMap;
#endif
uniform float3 derived_ambient_color;
uniform vec4 colorVariance;
#if USE_BUMP!=0
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
uniform float4 texScale;
uniform float4 specularParams0;
uniform float4 specularParams1;
#if USE_SPOTLIGHT!=0
#if NUM_LIGHTS<=1
uniform float4 light_spotlight_params;
uniform float3 spotlight_direction;
#else
uniform float4 light_spotlight_params[MAX_LIGHTS];
uniform float3 spotlight_direction[MAX_LIGHTS];
#endif
#endif
uniform vec3 eyePosition;   // object space


void main_fp() 
#else
void main_fp(
  float4 iUv : TEXCOORD0,
  float4 iEyeToPosCruftU : TEXCOORD1,
  float4 iTangent : TEXCOORD2
  ,float4 iNormalCruftV : TEXCOORD3
#if USE_SHADOWS!=0
  ,float4 iShadowUV 
#if NUM_SHADOW_LIGHTS>1
                   [NUM_SHADOW_LIGHTS]
#endif
                      : TEXCOORD4
#endif

#if (NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0||(HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5)) || (NUM_LIGHTS==3&&USE_BUMP==0)
  ,float3 iBinormal:
#if (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5)
                TEXCOORD9
#else
#if NUM_LIGHTS==3&&USE_BUMP==0
		TEXCOORD7
#else
		TEXCOORD6
#endif
#endif
#endif
#if (HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<6) && USE_BUMP!=0
  ,float3 iEyeDirTanSpace:TEXCOORD8
#else
#if (NUM_SHADOW_LIGHTS<4 || USE_SHADOWS==0) && USE_BUMP!=0
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
  ,uniform float3 derived_ambient_color,
  uniform float4 texScale,
  uniform float4 specularParams0,
  uniform float4 specularParams1
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
  ,uniform float3 eyePosition,   // object space
  uniform float4 colorVariance,
#if USE_BUMP!=0
  uniform float4 numParallaxSteps,
#endif
  uniform sampler2D coverageMap  : register(s0),
  uniform sampler2D diffuseMap0  : register(s1), 
  uniform sampler2D diffuseMap1  : register(s2),
  uniform sampler2D diffuseMap2  : register(s3),
  uniform sampler2D diffuseMap3  : register(s4), 
  uniform sampler2D normalMap0   : register(s5),
  uniform sampler2D normalMap1   : register(s6),
  uniform sampler2D normalMap2   : register(s7),
  uniform sampler2D normalMap3   : register(s8),
  out float4 oColour : COLOR)

#endif
{
    int i, j;

#if !((NUM_SHADOW_LIGHTS<3 || USE_SHADOWS==0||(HAVE_TEN_TEXCOORD!=0&&NUM_SHADOW_LIGHTS<5)) || (NUM_LIGHTS==3&&USE_BUMP==0))
  //recompute binormal if not passed in. Duplicates potentially #ifdef'd vp calc
  float3 iBinormal=cross(iTangent.xyz,iNormalCruftV.xyz);
  if (iTangent.w<0.0) iBinormal=-iBinormal;
#endif

    float3 surfacePosition=eyePosition+iEyeToPosCruftU.xyz;

    // Get coverage value for each layer
    float4 coverage = tex2D(coverageMap,iUv.xy).zwyx;

    // FIXME: fake coverage values based on height
    float4 height_cutoff = float4(2783.365,3093.67,4178.96,4481.82);
    coverage.x=1.0-smoothstep(height_cutoff.x,height_cutoff.y,surfacePosition.y);
    coverage.y=smoothstep(height_cutoff.x,height_cutoff.y,surfacePosition.y)+.001;
    coverage.z=smoothstep(height_cutoff.y,height_cutoff.z,surfacePosition.y);
    coverage.w=smoothstep(.5*(height_cutoff.z+height_cutoff.y),height_cutoff.w,surfacePosition.y);

    if (coverage.z>0.0) {
        float omz=1.0-coverage.z;
        if (coverage.x>omz) {
            coverage.x=omz;
        }
        if (coverage.y>omz) {
            coverage.y=omz;
        }
    }
    if (coverage.w>0.0) {
        float omw=1.0-coverage.w;
        if (coverage.z>omw)
        coverage.z=omw;
        if (coverage.y>omw)
            coverage.y=omw;
        if (coverage.x>omw)
            coverage.x=omw;
    }

    float zheight=smoothstep(0.1,0.5,1.0-iNormalCruftV.y);
    coverage.z+=zheight;
    coverage.x-=zheight*.5;
    coverage.y-=zheight*.5;
    coverage.w-=zheight*.5;
    coverage.xyw=clamp(coverage.xyw,0.0,1.0);
    coverage/=(coverage.x+coverage.y+coverage.z+coverage.w);
    // END FIXME

    // Scale texture coordinates
    float2 texCoords[4];
    texCoords[0] = iUv.xy * texScale.x;
    texCoords[1] = iUv.xy * texScale.y;
    texCoords[2] = iUv.xy * texScale.z;
    texCoords[3] = iUv.xy * texScale.w;
    
#if USE_DIFFUSE!=0
  float4 diffuse = coverage.x * tex2D(diffuseMap0, texCoords[0])
                 + coverage.y * tex2D(diffuseMap1, texCoords[1])
                 + coverage.z * tex2D(diffuseMap2, texCoords[2])
                 + coverage.w * tex2D(diffuseMap3, texCoords[3]);
#else
  float4 diffuse=float4(0,0,0,1);
#endif

// Calculate specular intensity
// (specular value is in diffuse map alpha channel)
#if USE_SPECULAR!=0
  float specular = diffuse.w;
#else
  float specular = 1.0;
#endif

// Calculate specular parameters
float2 specularParams = specularParams0.xy * coverage.x
                      + specularParams0.zw * coverage.y
                      + specularParams1.xy * coverage.z
                      + specularParams1.zw * coverage.w;
    
specular *= specularParams.x;

// Calculate normal
#if USE_NORMAL!=0 
  float3 normal = normalize(
      expand(tex2D(normalMap0, texCoords[0]).xy) * coverage.x +
      expand(tex2D(normalMap1, texCoords[1]).xy) * coverage.y +
      expand(tex2D(normalMap2, texCoords[2]).xy) * coverage.z +
      expand(tex2D(normalMap3, texCoords[3]).xy) * coverage.w);
  float3 normalWorld=(normal.xxx*iTangent.xyz+normal.yyy*iBinormal.xyz+normal.zzz*iNormalCruftV.xyz);
#else
  float3 normal = float3(0,0,1);
  float3 normalWorld=iNormalCruftV.xyz;
#endif

  // Reflected eye vector for specular lighting
  float3 reflectedEye=normalize(reflect(iEyeToPosCruftU.xyz,normalWorld));
  oColour=float4(0,0,0,1);

  // Ambient lighting
#if USE_AMBIENT!=0
  oColour.xyz=diffuse.xyz*derived_ambient_color.xyz;
#endif

  float shininess = specularParams.y;
  

#if USE_SHADOWS!=0
  for (i=START_LIGHT;i<NUM_SHADOW_LIGHTS;++i) {
    
    oColour.xyz+=lightCalculation(surfacePosition,
                              normalWorld,
                              iNormalCruftV.xyz,
                              reflectedEye
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
                              ,iNormalCruftV.xyz
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
                              )
;
  }
#endif  

  //oColour*=colorVariance;
  //just assign output color to diffuse map, no lighting just yet

	// factor in spotlight angle
	//float rho = saturate(dot(spotDir, lightDir));
	// factor = (rho - cos(outer/2) / cos(inner/2) - cos(outer/2)) ^ falloff
	//float spotFactor = pow(
	//	saturate(rho - spotParams.y) / (spotParams.x - spotParams.y), spotParams.z);
	//col1 = col1 * spotFactor;
	
	// shadow textures
	//col1 = col1;// * tex2Dproj(shadowMap1, iShadowUV[0]);

//  oColour=float4(light_attenuation[0].xyz/2000000.,1);
   //oColour.xyz=float3(smoothstep(.999,.9999,dot(normalize(light_position[0].xyz-surfacePosition),-normalize(spotlight_direction[0]))));
   //oColour.y=abs(oColour.y);
   //oColour.z=-oColour.z;
   //oColour=tex2D(diffuseMap,texCoords[DIFFUSE_UV]);
   //oColour=ao.wwww;
    //if (iTangent.w >= 0.0)  
    //    oColour.xyz = float3(1.0,1.0,1.0);
    //oColour.xyz = 0.5 * normalWorld + float3(0.5,0.5,0.5);
  //oColour.xyz = oColour.xyz*0.0001 + shininess / 16.0;
}
#endif
