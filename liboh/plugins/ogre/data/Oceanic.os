import * from Ocean.material
material Oceanic : Ocean2
{
	technique *
	{
		pass *
		{
			//polygon_mode wireframe
			vertex_program_ref *
			{
				param_named BumpScale float 0.2
				param_named textureScale float2 25 26
				param_named bumpSpeed float2 0.015 0.005
				param_named_auto time time_0_x 100.0
				param_named waveFreq float 0.028
				param_named waveAmp float 0.0 //1.8
			}

			fragment_program_ref *
			{
				param_named deepColor float4 0 0.3 0.5 1.0
				param_named shallowColor float4 0 1 1 1.0
				param_named reflectionColor float4 0.95 1 1 1.0
				param_named reflectionAmount float .5
				param_named reflectionBlur float 0.0
				param_named waterAmount float 0.3
				param_named fresnelPower float 5.0
				param_named fresnelBias float 0.328
				param_named hdrMultiplier float 0.471
			}

			texture_unit
			{
				texture waves2.dds
				tex_coord_set 0
				filtering linear linear linear
			}

		}

	}
}
