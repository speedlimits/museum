// Note, this won't work on ATI which is why it's not used
// the issue is with the array initializers
// no card supports GL_3DL_array_objects but it does work on nvidia, not on ATI
//#extension GL_3DL_array_objects : enable

//-------------------------------
//BlurV_ps20.glsl
// Vertical Gaussian-Blur pass
//-------------------------------

uniform sampler2D Blur0;

//We use the Normal-gauss distribution formula
//f(x) being the formula, we used f(0.5)-f(-0.5); f(1.5)-f(0.5)...

void main()
{
vec2 pos[11];
pos[0]=	vec2(0, -5);
pos[1]=	vec2(0, -4);
pos[2]=	vec2(0, -3);
pos[3]=	vec2(0, -2);
pos[4]=	vec2(0, -1);
pos[5]=	vec2(0, 0);
pos[6]=	vec2(0, 1);
pos[7]=	vec2(0, 2);
pos[8]=	vec2(0, 3);
pos[9]=	vec2(0, 4);
pos[10]=	vec2(0, 5);
float samples[11];
samples[0]=0.01222447;
samples[1]=0.02783468;
samples[2]=0.06559061;
samples[3]=0.12097757;
samples[4]=0.17466632;
samples[5]=0.19741265;
samples[6]=0.17466632;
samples[7]=0.12097757;
samples[8]=0.06559061;
samples[9]=0.02783468;
samples[10]=0.01222447;
    vec4 retVal;
    
    vec4 sum;
    vec2 texcoord = vec2(gl_TexCoord[0]);
    int i = 0;

    sum = vec4( 0 );
    for( ;i < 11; i++ )
    {
        sum += texture2D( Blur0, texcoord + (pos[i] * 0.0100000) ) * samples[i];
    }

    gl_FragColor = sum;
}
