material 01___Default
{
  technique
  {
    pass
    {
      //polygon_mode wireframe

      ambient .125 .125 .125
      emissive .5 .5 0
      diffuse .5 .5 .5
      specular .3 .3 .3 .3 
      
      texture_unit
      {
        texture white.png
				tex_coord_set 0
				colour_op modulate
      }
    }
  }
}     