uniform vec4 height_cutoff;
uniform vec4 lightAmbient;
uniform vec4 lightDiffuse;
uniform vec4 lightSpecular;
uniform vec4 lightAttenuation;
uniform vec4 scaleBias;
uniform vec4 matDiffuse;
uniform vec4 detailSpecCoefs;

varying vec2 iTexCoord0;
varying vec3 iNormal;
varying vec3 eyeDirObjectSpace;
varying vec4  iTexCoord1sd;
varying vec3 iTangent;
varying vec3 iBinormal;
varying vec4 iLightVectorPassNumberEqualZero;
varying vec3 iPos;


  uniform sampler2D coverageMap;
  uniform sampler2D splat1Map;
  uniform sampler2D splat2Map;
  uniform sampler2D splat3Map;
  uniform sampler2D splat4Map;
  uniform sampler2D heightMap;
  uniform sampler2D detail0Map;
  uniform sampler2D detail1Map;
  uniform sampler2D detail2Map;
  uniform sampler2D detail3Map;
  uniform sampler2D normal0Map;
  uniform sampler2D normal1Map;
  uniform sampler2D normal2Map;
  uniform sampler2D normal3Map;
vec3 expand(vec2 inp) {
  return vec3(inp.x,inp.y,.75)*2.0-1.0;
}
vec3 mycross(vec3 a, vec3 b) {
  return cross(a,b);
}

void main () {
  vec4 oColor;
  vec3 iLightVector=iLightVectorPassNumberEqualZero.xyz;
  float dist = length(iLightVector);

  //light vector

  mat3 rotation=mat3(iTangent,iBinormal,iNormal);
  vec3 lightDir=normalize((rotation*iLightVector));
  vec3 eyeDir=normalize((rotation*eyeDirObjectSpace));
  vec3 halfAngle=normalize(eyeDir+lightDir);
  vec4 coverage = texture2D(coverageMap, iTexCoord0).zwyx;  
  coverage.x=1.0-smoothstep(height_cutoff.x,height_cutoff.y,iPos.y);
  coverage.y=smoothstep(height_cutoff.x,height_cutoff.y,iPos.y)+.001;
  coverage.z=smoothstep(height_cutoff.y,height_cutoff.z,iPos.y);
  coverage.w=smoothstep(.5*(height_cutoff.z+height_cutoff.y),height_cutoff.w,iPos.y);
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
  float zheight=smoothstep(0.1,0.5,1.0-iNormal.y);
  coverage.z+=zheight;
  coverage.x-=zheight*.5;
  coverage.y-=zheight*.5;
  coverage.w-=zheight*.5;
  coverage.xyw=clamp(coverage.xyw,0.0,1.0);
  coverage.y+=coverage.w;
  coverage.w=0;
  coverage/=(coverage.x+coverage.y+coverage.z+coverage.w);
  float heights=dot(texture2D(heightMap,iTexCoord1sd.zw).xyzw,coverage);//average this
  float displacement= (heights*scaleBias.x)+scaleBias.y;
  vec2 newTexCoord = ((eyeDir*displacement).xy+iTexCoord1sd.zw);

  vec3 normal=normalize(expand(
			texture2D(normal0Map,newTexCoord).xw*coverage.x+
			texture2D(normal1Map,newTexCoord).xw*coverage.y+
			texture2D(normal2Map,newTexCoord).xw*coverage.z+
			texture2D(normal3Map,newTexCoord).xw*coverage.w));
  float nDotH = dot(normal,halfAngle);
  float nDotL = dot(normal, lightDir);
  vec4 detail0 = coverage.x*texture2D(detail0Map,newTexCoord);
  vec4 detail1 = coverage.y*texture2D(detail1Map,newTexCoord);
  vec4 detail2 = coverage.z*texture2D(detail2Map,newTexCoord);
  vec4 detail3 = coverage.w*texture2D(detail3Map,newTexCoord);
  vec4 surfcolor = texture2D(splat1Map, iTexCoord1sd.xy) * detail0
      + texture2D(splat2Map, iTexCoord1sd.xy) * detail1
      + texture2D(splat3Map, iTexCoord1sd.xy) * detail2
      + texture2D(splat4Map, iTexCoord1sd.xy) * detail3;

  float att = 1./(lightAttenuation[1] + lightAttenuation[2] * dist + lightAttenuation[3] * dist * dist);//FIXME NO DROPOFF
  
  //add diffuse light from material and light
  vec4 color = (att * (lightDiffuse) * matDiffuse * (nDotL > 1.0 ? 1.0 : (nDotL < 0.0 ? 0.0 : nDotL)));
  color+=iLightVectorPassNumberEqualZero.w*(lightAmbient)+lightDiffuse.xyzz*lightDiffuse.w*att;//Only adds it when pass iteration =0;
  color*=surfcolor;
  if (nDotH>0.0) {
    vec4 specular = dot(detailSpecCoefs,coverage)*pow(nDotH,35.0)*lightSpecular*att;
  
//  nDotH=0;
    color+=specular;
  }
  oColor=color;

  gl_FragColor=oColor;
}