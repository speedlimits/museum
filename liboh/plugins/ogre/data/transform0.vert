void main_vp
(
    float4             iPosition           : POSITION,
    float4x4            worldViewProj,
	float4             iNormal             : NORMAL,
	float2             iTexcoord           : TEXCOORD0,
	out float4         oPosition           : POSITION,
	out float4         oTexcoord           : TEXCOORD0)
{

	oPosition  = mul(worldViewProj,iPosition);

	oTexcoord = float4(iTexcoord.x, iTexcoord.y, 0.0, 0.0);	
}
