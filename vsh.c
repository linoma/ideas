uniform int PolyData[20];
varying vec4 VertexColor;
varying vec3 normal;
varying vec4 vertex;

void main(void)
{
   vec4 Ambient;
   vec4 Diffuse;
   vec4 Specular;
   vec4 color;
   int i;
   float nDotVP,nDotHV,pf;

   gl_Position = gl_Vertex;
   vertex = gl_Vertex;
   if(vertex.w != 0.0){
       pf = 1.0 / vertex.w;
       vertex.x = vertex.x * pf;
       vertex.y = vertex.y * pf;
       vertex.z = vertex.z * pf;
   }                                 
   gl_TexCoord[0] = gl_MultiTexCoord0;
   VertexColor = gl_Color;
   if(PolyData[5] != 0)
       gl_FogFragCoord = abs(vertex.z);
   if(PolyData[0] != 0){
       Ambient  = vec4(0.0);
       Diffuse  = vec4(0.0);
       Specular = vec4(0.0);
       normal = gl_Normal;
       normal = normalize(normal);
       for(i=0;i<4;i++){
           if(PolyData[i+1] != 0){
               nDotVP = max(0.0, dot(normal,normalize(vec3(gl_LightSource[i].position))));
               if (nDotVP == 0.0)
               {
                   pf = 0.0;
               }
               else
               {
                   nDotHV = max(0.0,dot(normal, vec3 (gl_LightSource[i].halfVector)));
                   pf = pow(nDotHV, gl_FrontMaterial.shininess);
               }
               Ambient += gl_LightSource[i].ambient;
               Diffuse  += gl_LightSource[i].diffuse * nDotVP;
               Specular += gl_LightSource[i].specular * pf;
           }
       }
       color = gl_FrontLightModelProduct.sceneColor;
       color += Ambient * gl_FrontMaterial.ambient + Diffuse * gl_FrontMaterial.diffuse;
       color += Specular * gl_FrontMaterial.specular;
       color = clamp(color,0.0,1.0);
   }
   else{
       color = VertexColor;
   }
   gl_FrontColor = color;
}
