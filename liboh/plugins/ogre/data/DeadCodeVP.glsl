
uniform mat4 worldViewProj;
uniform mat4 worldView;
uniform mat4 worldIT;
uniform vec4 splatScales;
uniform vec4 detailScales;
uniform vec4 lightPosition;
uniform vec4 passNumber;
uniform vec4 eyePosition;
varying vec2 iTexCoord0;
varying vec3 iNormal;
varying vec3 eyeDirObjectSpace;
varying vec4  iTexCoord1sd;
varying vec3 iTangent;
varying vec3 iBinormal;
varying vec4 iLightVectorPassNumberEqualZero;
varying vec3 iPos;

void main() {
  vec4 iPosition=gl_Vertex;  

  eyeDirObjectSpace = eyePosition.xyz-iPosition.xyz;
  iNormal = normalize((worldIT*vec4(gl_Normal,0.0)).xyz);
  iBinormal = normalize(cross(eyeDirObjectSpace,iNormal));
  iTangent=normalize(cross(iNormal,iBinormal));
  iTexCoord0 = gl_MultiTexCoord0.xy;
  iTexCoord1sd.xy= iTexCoord0 * splatScales.x;
  iTexCoord1sd.zw= iTexCoord0 * detailScales.x;
  //pass the object space vertex position through to the fragment shader


  iLightVectorPassNumberEqualZero.xyz=lightPosition.xyz-iPosition.xyz*lightPosition.w;
  iLightVectorPassNumberEqualZero.w=1.0;


  // transform position to projection space
  vec4 oPosition=ftransform();
  iPos=(worldView*iPosition).xyz;
  gl_Position=oPosition;

}