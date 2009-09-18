void main_fp(
    float4              iTexcoord	: TEXCOORD0,
    out float4          oColor		: COLOR,
    uniform sampler2D   textureUnit0: TEXUNIT0)
{
	oColor = tex2D(textureUnit0, iTexcoord.xy);
}

