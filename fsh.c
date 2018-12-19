uniform int PolyData[20];
uniform sampler2D tex;
uniform sampler1D ToonColor;
varying vec4 VertexColor;
varying vec3 normal;
varying vec4 vertex;

void main()
{
   vec4 color,texel,col;
   float f;
   bool bDraw;

   color = gl_Color;
   bDraw = true;
   if(PolyData[9] > 1 && PolyData[0] != 0 && PolyData[8] != PolyData[10] && PolyData[11] == 31)
   {
       f = dot(vec3(0.0,0.0,1.0),normal);
       if(abs(f) < 0.2){
           color = texture1D(ToonColor,0.5 + (float(PolyData[10]) / 1024.0));
           color.a = VertexColor.a;
           bDraw = false;
       }
   }
   if(bDraw && PolyData[7] > 2){
       texel = texture2D(tex,gl_TexCoord[0].xy);
       if(PolyData[6] == 1){
           color = texel * color;
       }
       else if(PolyData[6] == 2){
           color = vec4(mix(color.rgb, texel.rgb, texel.a), color.a);
       }
       else if(PolyData[6] == 3){
           col = texture1D(ToonColor,VertexColor.r / 4.1);
           col.a = color.a;
           if(col.r == 0.0 && col.g == 0.0 && col.b == 0.0)
               color = vec4(texel.rgb,color.a);
           else
               color = texel * col;
       }
       else if(PolyData[6] == 4){
           col = texture1D(ToonColor,VertexColor.r / 4.1);
           col.a = color.a;
           if(col.r == 0.0 && col.g == 0.0 && col.b == 0.0)
               col = vec4(texel.rgb,color.a);
           else
               col = texel * col;
           color = clamp(col + vec4(col.rgb,0),0.0,1.0);
       }
   }
   if(PolyData[5] == 1){
       if(gl_FogFragCoord < gl_Fog.start || gl_FogFragCoord > gl_Fog.end)
           f = 1.0;
       else{
           f = (gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale;
           f = clamp(f, 0.0, 1.0);
       }
       color = mix(gl_Fog.color,color, f);
   }
   else if(PolyData[5] == 2){
       if(gl_FogFragCoord < gl_Fog.start)
           f = 1.0;
       else if(gl_FogFragCoord <= gl_Fog.end){
           f = clamp((gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale,0.0,1.0);
       }
       else
           f = 0.0;
//       f = texture1D(ToonColor,0.5 + (f / 2.0)).r;
       f = (gl_Fog.color.a * f) + ((1.0 - f) * color.a);
       color.a = f * 0.5;
   }
   if(PolyData[13] > 0){
       f = float(PolyData[13] - 1) / 31.0;
       if(color.a <= f)
           bDraw = false;
   }
   if(bDraw){
       gl_FragData[0] = color;
       gl_FragData[1] = vec4(float(PolyData[10] + 1) / 64.0,float(PolyData[9]),1.0,1.0);
   }
}
