#include "3d.h"
#include "math3.h"
#include "dsreg.h"
#include "util.h"
#include <float.h>
#include "videoplug.h"
#include "3dguid.h"
#include "lregkey.h"
#include "language.h"
#include "7zcrc.h"
#include "shaders.h"

#ifdef __WIN32__
#pragma link "math3sse.obj"
#endif

extern "C" void vet4_mtx4x4sse(float *,float *);
extern "C" void mtx4x4_multsse(float *,float *);
extern "C" void vet3_mtx4x4sse(float *,float *);
extern "C" void clearsse(void);
extern "C" BOOL check_ndsoverflow(int *,int *,int *);

//0x6e4042b0
#define DSRGB(a) ((a & 0x1F) << 3)|((a & 0x3E0) << 6)|((a & 0x7C00) << 9)
#define EXEC_COMMANDS(a)  \
   if(hThread == NULL)\
       ExecCommands(a);\
   else if(!bExecCommand && com.count()){\
       WaitForSingleObject(hMutex,INFINITE);\
       command |= 2;\
       SetEvent(hEvents[0]);\
       ReleaseMutex(hMutex);\
       WaitForSingleObject(hEvents[3],INFINITE);\
   }
//---------------------------------------------------------------------------
static float tm44[16],tm43[12],ts[3],tt[3],mm33[9],vtx[9],nor[3],st[2],fog[4],nor_e[4];
static float bbox[6],pos_test[3],mtxClip[16];
float mtxTexture[16],mtxProjection[16],mtxView[16],mtxPosition[16];
#ifdef __WIN32__
static HGLRC hRC;
#else
static GLXContext hRC;
static Display *g_pDisplay;
static GdkColormap *colormap;
static GdkDisplay *display;
static GdkScreen *screen;
#endif
static color_M color[9];
static int idxBoxTest,idxLoad4x4,idxLoad4x3,idxMul4x4,idxMul4x3,idxMul3x3,idxTrans,idxScale,idxVtx,idxShin,idxTxt;
static int idxPosTest,PolyData[20];
static u32 toonColor[32],edgeColor[8],dwCores;
static LPTEXTURE tex,crTex;
static LPZ_BUFFER z_buffer;
int mtxMode;
static GLuint texToon,shaderPrg,frameBuffer,texBuffer[2],depthBuffer;
static LPBYTE outBuffer;

static stack_PM sPM;
static stack_MV sMV;
static stack_MV sMP;
static stack_MTX sTX;
static stack_MTX *crS;
static LLight light[4];
static u32 nBeginMode,ulPolyRam,ulVertices,id_internal,*id_buffer,id_poly_internal,*cimg_buffer;
static u8 bToon,bToonChanged,bFog,bAlphaTest,bSwap,bSave=0,bChangeMatrix,bOrtho,poly_alpha,*cimgd_buffer,bControlChanged;
static u8 bFogChanged,bEdgeChanged,bMemoryChanged,bFogShift,fogDensity[32],bUpdateOutBuffer,bExecCommand,bOldSwap;
static u32 texPalette,texIndex,texData,dataPoly,polyData,texpalMem,dataTex,idxList,bCImage,cr_frame,frame_mode=0xFFFFFFFF;
static u64 nOptions,command_value;
static INT_3DFUNC ioFunc[128];
static int ioTable[2048];
static VIDEO3DPARAM v3d;
static LVector<u64> transPoly,com;
static GX_FIFO gx_fifo;
//---------------------------------------------------------------------------
static float mtx4x4I[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
//---------------------------------------------------------------------------
void (*pfn_mtx4x4_mult)(float *,float *);
void (*pfn_vet4_mtx4x4)(float *,float *);
void (*pfn_vet3_mtx4x4)(float *,float *);
//---------------------------------------------------------------------------
#if defined(_DEBPRO)
FILE *LINO,*LINO1;
unsigned long lLino = 0;
LCommandBuffer comBuffer;
u32 id_poly_render_only[2];
#endif

static void ExecCommands(int mode);
static void DrawTranslucentPolygons(BOOL bSort);
static void UpdateOutBuffer();
static void DeleteTranslucentPolygons();
static void CompactTextureBuffer();
static void reset_Indices();
static void clear_buffer();
static BOOL EnableMultiCores(BOOL bEnable);
static void ResetTextureBuffer(BOOL bForce);
//---------------------------------------------------------------------------
static HANDLE hEvents[4],hThread,hMutex;
static DWORD command;
//---------------------------------------------------------------------------
static int flush_render_buffer(u32 nUpdateFrame)
{
   if(!(u8)nUpdateFrame){
       ExecCommands(TRUE);
       if(!(nOptions & NTR_USE_DIRECT_COMMAND))
       	DrawTranslucentPolygons(!((bSwap >> 1) & 1));
   }
   else
       ExecCommands(2);
   DeleteTranslucentPolygons();
   GXSTAT |= (1 << 27);
   ulPolyRam = 0;
   ulVertices = 0;
   reset_Indices();
}
//---------------------------------------------------------------------------
static int render_buffer(u32 nUpdateFrame)
{
   u32 i;

//   glFinish();
   if(!(u8)nUpdateFrame){
       glReadPixels(0,0,256,192,GL_BGRA,GL_UNSIGNED_BYTE,outBuffer);
       UpdateOutBuffer();
   }
   glBindTexture(GL_TEXTURE_2D,0);
   for(i=0;i<idxTxt;i++){
       if(tex[i].index == 0)
           continue;
       if(tex[i].used == 0){
           if(++tex[i].collect_level > 50){
               glDeleteTextures(1,&tex[i].index);
               if(glGetError() == GL_NO_ERROR)
                   ZeroMemory(&tex[i],sizeof(TEXTURE));
               else{
                   tex[i].palData = -1;
                   tex[i].palmem_adr = -1;
                   tex[i].data = -1;
               }
           }
       }
       tex[i].used = 0;
   }
   CompactTextureBuffer();
}
//---------------------------------------------------------------------------
static void ioCUTOFFDEPTH(u32 adr,u32 data,u8)
{
/*   if(polyData & (1<<13)){
       glDisable(GL_CLIP_PLANE0);
       return;
   }
   polyData = (u32)-1;
   double eq[4],len;
   float n[]={0,0,-1,1},l[]={0,0,.9801},pos[3];

   if(bChangeMatrix & 3)
       UpdateClipMatrix();

   vet3_mtx4x4(n,mtxClip);
   eq[0] = n[0] /n[3];
   eq[1] = n[1] /n[3];
   eq[2] = n[2] /n[3];
   pos[0] = 0;//mtxClip[12] / mtxClip[15];
   pos[1] = 0;//mtxClip[13] / mtxClip[15];
   pos[2] = 1; //mtxClip[14] / mtxClip[15];
   eq[3] = .1;
   len = sqrt(eq[0]*eq[0]+eq[1]*eq[1]+eq[2]*eq[2]);
//   len = 1;
   if(len != 0){
       eq[0] /= len;
       eq[1] /= len;
       eq[2] /= len;
//       eq[3] /= len;
   }
   glClipPlane(GL_CLIP_PLANE0,eq);*/
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"CUTOFFDEPTH 0x%08X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void UpdateOutBuffer()
{
   int r,g,b,i1,x,y,id,start;
   u32 color,*p1,*p2,color2,*p3;

   if(!bUpdateOutBuffer)
       goto ex_UpdateOutBuffer;
   if(!(nOptions & NTR_FRAMEBUFFER_OBJ_FLAG) || PolyData[12])
       goto ex_UpdateOutBuffer;
/*   for(y=0,p = z_buffer;y<192;y++){
       for(x=0;x<256;x++,p++){
       }
   }
   for(x=0;x<256;x++){
       p = &z_buffer[x];
       for(y=0;y<192;y++,p += 256){
       }
   }*/
   //
   p2 = id_buffer;
   glReadBuffer(GL_COLOR_ATTACHMENT1_EXT);
   glReadPixels(0,0,256,192,GL_RGBA,GL_UNSIGNED_BYTE,p2);
   glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

   for(y = 191;y >= 0;y--){
       id = 0;
       p3 = &p2[y << 8];
       for(x=0,start = 0;x<256;x++){
           color = *p3++;
           i1 = ((color >> 8) & 0xFF);
           if(i1 < 1 && !start)
               continue;
           i1 = ((color & 0xFF) * 64 / 256);
           if(i1 < 1){
               if(id != 0){
                   color = edgeColor[(id - 1) >> 3];
                   id = 0;
               }
               else
                   continue;
           }
           else if(id == i1)
               continue;
           else{
               id = i1;
               color = edgeColor[(id - 1) >> 3];
           }
           p1 = ((u32 *)&outBuffer[(y << 10) + (x << 2)]);
           if(!start){
               start = 1;
               color2 = *(p1 + 1);
           }
           else if(start){
               start = 0;
               color2 = *(p1 - 1);
           }
           r = (int)(u8)color;
           g = (int)(u8)(color >> 8);
           b = (int)(u8)(color >> 16);

           r *= (int)(u8)color2;
           g *= (int)(u8)(color2 >> 8);
           b *= (int)(u8)(color2 >> 16);

           *p1 = (color2 & 0xFF000000)|RGB(b>>8,g>>8,r>>8);
       }
   }
ex_UpdateOutBuffer:
   bUpdateOutBuffer = 0;
   id_internal = 0;
}
//---------------------------------------------------------------------------
static void UpdateToonFogTable()
{
   LPTEXTURE Tex;
   u32 value[128],val;
   GLuint i;
   BOOL bDelete;

   if((bFogChanged & 8) == 8 || bToonChanged || bEdgeChanged){
       for(i=0;i<32;i++){
           value[i] = 0xFF000000|toonColor[i];
           val = fogDensity[i] * 2 + 1;
           value[32+i] = (val << 24)|(val << 16)|(val << 8)|val;
       }
       for(i=0;i<8;i++)
           value[64 + i] = 0xFF000000|edgeColor[i];
       pfn_glActiveTexture(GL_TEXTURE1);
       glGenTextures(1,&i);
       glBindTexture(GL_TEXTURE_1D,i);
       glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,128,0,GL_RGBA,GL_UNSIGNED_BYTE,value);
       glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
       glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
       if(texToon != 0 && i != texToon){
           bDelete = TRUE;
           for(val=0,Tex = tex;val<idxTxt;val++,Tex++){
               if(Tex->index == texToon){
                   bDelete = FALSE;
                   break;
               }
           }
           if(bDelete)
               glDeleteTextures(1,&texToon);
       }
       texToon = i;
       bFogChanged &= ~8;
       bToonChanged = FALSE;
       bEdgeChanged = FALSE;
       pfn_glActiveTexture(GL_TEXTURE0);
   }
}
//---------------------------------------------------------------------------
static void reset_Indices()
{
	idxPosTest = idxBoxTest = idxShin = idxScale = idxTrans = idxVtx = 0;
   idxMul4x4 = idxLoad4x3 = idxMul4x3 = idxMul3x3 = idxLoad4x4 = 0;
   gx_fifo.Func = NULL;
   gx_fifo.Index = 0;
   gx_fifo.Counter = 0;
   bExecCommand = 0;
}
//---------------------------------------------------------------------------
static int get_pixelFormatMultiSample()
{
   int pixelFormat,valid;
   UINT numFormats;
#ifdef __WIN32__
   PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
   float fAttributes[] = {0,0};
	int iAttributes[] = {WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
		WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,24,WGL_ALPHA_BITS_ARB,8,    
		WGL_DEPTH_BITS_ARB,16,WGL_STENCIL_BITS_ARB,8,
		WGL_DOUBLE_BUFFER_ARB,GL_TRUE,WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
		WGL_SAMPLES_ARB,4,0,0};

   switch(((nOptions & NTR_MULTISAMPLE_MASK) >> NTR_MULTISAMPLE_SHIFT)){
       default:
           iAttributes[19] = 2;
       break;
       case 3:
           iAttributes[19] = 4;
       break;
   }
   wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	if(!wglChoosePixelFormatARB)
       return -1;
	valid = wglChoosePixelFormatARB(v3d.lpWnd->DC(),iAttributes,fAttributes,1,&pixelFormat,&numFormats);
	if(valid && numFormats > 0)
       return pixelFormat;
	return -1;
#else
	XVisualInfo *visualInfo;

	int iAttributes[] = {GLX_RGBA,GLX_DOUBLEBUFFER,GLX_ALPHA_SIZE,8,GLX_DEPTH_SIZE,24,
		GLX_STENCIL_SIZE,8,GLX_SAMPLE_BUFFERS_ARB,GL_TRUE,GLX_SAMPLES_ARB,4,None };

   switch(((nOptions & NTR_MULTISAMPLE_MASK) >> NTR_MULTISAMPLE_SHIFT)){
       default:
           iAttributes[11] = 2;
       break;
       case 3:
           iAttributes[11] = 4;
       break;
   }
	visualInfo = glXChooseVisual(g_pDisplay,gdk_x11_get_default_screen(),iAttributes);
	if(visualInfo != NULL)
		return (int)visualInfo;
	return -1;
#endif
}
//---------------------------------------------------------------------------
static BOOL EnableFrameBufferExtension(BOOL bEnable)
{
   GLenum mrt[] = {GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT1_EXT};
   int i;

   if(depthBuffer != 0){
       pfn_glDeleteRenderbuffersEXT(1,&depthBuffer);
       depthBuffer = 0;
   }
   if(frameBuffer != 0){
       pfn_glDeleteFramebuffersEXT(1,&frameBuffer);
       frameBuffer = 0;
   }
   for(i=0;i<2;i++){
       if(texBuffer[i] != 0){
           glDeleteTextures(1,&texBuffer[i]);
           texBuffer[i] = 0;
       }
   }
   if(!bEnable || !(nOptions & NTR_FRAMEBUFFER_OBJ_OPTION)){
       nOptions &= ~NTR_FRAMEBUFFER_OBJ_OPTION;
       PolyData[12] = 1;
       return TRUE;
   }
   if((nOptions & (NTR_FRAGMENT_SHADER_FLAG|NTR_VERTEX_SHADER_FLAG)) !=
       (NTR_FRAGMENT_SHADER_FLAG|NTR_VERTEX_SHADER_FLAG) ||
       !(nOptions & NTR_FRAMEBUFFER_OBJ_FLAG)){
       nOptions &= ~NTR_FRAMEBUFFER_OBJ_FLAG|NTR_FRAMEBUFFER_OBJ_OPTION;
       return FALSE;
   }
   pfn_glGenFramebuffersEXT(1, &frameBuffer);
   pfn_glGenRenderbuffersEXT(1,&depthBuffer);
   pfn_glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depthBuffer);
   pfn_glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH24_STENCIL8_EXT,256,192);
   pfn_glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
   glGenTextures(2,texBuffer);
   glBindTexture(GL_TEXTURE_2D,texBuffer[0]);
   glTexImage2D(GL_TEXTURE_2D,0,4,256,192,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
   glBindTexture(GL_TEXTURE_2D,texBuffer[1]);
   glTexImage2D(GL_TEXTURE_2D,0,4,256,192,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
   glBindTexture(GL_TEXTURE_2D,0);

   pfn_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,frameBuffer);

   pfn_glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
       GL_TEXTURE_2D,texBuffer[0],0);
   pfn_glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT1_EXT,
       GL_TEXTURE_2D,texBuffer[1],0);
   pfn_glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
       GL_RENDERBUFFER_EXT,depthBuffer);
   pfn_glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,
       GL_RENDERBUFFER_EXT,depthBuffer);
   pfn_glDrawBuffers(2,mrt);
   if(!checkFramebufferStatus()){
       PolyData[12] = 1;
       nOptions &= ~NTR_FRAMEBUFFER_OBJ_FLAG;
       return FALSE;
   }
   PolyData[12] = 0;
   nOptions |= NTR_FRAMEBUFFER_OBJ_OPTION;
   return TRUE;
}
//---------------------------------------------------------------------------
static BOOL EnableShaderExtension(BOOL bEnable)
{
   GLuint vtxShader,fragShader;
   BOOL res;
   char *s;
#ifdef _DEBPRO
   FILE *fp;
#endif
   int i;

   vtxShader = fragShader = 0;
   if(!bEnable || !(nOptions & NTR_SHADERS_OPTION)){
       if(shaderPrg != 0){
           pfn_glUseProgram(NULL);
           pfn_glDeleteProgram(shaderPrg);
           shaderPrg = 0;
       }
       nOptions &= ~(NTR_SHADERS_OPTION|NTR_FRAMEBUFFER_OBJ_OPTION|NTR_EDGE_MARKING_OPTION);
       return TRUE;
   }
   if((nOptions & (NTR_FRAGMENT_SHADER_FLAG|NTR_VERTEX_SHADER_FLAG)) !=
       (NTR_FRAGMENT_SHADER_FLAG|NTR_VERTEX_SHADER_FLAG)){
       nOptions &= ~NTR_SHADERS_OPTION;
       return FALSE;
   }
   res = FALSE;
   if(nOptions & NTR_SHADERS_OPTION){
       if(shaderPrg == 0){
           vtxShader = pfn_glCreateShader(GL_VERTEX_SHADER);
           s = (char *)LocalAlloc(LPTR,4000);
#ifdef _DEBPROa
           fp = fopen("\\ideas\\vsh.c","rb");
           fread(s,4000,1,fp);
           fclose(fp);
#else
			memcpy(s,vertex_shaders,sizeof(vertex_shaders));
           for(i=0;i<sizeof(vertex_shaders);i++)
               s[i] ^= 25;
#endif
           pfn_glShaderSource(vtxShader,1,(const char **)&s,NULL);
           pfn_glCompileShader(vtxShader);
           fragShader = pfn_glCreateShader(GL_FRAGMENT_SHADER);
           ZeroMemory(s,4000);
#ifdef _DEBPROa
           fp = fopen("\\ideas\\fsh.c","rb");
           fread(s,4000,1,fp);
           fclose(fp);
#else
           memcpy(s,fragment_shaders,sizeof(fragment_shaders));
           for(i=0;i<sizeof(fragment_shaders);i++)
               s[i] ^= 25;
#endif
           pfn_glShaderSource(fragShader,1,(const char **)&s,NULL);
           pfn_glCompileShader(fragShader);
           LocalFree(s);

//           printShaderInfoLog(vtxShader);
//           printShaderInfoLog(fragShader);
           shaderPrg = pfn_glCreateProgram();
           pfn_glAttachShader(shaderPrg,vtxShader);
           pfn_glAttachShader(shaderPrg,fragShader);

           pfn_glLinkProgram(shaderPrg);
           if(!printProgramInfoLog(shaderPrg))
               nOptions &= ~(NTR_FRAGMENT_SHADER_FLAG|NTR_VERTEX_SHADER_FLAG|NTR_SHADERS_OPTION|NTR_EDGE_MARKING_OPTION);
           else
               res = TRUE;
       }
       else
           res = TRUE;
   }
   if(fragShader != 0)
       pfn_glDeleteShader(fragShader);
   if(vtxShader != 0)
       pfn_glDeleteShader(vtxShader);
   if(!res){
       pfn_glUseProgram(NULL);
       pfn_glDeleteProgram(shaderPrg);
       shaderPrg = 0;
   }
   else
       pfn_glUseProgram(shaderPrg);
   return res;
}
//---------------------------------------------------------------------------
static void EnableAsyncRead(BOOL bDestroy)
{
   if(bDestroy){
       if(outBuffer != NULL){
           LocalFree(outBuffer);
           outBuffer = NULL;
       }
   }
   else if(outBuffer == NULL)
       outBuffer = (LPBYTE)LocalAlloc(LPTR,256*192*4);
}
//---------------------------------------------------------------------------
static void InitMultiSample()
{
#ifdef __WIN32__
   PROC wglGetExtString;
#else
	__GLXextFuncPtr wglGetExtString;
#endif
   char *supported;
   u64 old;

   old = nOptions;
   nOptions &= ~NTR_MULTISAMPLE_MASK;
   supported = NULL;
#ifdef __WIN32__
   wglGetExtString = wglGetProcAddress("wglGetExtensionsStringARB");
	if(wglGetExtString)
		supported = ((char*(__stdcall*)(HDC))wglGetExtString)(wglGetCurrentDC());
   if(!IsExtensionSupported("WGL_ARB_multisample",supported))
       return;
#else
	supported = (char *)glXQueryExtensionsString(g_pDisplay,gdk_x11_get_default_screen());
   if(!IsExtensionSupported("GLX_ARB_multisample",supported))
       return;
#endif
   if(get_pixelFormatMultiSample() < 1)
       return;
   if((old & NTR_MULTISAMPLE_MASK) == 0)
       old |= NTR_MULTISAMPLE_SUPPORTED;
   nOptions |= (old & NTR_MULTISAMPLE_MASK);
}
//---------------------------------------------------------------------------
static void Create3DDevice(LPVIDEO3DPARAM p1)
{
   LRegKey key;

   nOptions = 0x23FD00;
   memcpy(&v3d,p1,sizeof(v3d));

   if(key.Open("Software\\OpenGL\\Settings")){
       nOptions = key.ReadQword("Settings",0x23FD00);
       key.Close();
   }
   nOptions &= ~(NTR_ASYNC_READ_FLAG|NTR_TEX_COMBINE_FLAG|
               NTR_VERTEX_SHADER_FLAG|NTR_FRAGMENT_SHADER_FLAG|NTR_FRAMEBUFFER_OBJ_FLAG);
#ifndef __WIN32__
    nOptions &= ~NTR_USE_MULTICORES;
#endif
   if(nOptions & NTR_USE_DIRECT_COMMAND)
       nOptions &= ~NTR_USE_MULTICORES;

#ifdef _DEBPRO
   nOptions |= NTR_USE_SOFT_LIGHT;
#endif
//	nOptions &= ~0x400000;
   reset_Indices();
	outBuffer = NULL;
   hRC = NULL;
	texToon = 0;
   tex = NULL;
   idxTxt = 0;
}
//---------------------------------------------------------------------------
static void Destroy3DDevice()
{
	int i;
   LRegKey key;

   if(key.Open("Software\\OpenGL\\Settings")){
       key.WriteQword("Settings",nOptions);
       key.Close();
   }
	if(texToon)
		glDeleteTextures(1,&texToon);
   texToon = 0;
   if(tex != NULL){
	    for(i=0;i<TEXTURE_ALLOCATED;i++){
           if(tex[i].index != 0)
   		    glDeleteTextures(1,&tex[i].index);
       }
   	LocalFree(tex);
   }
   tex = NULL;
   EnableAsyncRead(TRUE);
   if(hRC){
#ifdef __WIN32__
       wglMakeCurrent(NULL,NULL);
       wglDeleteContext(hRC);
#else
	    glXMakeCurrent(g_pDisplay,NULL,NULL);
	    glXDestroyContext(g_pDisplay,hRC);
#endif
   }
   if(shaderPrg != 0){
       pfn_glUseProgram(NULL);
       pfn_glDeleteProgram(shaderPrg);
       shaderPrg = 0;
   }
   if(depthBuffer != 0){
       pfn_glDeleteRenderbuffersEXT(1,&depthBuffer);
       depthBuffer = 0;
   }
   if(frameBuffer != 0){
       pfn_glDeleteFramebuffersEXT(1,&frameBuffer);
       frameBuffer = 0;
   }
   for(i=0;i<2;i++){
       if(texBuffer[i] != 0){
           glDeleteTextures(1,&texBuffer[i]);
           texBuffer[i] = 0;
       }
   }
}
//---------------------------------------------------------------------------
static BOOL Init3DDevice(BOOL bInit)
{
#ifdef __WIN32__
   PIXELFORMATDESCRIPTOR pfd={0};
   int pixelformat;

   if(v3d.lpWnd->DC() == NULL)
        return FALSE;
#else
   int doubleBufferVisual[] = {GLX_RGBA,GLX_DOUBLEBUFFER,GLX_ALPHA_SIZE,8,GLX_DEPTH_SIZE,24,GLX_STENCIL_SIZE,8,None };
   XVisualInfo *visualInfo;
   GdkVisual *visual;
#endif
   if(tex == NULL){
       tex = (LPTEXTURE)LocalAlloc(LPTR,TEXTURE_ALLOCATED * sizeof(TEXTURE) +
           256*192*sizeof(Z_BUFFER) + 256*192*sizeof(u32) + 256*256*sizeof(32) + 256 * 256 * sizeof(long)
           );
       if(tex == NULL)
           return FALSE;
       z_buffer = (LPZ_BUFFER)(tex + TEXTURE_ALLOCATED);
       id_buffer = (u32 *)(z_buffer + 256*192);
       cimg_buffer = (u32 *)(id_buffer + 256*192);
       cimgd_buffer = (u8 *)(cimg_buffer + 256 * 256);
   }
Init3DDevice_0:
#ifdef __WIN32__
   pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
   pfd.nVersion = 1;
   pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
   pfd.dwLayerMask = PFD_MAIN_PLANE;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.cColorBits = 24;
   pfd.cAlphaBits = 8;
   pfd.cDepthBits = 24;
   pfd.cStencilBits = 8;
#endif
   if(bInit || ((nOptions & NTR_MULTISAMPLE_MASK) >> NTR_MULTISAMPLE_SHIFT) < 2){
        if(!bInit){
            v3d.lpWnd->Release();
            v3d.lpWnd->Create();
        }
#ifdef __WIN32__
        if((pixelformat = ChoosePixelFormat(v3d.lpWnd->DC(),&pfd)) == 0){
            MessageBox(NULL,"Your video card don't support this pixelformat : \r\nbpp: 24 alpha:8 z:16 stencil:8\r\nCheck your video card settings or driver.","iDeaS Emulator",MB_OK|MB_ICONERROR);
            return FALSE;
        }
#else
        if(g_pDisplay == 0){
            display = gdk_drawable_get_display(v3d.lpWnd->Handle()->window);
            screen = gdk_drawable_get_screen(v3d.lpWnd->Handle()->window);
            g_pDisplay = gdk_x11_display_get_xdisplay(display);
        }
        visualInfo = glXChooseVisual(g_pDisplay,gdk_x11_get_default_screen(),doubleBufferVisual);
        if(visualInfo == NULL){
            MessageBox(NULL,"Your video card don't support this pixelformat : \r\nSetPixelFormat: bpp: 24 alpha:8 z:16 stencil:8\r\nCheck your video card settings or driver.","iDeaS Emulator",MB_OK|MB_ICONERROR);
            return FALSE;
        }
        if(v3d.lpWnd->Handle()->window != 0)
            visual = gdk_drawable_get_visual(v3d.lpWnd->Handle()->window);
        else
            visual = NULL;
        if(colormap != NULL)
            DeleteObject(colormap);
        colormap = NULL;
        if (visual == NULL || GDK_VISUAL_XVISUAL(visual)->visualid != visualInfo->visualid){
            visual = gdk_x11_screen_lookup_visual(screen, visualInfo->visualid);
            colormap = gdk_colormap_new (visual, FALSE);
            v3d.lpWnd->Release();
            v3d.lpWnd->Create();
            gtk_widget_set_colormap(v3d.lpWnd->Handle(),colormap);
            gtk_widget_realize(v3d.lpWnd->Handle());
        }
#endif
        if(hRC){
#ifdef __WIN32__
            wglMakeCurrent(NULL,NULL);
            wglDeleteContext(hRC);
#else
            glXMakeCurrent(g_pDisplay,NULL,NULL);
            glXDestroyContext(g_pDisplay,hRC);
#endif
            hRC = NULL;                               
        }
#ifdef __WIN32__
        if(!::SetPixelFormat(v3d.lpWnd->DC(),pixelformat,&pfd)){
            MessageBox(NULL,"Your video card don't support this pixelformat : \r\nSetPixelFormat: bpp: 24 alpha:8 z:16 stencil:8\r\nCheck your video card settings or driver.","iDeaS Emulator",MB_OK|MB_ICONERROR);
            return FALSE;
        }
        hRC = wglCreateContext(v3d.lpWnd->DC());
        if(hRC == NULL)
            return FALSE;
        wglMakeCurrent(v3d.lpWnd->DC(),hRC);
#else
        hRC = glXCreateContext(g_pDisplay,visualInfo,NULL,GL_TRUE);
        if(hRC == NULL)
            return FALSE;
        glXMakeCurrent(g_pDisplay,GDK_DRAWABLE_XID(v3d.lpWnd->Handle()->window),hRC);
#endif
        if(bInit)
            InitMultiSample();
	}
   if(((nOptions & NTR_MULTISAMPLE_MASK) >> NTR_MULTISAMPLE_SHIFT) > 1){
#ifdef __WIN32__
       if((pixelformat = get_pixelFormatMultiSample()) < 1){
           MessageBox(NULL,"wglGetPixelFormat\r\nYour video card don't support this pixelformat : \r\nbpp: 24 alpha:8 z:16 stencil:8\r\nmultisample 2x\r\nCheck your video card settings or driver.","iDeaS Emulator",MB_OK|MB_ICONERROR);
           nOptions &= ~NTR_MULTISAMPLE_MASK;
           goto Init3DDevice_0;
       }
#else
       if((int)(visualInfo = (XVisualInfo *)get_pixelFormatMultiSample()) < 1){
           MessageBox(NULL,"wglGetPixelFormat\r\nYour video card don't support this pixelformat : \r\nbpp: 24 alpha:8 z:16 stencil:8\r\nmultisample 2x\r\nCheck your video card settings or driver.","iDeaS Emulator",MB_OK|MB_ICONERROR);
           nOptions &= ~NTR_MULTISAMPLE_MASK;
           goto Init3DDevice_0;
       }
#endif
       if(hRC){
#ifdef __WIN32__
           v3d.lpWnd->Release();
           v3d.lpWnd->Create();
           wglMakeCurrent(NULL,NULL);
           wglDeleteContext(hRC);
#else
           glXMakeCurrent(g_pDisplay,NULL,NULL);
           glXDestroyContext(g_pDisplay,hRC);
           if(colormap != NULL)
               g_object_ref(colormap);
           colormap = NULL;
#endif
           hRC = NULL;
       }
#ifdef __WIN32__
       if(!::SetPixelFormat(v3d.lpWnd->DC(),pixelformat,&pfd)){
           nOptions &= ~NTR_MULTISAMPLE_MASK;
           MessageBox(NULL,"SetPixelFormat\r\nYour video card don't support this pixelformat : \r\nbpp: 24 alpha:8 z:16 stencil:8\r\nmultisample 2x\r\nCheck your video card settings or driver.","iDeaS Emulator",MB_OK|MB_ICONERROR);
           goto Init3DDevice_0;
       }
       hRC = wglCreateContext(v3d.lpWnd->DC());
       if(hRC == NULL)
           return FALSE;
       wglMakeCurrent(v3d.lpWnd->DC(),hRC);
#else
       if(v3d.lpWnd->Handle()->window != 0)
           visual = gdk_drawable_get_visual(v3d.lpWnd->Handle()->window);
       else
           visual = NULL;
       if(colormap != NULL)
           DeleteObject(colormap);
       colormap = NULL;
       if (visual == NULL || GDK_VISUAL_XVISUAL(visual)->visualid != visualInfo->visualid){
           visual = gdk_x11_screen_lookup_visual(screen, visualInfo->visualid);
           colormap = gdk_colormap_new (visual, FALSE);
           v3d.lpWnd->Release();
           v3d.lpWnd->Create();
           gtk_widget_set_colormap(v3d.lpWnd->Handle(),colormap);
           gtk_widget_realize(v3d.lpWnd->Handle());
       }
       hRC = glXCreateContext(g_pDisplay,visualInfo,NULL,GL_TRUE);
       if(hRC == NULL)
           return FALSE;
       glXMakeCurrent(g_pDisplay, GDK_DRAWABLE_XID(v3d.lpWnd->Handle()->window), hRC);
#endif
   }
#ifndef __WIN32__
   if(colormap != NULL)
       v3d.lpWnd->Map();
#endif
   nOptions |= init_gl_extension();
   EnableShaderExtension(TRUE);
   EnableFrameBufferExtension(TRUE);
   EnableAsyncRead(FALSE);
	if(nOptions & NTR_FOGCOORD_FLAG)
   	glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
   sPM.init(0);
   sMV.init(31);
   sMP.init(31);
   sTX.init(0);
   return (BOOL)TRUE;
}
//---------------------------------------------------------------------------
static void CompactTextureBuffer()
{
   u32 i,i1;

   for(i=0,i1=0xFFFFFFFF;i<idxTxt;i++){
       if(tex[i].index != 0){
           i1++;
           if(i != i1)
               CopyMemory(&tex[i1],&tex[i],sizeof(TEXTURE));
       }
   }
   if(++i1 != idxTxt){
       idxTxt = i1;
       ZeroMemory(&tex[idxTxt],sizeof(TEXTURE));
   }
}
//---------------------------------------------------------------------------
static void ResetTextureBuffer(BOOL bForce)
{
	u32 i,prev_adr,status,prev_status;
   BOOL bRemove;
   LPTEXTURE Tex;

   glBindTexture(GL_TEXTURE_2D,0);
   if(tex == NULL)
   	return;
   prev_status = prev_adr = 0;
	for(i=0,Tex = tex;i<idxTxt;i++,Tex++){
       if(Tex->index != 0){
           bRemove = TRUE;
           if(!bForce){
               if(Tex->format != 5){
                   if(prev_adr == Tex->tex_adr && (prev_status & 2) && (prev_status & 1) == 0)
                       status = 0;
                   else{
                       status = 1;
                       prev_status = 0;
                       if(ds.get_VideoMemoryStatus(Tex->tex_adr,Tex->num_pixels,&status) == S_OK)
                           prev_status = 2 | (status & 1);
                   }
                   if(!status){
                       if(Tex->pal_adr != 0){
                           if(ds.get_VideoMemoryStatus(Tex->pal_adr,Tex->num_cols*2,&status) == S_OK && !status)
                               bRemove = FALSE;
                       }
                       else
                           bRemove = FALSE;
                   }
                   prev_adr = Tex->tex_adr;
               }
           }
           if(bRemove){
               glDeleteTextures(1,&Tex->index);
               if(glGetError() == GL_NO_ERROR)
                   ZeroMemory(Tex,sizeof(TEXTURE));
               else{
                   Tex->data = -1;
                   Tex->palData = -1;
                   Tex->palmem_adr = -1;
               }
           }
       }
   }
   if(bForce)
       idxTxt = 0;
   else
       CompactTextureBuffer();
}
//---------------------------------------------------------------------------
static void set_Options(u64 value)
{
   u64 old;
   u32 i;
   BOOL bUpdate;

   old = nOptions;
   nOptions = value;
   bUpdate = FALSE;
   if((value & NTR_MULTISAMPLE_MASK) != (old & NTR_MULTISAMPLE_MASK)){
       glEnd();
       glEndList();
       for(i=1;i<=transPoly.count();i++)
          	glDeleteLists((GLuint)(transPoly[i] & 0xFFFFF),20);
	    transPoly.clear();
       ResetTextureBuffer(TRUE);
       EnableShaderExtension(FALSE);
       EnableFrameBufferExtension(FALSE);
       nOptions |= value & (NTR_FRAMEBUFFER_OBJ_OPTION|NTR_SHADERS_OPTION);
       if(!Init3DDevice(FALSE)){
           nOptions &= ~NTR_MULTISAMPLE_MASK;
           Init3DDevice(FALSE);
       }
       bUpdate = TRUE;
   }
   if(bUpdate)
       return;
   EnableAsyncRead(FALSE);
   if((value & NTR_SHADERS_OPTION) != (old & NTR_SHADERS_OPTION))
       EnableShaderExtension(((value & NTR_SHADERS_OPTION) ? TRUE : FALSE));
   if((value & NTR_FRAMEBUFFER_OBJ_OPTION) != (old & NTR_FRAMEBUFFER_OBJ_OPTION))
       EnableFrameBufferExtension(((value & NTR_FRAMEBUFFER_OBJ_OPTION) ? TRUE : FALSE));
}
//---------------------------------------------------------------------------
static void Reset3DDevice()
{
	int i;

#ifdef _DEBPRO
   if(LINO)
       fclose(LINO);
   LINO = NULL;
   if(LINO1)
       fclose(LINO1);
   LINO1 = NULL;
   lLino = 0;
   comBuffer.reset();
#endif
   glEnd();
   glEndList();
   for(i=1;i<=transPoly.count();i++)
 	    glDeleteLists((GLuint)(transPoly[i] & 0xFFFFF),20);
	transPoly.clear();
   glFlush();
   glFinish();
	ResetTextureBuffer(TRUE);
	if(outBuffer != NULL)
   	ZeroMemory(outBuffer,256*192*4);
	if(texToon)
		glDeleteTextures(1,&texToon);
   nBeginMode = 0;
   texToon = 0;
   bToon = 0;
   bToonChanged = 0;
   bFogShift = 0;
   bUpdateOutBuffer = 0;
   bControlChanged = 0;
   id_internal = 0;
   id_poly_internal = 0;
   cr_frame = 0;
   reset_Indices();
   memset(z_buffer,0,sizeof(Z_BUFFER)*256*192);

	gx_fifo.Func = NULL;
	gx_fifo.Reg = 0;
	gx_fifo.Counter = 0;
	gx_fifo.CurrentReg = 0;
   gx_fifo.Cycles = 0;
	gx_fifo.Index = 0;

   crTex = NULL;
	ZeroMemory(tm44,sizeof(tm44));
	ZeroMemory(mm33,sizeof(mm33));
   ZeroMemory(tm43,sizeof(tm43));
 	glEnable(GL_DITHER);
//   glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
   glFogi(GL_FOG_MODE,GL_LINEAR);
  	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_TEXTURE_1D);
	glDisable(GL_FOG);
   glDisable(GL_POLYGON_SMOOTH);
   glDisable(GL_ALPHA_TEST);
   glDisable(GL_BLEND);
	glClearColor(0,0,0,0);
  	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
   for(i=0;i<4;i++){
		light[i].set_Index(i);
       light[i].reset();
   }
	glMatrixMode(GL_TEXTURE);
  	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
  	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
  	glLoadIdentity();
   crS = &sMV;
   mtxMode = NTR_MODELVIEW_MATRIX;
	memcpy(mtxView,mtx4x4I,sizeof(mtx4x4I));
  	memcpy(mtxProjection,mtx4x4I,sizeof(mtx4x4I));
 	memcpy(mtxTexture,mtx4x4I,sizeof(mtx4x4I));
 	memcpy(mtxPosition,mtx4x4I,sizeof(mtx4x4I));
   sPM.reset();
   sPM.set_type(NTR_PROJECTION_MATRIX);
   sMV.reset();
   sMV.set_type(NTR_MODELVIEW_MATRIX);
   sMP.reset();
   sMP.set_type(NTR_POSITION_MATRIX);
   color[0].set_type(0);
  	color[1].set_type(GL_DIFFUSE);
   color[2].set_type(GL_AMBIENT);
   color[3].set_type(GL_SPECULAR);
   color[4].set_type(GL_EMISSION);
   color[5].set_type(GL_SHININESS);
	for(i=0;i<7;i++)
   	color[i].reset();
   color[0].set(31,31,31);
   color[6] = color[0];
   color[1].set(24,24,24);
   color[2].set(6,6,6);
   color[3].set(0,0,0);
   color[4].set(31,31,31);
   color[5].set(31,31,31);
   texIndex = texData = polyData = (u32)-1;
   bChangeMatrix = 0;
   ulPolyRam = 0;
   ulVertices = 0;
   bOrtho = FALSE;
   texpalMem = 0;
   bFogChanged = bEdgeChanged = 0;
   bCImage = 0;
   bOldSwap = bSwap = 0;
}
//---------------------------------------------------------------------------
static void BindTexture(LPTEXTURE p)
{
   crTex = p;
   if(crTex == NULL){
       PolyData[7] &= ~2;
       return;
   }
   PolyData[7] |= 2;
   crTex->used = 1;
   crTex->collect_level = 0;
   glBindTexture(GL_TEXTURE_2D,crTex->index);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,crTex->fs ? GL_MIRRORED_REPEAT : crTex->ws ? GL_REPEAT : GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,crTex->ft ? GL_MIRRORED_REPEAT : crTex->wt ? GL_REPEAT : GL_CLAMP);
}
//---------------------------------------------------------------------------
static void BindTexture(int index)
{
   BindTexture(&tex[index]);
}
//---------------------------------------------------------------------------
static BOOL CreateTexture(void *buffer,u32 index)
{
	u8 px,*vram,px1;
   u32 *pTexture;
	u32 p,num_pixels,p1,a,p2,p3,col32;
	u16 col,*pal,col1,*pal1;
   int i1,i,i2,i3,r,g,b,r1,g1,b1;
	LPTEXTURE Tex;

  	Tex = &tex[index];
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"CREATETEXTURE %u\n",index);
#endif
   if(index >= TEXTURE_ALLOCATED || Tex->data == (u32)-1 || (index >= idxTxt && index != TEXTURE_ALLOCATED-1))
       return FALSE;
   p = ((buffer == NULL) ? texPalette : Tex->palData);
	pal = (u16 *)(v3d.video_mem + (p1 = (p << 4) + texpalMem) + 524288);
   vram = v3d.video_mem + (a = Tex->adr);
   px = (u8)((0x83|((i = (a >> 17)) << 3)));
   if((v3d.io_mem[0x240 + (a >> 17)] & 0x9F) != px){
		for(i=0;i<4;i++){
			if((v3d.io_mem[0x240+i] & 0x9F) == px){
				a = (a & 0x1FFFF) + (i << 17);
               vram = v3d.video_mem + a;
           	break;
           }
       }
   }
   num_pixels = Tex->h_size * Tex->v_size;
   if(buffer == NULL){
       if((pTexture = (u32 *)LocalAlloc(LPTR,num_pixels*sizeof(u32))) == NULL)
  	        return FALSE;
       Tex->tex_adr = 0x06800000|a;
       Tex->slot = (u8)i;
   }
   else
       pTexture = (u32 *)buffer;
	switch(Tex->format){
   	case 1:           //1 2 3
           Tex->num_cols = 256;
           Tex->num_pixels = num_pixels;
           Tex->palData = (u16)texPalette;
           Tex->pal_adr = 0x06800000 + (texPalette << 4) + texpalMem + 524288;
           Tex->palmem_adr = texpalMem;
			for(p = 0; p < num_pixels;p++){
           	a = vram[p];
           	col = pal[(a&0x1F)];
               a = (a >> 5);
               a = (a << 5) + (a<<2);
				pTexture[p] = (a<<24)|DSRGB(col);
	        }
       break;
   	case 2:
       	pal = (u16 *)(v3d.video_mem + (a = ((p << 3) + texpalMem)) + 524288);
			num_pixels >>= 2;
           Tex->num_cols = 4;
           Tex->num_pixels = num_pixels;
           Tex->pal_adr = 0x06800000 + (texPalette << 3) + texpalMem + 524288;
           Tex->palData = (u16)texPalette;
           Tex->palmem_adr = texpalMem;
			for(p = p1 = 0; p < num_pixels;p++){
           	px = vram[p];
				for(i1=0;i1<4;i1++){
               	col = pal[px1 = (u8)(px & 0x3)];
                   a = (px1 == 0 && Tex->tr) ? 0 : 0xFF000000;
					pTexture[p1++] = a |DSRGB(col);
                   px >>= 2;
               }
	        }
       break;
       case 3:
           num_pixels >>= 1;
           Tex->num_cols = 16;
           Tex->num_pixels = num_pixels;
           Tex->palData = (u16)texPalette;
           Tex->pal_adr = 0x06800000 + (texPalette << 4) + texpalMem + 524288;
           Tex->palmem_adr = texpalMem;
			for(p = p1 = 0; p < num_pixels;p++){
           	px = vram[p];
              	col = pal[(px1 = (u8)(px & 0xF))];
             	a = (px1 == 0 && Tex->tr) ? 0 : 0xFF000000;
				pTexture[p1++] = a|DSRGB(col);
              	col = pal[(px1 = (u8)(px >> 4))];
             	a = (px1 == 0 && Tex->tr) ? 0 : 0xFF000000;
				pTexture[p1++] = a|DSRGB(col);
	        }
       break;
       case 4:
           Tex->num_cols = 256;
           Tex->num_pixels = num_pixels;
           Tex->palData = (u16)texPalette;
           Tex->pal_adr = 0x06800000 + (texPalette << 4) + texpalMem + 524288;
           Tex->palmem_adr = texpalMem;
			for(p = 0; p < num_pixels;p++){
           	col = pal[px1 = vram[p]];
               a = (px1 == 0 && Tex->tr) ? 0 : 0xFF000000;
				pTexture[p] = a|DSRGB(col);
	        }
       break;
       case 5:
           Tex->palData = (u16)texPalette;
           Tex->pal_adr = 0x06800000 + (texPalette << 4) + texpalMem + 524288;
           Tex->palmem_adr = texpalMem;
           Tex->num_cols = 32768;
           for(i=0;i<4;i++){
               if((v3d.io_mem[0x240+i] & 0x98) == 0x88)
                   break;
           }
           p = (i << 17);
           if(i < (a >> 17))
           	p += 0x10000;
           p += ((a & 0x1FFFF) >> 1);
           p2 = p3 = 0;
       	for(i=0;i < Tex->v_size;i += 4){
               for(i1=0;i1 < Tex->h_size;i1 += 4){
                   p1 = (u32)(((u32 *)vram)[p3]);
                   a = (u32)((u16 *)(v3d.video_mem+p))[p3++];
                   pal1 = &pal[(a & 0x3FFF) << 1];
                   for(i2=0;i2 < 4;i2++){
                       for(i3=0;i3 < 4;i3++){
                           col = (u16)(p1 & 3);
                           switch(a >> 14){
                               case 0:
                                   switch(col){
                                       case 3:
                                           col32 = (u32)0;
                                       break;
                                       default:
                                           col32 = (u32)(0xFF000000|DSRGB(pal1[col]));
                                       break;
                                   }
                               break;
                               case 1:
                                   switch(col){
                                       case 0:
                                       case 1:
                                           col32 = (u32)(0xFF000000|DSRGB(pal1[col]));
                                       break;
                                       case 3:
                                           col32 = (u32)0;
                                       break;
                                       case 2:
                                           col = pal1[0];
                                           col1 = pal1[1];
                                           r = (col >> 10) & 0x1F;
                                           g = (col >> 5) & 0x1F;
                                           b = (col & 0x1F);
                                           r1 = (col1 >> 10) & 0x1F;
                                           g1 = (col1 >> 5) & 0x1F;
                                           b1 = (col1 & 0x1F);
                                           r = (r + r1);
                                           b = (b + b1);
                                           g = (g + g1);
                                           col32 = (u32)(0xFF000000|RGB(b<<2,g<<2,r<<2));
                                       break;
                                   }
                               break;
                               case 2:
                                   col32 = (u32)(0xFF000000|DSRGB(pal1[col]));
                               break;
                               case 3:
                                   switch(col){
                                       case 0:
                                       case 1:
                                           col32 = (u32)(0xFF000000|DSRGB(pal1[col]));
                                       break;
                                       case 2:
                                           col = pal1[0];
                                           col1 = pal1[1];
                                           r = (col >> 10) & 0x1F;
                                           g = (col >> 5) & 0x1F;
                                           b = (col & 0x1F);
                                           r1 = (col1 >> 10) & 0x1F;
                                           g1 = (col1 >> 5) & 0x1F;
                                           b1 = (col1 & 0x1F);
                                           if((r = ((r*5) + (r1*3))) > 255)
                                               r = 255;
                                           if((b = ((b*5) + (b1*3))) > 255)
                                               b = 255;
                                           if((g = ((g*5) + (g1*3))) > 255)
                                               g = 255;
                                           col32 = (u32)(0xFF000000|RGB(b,g,r));
                                       break;
                                       case 3:
                                           col = pal1[0];
                                           col1 = pal1[1];
                                           r = (col >> 10) & 0x1F;
                                           g = (col >> 5) & 0x1F;
                                           b = (col & 0x1F);
                                           r1 = (col1 >> 10) & 0x1F;
                                           g1 = (col1 >> 5) & 0x1F;
                                           b1 = (col1 & 0x1F);
                                           if((r = ((r*3) + (r1*5))) > 255)
                                               r = 255;
                                           if((b = ((b*3) + (b1*5))) > 255)
                                               b = 255;
                                           if((g = ((g*3) + (g1*5))) > 255)
                                               g = 255;
                                           col32 = (u32)(0xFF000000|RGB(b,g,r));
                                       break;
                                   }
                               break;
                           }
                           pTexture[(i+i2)*Tex->h_size+i3+i1] = col32;
                           p1 >>= 2;
                       }
                   }
               }
           }
       break;
		case 6:
           Tex->num_cols = 256;
           Tex->num_pixels = num_pixels;
           Tex->palData = (u16)texPalette;
           Tex->pal_adr = 0x06800000 + (texPalette << 4) + texpalMem + 524288;
           Tex->palmem_adr = texpalMem;
			for(p = 0; p < num_pixels;p++){
           	a = vram[p];
               col = pal[a & 7];
               a >>= 3;
				pTexture[p] = (a<<27)|DSRGB(col);
	        }
       break;
   	case 7:
           if(num_pixels*2 > 524287)
               num_pixels = 524288/2;
           Tex->num_cols = 32768;
           Tex->num_pixels = num_pixels*2;
   		for(p = 0; p < num_pixels;p++){
   			col = (u16)(((u16 *)vram)[p]);
				pTexture[p] = ((col & 0x8000) ? 0xFF000000 : 0) |DSRGB(col);
   		}
       break;
   }
   if(buffer != NULL)
       return TRUE;
   glGenTextures(1,&Tex->index);
   glBindTexture(GL_TEXTURE_2D,Tex->index);
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,Tex->h_size,Tex->v_size,0,GL_RGBA,GL_UNSIGNED_BYTE,(GLvoid *)pTexture);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   ds.set_VideoMemoryStatus(Tex->pal_adr,Tex->num_cols*2,0);
   ds.set_VideoMemoryStatus(Tex->tex_adr,Tex->num_pixels,0);
   Tex->idxList = idxList;
   BindTexture(index);
#ifdef _DEBPRO
   if(bSave)
       SaveTextureAsTGA((u8 *)pTexture,(u16)Tex->h_size,(u16)Tex->v_size,Tex->data);
#endif
   LocalFree(pTexture);
   for(i=0;i<=idxTxt;i++){
       for(i1=i+1;i1<=idxTxt;i1++){
           if(tex[i].index == tex[i1].index){
               if(tex[i].data == 0xFFFFFFFF || tex[i1].data == 0xFFFFFFFF)
                   ZeroMemory(&tex[i],sizeof(TEXTURE));
           }
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
static void UpdatePolyRamCount()
{
   int i;

   i = 0;
   ulVertices++;
   switch((u8)nBeginMode){
       case GL_TRIANGLES:
           if(ulVertices > 2){
               ulVertices = 0;
               i = 1;
           }
       break;
       case GL_TRIANGLE_STRIP:
           if(ulVertices > 2){
               i = 1;
               ulVertices -= 2;
           }
       break;
       case GL_QUADS:
           if(ulVertices > 3){
               ulVertices = 0;
               i = 1;
           }
       break;
       case GL_QUAD_STRIP:
           if(ulVertices > 3){
               ulVertices -= 3;
               i = 1;
           }
       break;
   }
   if(ulPolyRam <= 2048){
       ulPolyRam += i;
       (LISTRAM_COUNT) = (u16)ulPolyRam;
   }
   else
       *((u16 *)&v3d.io_mem[0x60]) |= 0x2000;
}
//---------------------------------------------------------------------------
static void UpdateClipMatrix()
{
   float f;
   int i;

   memcpy(mtxClip,mtxProjection,sizeof(mtxClip));
   if(bChangeMatrix & 1){
       bOrtho = FALSE;
       if(bOldSwap & 4){
           f = mtxProjection[11] + mtxProjection[15];
           if(f == 1 && mtxProjection[15])
               bOrtho = TRUE;
       }
   }
  	pfn_mtx4x4_mult(mtxClip,mtxView);
   for(i=0;i<16;i++)
       ((u32 *)&v3d.io_mem[0x640])[i] = (u32)((mtxClip[i] * 4096.0f));
/*   mtxClip[2] *= .99999f;
   mtxClip[6] *= .99999f;
   mtxClip[10] *= .99999f;
   mtxClip[14] *= .99999f;*/
   bChangeMatrix = 0;
}
//---------------------------------------------------------------------------
static void put_TexCoord()
{
   float fv[2];

   if(crTex != NULL && crTex->coordMode == 1){
       fv[0] = st[0] * mtxTexture[0] + st[1] * mtxTexture[4] + 0.0625f * mtxTexture[8] + 0.0625f * mtxTexture[12];
       fv[1] = st[0] * mtxTexture[1] + st[1] * mtxTexture[5] + 0.0625f * mtxTexture[9] + 0.0625f * mtxTexture[13];
   }
   else{
       fv[0] = st[0];
       fv[1] = st[1];
   }
   if(crTex != NULL){
       if(crTex->h_size)
           fv[0] /= crTex->h_size;
       if(crTex->v_size)
           fv[1] /= crTex->v_size;
   }
   glTexCoord2fv(fv);
#ifdef _DEBPRO
   comBuffer.add_Command(2,fv[0] * 16384.0f,0);
   comBuffer.add_Command(2,fv[1] * 16384.0f,0);
   if(LINO)
  		fprintf(LINO,"VTX_ST %f,%f\n",fv[0],fv[1]);
#endif
}
//---------------------------------------------------------------------------
static void put_Normal()
{
	float v[4],dot,diffuseLevel,shininessLevel;
   int i,i1;

   nor_e[0] = nor[0];
   nor_e[1] = nor[1];
   nor_e[2] = nor[2];
   nor_e[3] = 1;
   pfn_vet3_mtx4x4(nor_e,mtxPosition);
#ifdef _DEBPRO
	comBuffer.add_Command(1,nor_e[0] * 16384.0f,0);
   comBuffer.add_Command(1,nor_e[1] * 16384.0f,0);
   comBuffer.add_Command(1,nor_e[2] * 16384.0f,0);
#endif
	if((nOptions & NTR_DONT_USE_LIGHT) == 0){
       if((nOptions & NTR_USE_SOFT_LIGHT)){
           switch(dataPoly & 0x30){
               case 0x20:
                   i1 = color[0].get_color()[0] * 31.0f;
                   v[0] = (toonColor[i1] & 0xFF) / 255.0f;
                   v[1] = ((toonColor[i1] >> 8) & 0xFF) / 255.0f;
                   v[2] = ((toonColor[i1] >> 16) & 0xFF) / 255.0f;
               break;
               default:
                   v[0] = color[4].get_color()[0];
                   v[1] = color[4].get_color()[1];
                   v[2] = color[4].get_color()[2];
               break;
           }
           for(i=0;i<4;i++){
               if((dataPoly & (1 << i)) == 0 || (nOptions & (1 << (i + 10))) == 0)
                   continue;
               dot = -(-light[i].get_pos()[0] * nor_e[0] +
                   -light[i].get_pos()[1] * nor_e[1] +
                   -light[i].get_pos()[2] * nor_e[2]);
               if(dot < 0)
                   diffuseLevel = 0;
               else
                   diffuseLevel = dot;
               dot = (light[i].get_halfpos()[0] * nor_e[0] +
                   light[i].get_halfpos()[1] * nor_e[1] +
                   light[i].get_halfpos()[2] * nor_e[2]);
               if(dot < 0)
                   shininessLevel = 0;
               else
                   shininessLevel = dot * dot;
               if(*((u32 *)&v3d.io_mem[0x4C4]) & 0x8000)
                   shininessLevel = v3d.io_mem[0x4d0 + (int)(shininessLevel * 128)] / 128.0f;
               for(i1=0;i1<3;i1++){
                   v[i1] += color[3].get_color()[i1] * light[i].get_color()[i1] * shininessLevel
                       + color[1].get_color()[i1] * light[i].get_color()[i1] * diffuseLevel
                       + color[2].get_color()[i1] * light[i].get_color()[i1];
               }
           }
           for(i=0;i<3;i++){
               if(v[i] > 1.0f)
                   v[i] = 1.0f;
           }
           color[8].set(v[0]*31.0f,v[1]*31.0f,v[2]*31.0f,poly_alpha);
       }
		else
       	color[8] = color[0];
   }                                                                   
	nBeginMode |= 0x40000000;
	glNormal3fv(nor_e);
   if(crTex != NULL && crTex->coordMode == 2){
       v[0] = ((nor[0] * mtxTexture[0] + nor[1] * mtxTexture[4] + nor[2] * mtxTexture[8]) * 0.0625f) + st[0];
       v[1] = ((nor[0] * mtxTexture[1] + nor[1] * mtxTexture[5] + nor[2] * mtxTexture[9]) * 0.0625f) + st[1];
       if(crTex->h_size)
           v[0] /= crTex->h_size;
       if(crTex->v_size)
           v[1] /= crTex->v_size;
#ifdef _DEBPRO
		comBuffer.add_Command(2,v[0] * 16384.0f,0);
   	comBuffer.add_Command(2,v[1] * 16384.0f,0);
#endif
       glTexCoord2fv(v);
   }
}
//---------------------------------------------------------------------------
static void put_Vertex()
{
   LPZ_BUFFER p;
   float v[4],f[4];
   u64 t,t1;
   s32 y,i,z,zpass;
#ifdef _DEBPRO
   s32 i_mtxClip[16],i_vtx[4];
#endif

   if(bChangeMatrix & 3)
       UpdateClipMatrix();
   if(!(nBeginMode & 0x80000000))
       return;
   v[0] = vtx[0] * (1.0f / 4096.0f);
   v[1] = vtx[1] * (1.0f / 4096.0f);
   v[2] = vtx[2] * (1.0f / 4096.0f);
   v[3] = 1.0f;

   if(crTex != NULL && crTex->coordMode == 3){
       f[0] = ((v[0] * mtxTexture[0] + v[1] * mtxTexture[4] + v[2] * mtxTexture[8]) * 0.0625f) + st[0];
       f[1] = ((v[0] * mtxTexture[1] + v[1] * mtxTexture[5] + v[2] * mtxTexture[9]) * 0.0625f) + st[1];
       if(crTex->h_size)
           f[0] /= crTex->h_size;
       if(crTex->v_size)
           f[1] /= crTex->v_size;
#ifdef _DEBPRO
       comBuffer.add_Command(2,f[0] * 16384.0f,0);
       comBuffer.add_Command(2,f[1] * 16384.0f,0);
#endif
       glTexCoord2f(f[0],f[1]);
   }
   pfn_vet4_mtx4x4(v,mtxClip);
#ifdef _DEBPRO
   if(LINO)
  		fprintf(LINO,"VTX_XYZW %f,%f,%f %f\n",v[0],v[1],v[2],v[3]);
#endif
   if(bOrtho){
       if(v[2] != 0)
           v[2] *= v[3] * .99999f;
       else if(v[3] != 1)
           v[2] = v[3] * .99999f;
   }
   if(bOldSwap & 4){
       f[0] = v[2] / v[3];
       f[1] = mtxClip[14] / mtxClip[15];
       if(f[1] > f[0])
           f[1] -= f[0];
       else if(f[0] > f[1])
           f[1] = f[0] - f[1];
       else
           f[1] += f[0];
       if((*((int *)&f[1]) == 0x7F800000 && PolyData[10] == 0)){
           f[1] = 0;
           return;
       }
       i = v[2] * v[3];
       if(i < -1025022)//Chess overflow
           v[2] = fabs(v[2]);
#ifdef _DEBPRO
/*       for(i=0;i<16;i++)
			i_mtxClip[i] = mtxClip[i] * 4096.0f;
		i_vtx[0] = vtx[0] * 4096.0f;
       i_vtx[1] = vtx[1] * 4096.0f;
       i_vtx[2] = vtx[2] * 4096.0f;
       i_vtx[3] = 4096;
		if(check_ndsoverflow((int *)i_vtx,(int *)&i_mtxClip[2],(int *)&i))
           v[2] = i / 4096.0f / 4096.0f;*/
#endif
   }
   if(bFogChanged & 4){
       f[0] = v[2];
       if(v[3] != 0)
           f[0] /= v[3];
       f[0] = fabs(f[0]);
       if(f[0] < fog[0])
           f[0] = 31.0f;
       else{
           if(f[0] > fog[1])
               f[0] = 31.0f;
           else{
               y = (f[0] - fog[0]) * fog[3];
               y = (v3d.io_mem[0x360 + y] & 0x7F) + 1;
               f[0] = ((color[7].get_color()[3] * y) + ((128 - y) * color[6].get_color()[3])) / 4.0f;
           }
       }
       y = 0;
		if((nOptions & NTR_DONT_USE_LIGHT) == 0){
           if((nOptions & NTR_USE_SOFT_LIGHT)){
       		color[8].set_alpha(f[0]);
               color[0].set_alpha(f[0]);
               y = 1;
           }
       }
       if(!y){
            for(i=0;i<5;i++){
                color[i].set_alpha(f[0]);
                color[i].apply();
            }
		}
   }
   f[0] = v[0];f[1] = v[1];f[2] = v[2];f[3] = v[3];
   if(*((int *)&f[3]) != 0){
       f[0] /= f[3];
       f[1] /= f[3];
       f[2] /= f[3];
   }
   f[3] = f[2];
   vet3_norm(f);
   if(*((int *)&f[2]) != 0){
       f[0] /= f[2];
       f[1] /= f[2];
   }
	p = NULL;
   y = (1.0f - f[1]) * 96.0f;
   if(y >= 0 && y < 192){
       i = ((1.0f - f[0]) * 128.0f);
       if(i >= 0 && i < 256){
           zpass = 0;
           p = &z_buffer[y * 256 + i];
           if(p->cr_frame != cr_frame){
               p->cr_frame = cr_frame;
               p->type = 0;
               p->value = 0;
               p->stencil = 0;
               p->flags = 0;
           }
           z = f[2] * 4096.0f;
           if(p->type == 0)
               zpass |= 1;
           else{
               if(dataPoly & 0x4000){
                   if(z == p->value)
                       zpass |= 1;
               }
               else{
                   if(z < p->value)
                       zpass |= 1;
               }
           }
           if(PolyData[6] == 5){
               if(!PolyData[10]){
                   p->stencil = (u8)((zpass & 1) ? 0 : 1);
                   zpass = 0;
               }
               else{
                   if(!p->stencil)
                       zpass = 0;
                   else if(!(zpass & 1))
                       p->stencil = 0;
               }
           }
           if(zpass & 1){
               if(VTXRAM_COUNT > 6143)
                   *((u16 *)&v3d.io_mem[0x60]) |= 0x2000;
               else
                   (VTXRAM_COUNT)++;
               p->id = (u8)(PolyData[10] + 1);
               p->id_internal = id_internal;
               if((p->flags = (u8)PolyData[9]))
                   bUpdateOutBuffer = 1;
               if(!(zpass & 2)){
                   p->value = z;
                   p->type = (u8)(nBeginMode+1);
					UpdatePolyRamCount();
               }
           }
       }
   }
#ifdef _DEBPRO
	comBuffer.add_Command(0,v[0] * 16384.0f,0);
   comBuffer.add_Command(0,v[1] * 16384.0f,0);
   comBuffer.add_Command(0,v[2] * 16384.0f,0);
   comBuffer.add_Command(0,v[3] * 16384.0f,0);
	if((nOptions & NTR_RENDER_ONLY_POLYGONS) &&
   	(id_poly_internal < id_poly_render_only[0] || id_poly_internal > id_poly_render_only[1]))
   		return;
#endif
	if((nOptions & NTR_DONT_USE_LIGHT) == 0){
       if((nOptions & NTR_USE_SOFT_LIGHT)){
       	if(nBeginMode & 0x40000000)
           	color[8].apply();
           else{
               if(dataPoly & 0xF){
                   color[8].set_alpha(poly_alpha);
                   color[8].apply();
               }
               else
                   color[0].apply();
           }
       }
       else
       	color[0].apply();
   }

   v[2] -= id_poly_internal * 0.000003f * fabs(f[2]);

   glVertex4fv(v);

   if((nOptions & (NTR_FOGCOORD_FLAG|NTR_SHADERS_OPTION)) == NTR_FOGCOORD_FLAG && (dataPoly & 0x8000) && bFog){
       i = *((int *)&v[2]);
       i &= ~0x80000000;
		f[0] = *((float *)&i);
       if(f[0] >= fog[1])
       	f[0] = 0;
       pfn_glFogCoordf(f[0]);
   }
   if(idxList && (bOldSwap & 2) == 0){
       if(transPoly.count() > 0){
           if(y > 255)
               y = 255;
           else if(y < 0)
               y = 0;
           t = transPoly.pop();
           i = (s32)(u8)(t >> 32);
           if(y > i)
               i = y;
           t1 = 0xFF;
           t1 <<= 32;
           t &= ~t1;
           t1 = (u64)(u8)i;
           t1 <<= 32;
           t |= t1;
           //       t = (u64)((t & ~t1)  | ((u64)(u8)(i << 32)));
           transPoly.push(t);
       }
   }
}
//---------------------------------------------------------------------------
static void get_TextureData(LPTEXTURE Tex,u32 data)
{
   Tex->format = (u8)((data >> 26) & 7);
   Tex->data = data;
   Tex->adr = ((u32)(u16)data) << 3;
   Tex->h_size = (8 << (((data >> 20) & 7)));
   Tex->v_size = (8 << (((data >> 23) & 7)));
   Tex->ws = (u8)((data >> 16) & 1);
   Tex->wt = (u8)((data >> 17) & 1);
	Tex->fs = (u8)((data >> 18) & 1) && Tex->ws;
   Tex->ft = (u8)((data >> 19) & 1) && Tex->wt;
	Tex->tr = (u8)((data >> 29) & 1);
   Tex->coordMode = (u8)(data >> 30);
}
//---------------------------------------------------------------------------
static void ioDISP3DCNT_MC(u32,u32 data,u8 am) //0x60
{
   *((u16 *)&v3d.io_mem[0x60]) &= (u16)(~(data & 0x3000));
   bControlChanged = 1;
}
//---------------------------------------------------------------------------
static void ioDISP3DCNT(u32,u32 data,u8 am) //0x60
{
   bControlChanged = 0;
	if(nBeginMode & 0x80000000){
       glEnd();
       nBeginMode &= ~0x80000000;
   }
   if((data & 1) && (nOptions & NTR_TEXTURE_OPTION)){
       glEnable(GL_TEXTURE_2D);
       PolyData[7] |= 1;
   }
   else{
       glDisable(GL_TEXTURE_2D);
       PolyData[7] &= ~1;
   }
   bCImage = (u32)((bCImage & ~1) | ((data >> 14) & 1));
	bToon = (u8)((data >> 1) & 1);
   if((nOptions & NTR_EDGE_MARKING_OPTION) && ((data >> 5) & 1))//OutLine Enabled
       PolyData[9] = 1;
   else
       PolyData[9] = 0;
   PolyData[9] += PolyData[12];
	bFog = (u8)((data >> 7) & 1);
   bAlphaTest &= ~1;
   bAlphaTest |= (u8)((data >> 2) & 1);
   if(bFogShift != ((data >> 8) & 0xF)){
       bFogShift = (u8)((data >> 8) & 0xF);
       bFogChanged |= 1;
   }
   if((data & 8) && (nOptions & NTR_BLEND_OPTION)){
      	glEnable(GL_BLEND);
   	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
       PolyData[14] = 1;
   }
   else{
       PolyData[14] = 0;
       glDisable(GL_BLEND);
   }
   if(data & 0x10){
       if(nOptions & NTR_MULTISAMPLE_SUPPORTED)
           glEnable(GL_MULTISAMPLE_ARB);
       else{
//   	    glEnable(GL_POLYGON_SMOOTH);
   	    glEnable(GL_LINE_SMOOTH);
           glEnable(GL_POINT_SMOOTH);
       }
   }
   else{
       if(nOptions & NTR_MULTISAMPLE_SUPPORTED)
           glDisable(GL_MULTISAMPLE_ARB);
       else{
//      	    glDisable(GL_POLYGON_SMOOTH);
           glDisable(GL_LINE_SMOOTH);
           glDisable(GL_POINT_SMOOTH);
       }
   }
   *((u16 *)&v3d.io_mem[0x60]) &= (u16)(~(data & 0x3000));
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"DISP3DCNT 0x%04X\n",data);
#endif
}
//---------------------------------------------------------------------------
static BOOL BeginPolygon()
{
	s32 index;
   float f[4];
   u64 t;

	if(polyData == (u32)-1)
   	return FALSE;
	poly_alpha = (u8)((polyData >> 16) & 0x1F);
   if(poly_alpha == 0){
       glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
       poly_alpha = 31;
   }
   else{
       if(poly_alpha < 31 || (crTex != NULL && (crTex->format == 6 || crTex->format == 1))){
       	if(poly_alpha == 31)
           	poly_alpha = 30;
           if(!(nOptions & NTR_DONT_USE_LISTS)){
               index = glGenLists(20);
               if(glIsList(index) == GL_TRUE){
                   idxList = index;
                   glNewList(idxList,GL_COMPILE);
                   t = polyData & 0xFFFF0000;
                   t = (t << 32) | idxList;
                   transPoly.push(t);
                   ioDISP3DCNT(0,((u16 *)&io_mem[0x60])[0],AMM_HWORD);
                   bFogChanged |= 0xF;
               }
           }
       }
       glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
   }
   if(nOptions & NTR_RENDER_WIREFRAME)
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
   glDisable(GL_STENCIL_TEST);
   if(bFog && (polyData & 0x8000) && (nOptions & NTR_FOG_OPTION)){
       if((v3d.io_mem[0x60] & 0x40)){
           glDisable(GL_FOG);
           if(nOptions & NTR_FOG_ALPHA_OPTION){
               if(nOptions & NTR_SHADERS_OPTION){
                   bFogChanged &= ~4;
                   PolyData[5] = 2;
               }
               else
                   bFogChanged |= 4;
           }
           else{
               PolyData[5] = 0;
               bFogChanged &= ~4;
           }
       }
       else{
           bFogChanged &= ~4;
           glEnable(GL_FOG);
           PolyData[5] = 1;
       }
   }
   else{
       glDisable(GL_FOG);
       bFogChanged &= ~4;
       PolyData[5] = 0;
   }
   PolyData[11] = poly_alpha;
   color[0].set_alpha(poly_alpha);
   color[1].set_alpha(poly_alpha);
   color[2].set_alpha(poly_alpha);
   color[3].set_alpha(poly_alpha);
   color[4].set_alpha(poly_alpha);
   color[5].set_alpha(poly_alpha);
   color[6].set_alpha(poly_alpha);

   switch(polyData & 0x30){
       case 0:
           glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
           glDepthMask(GL_TRUE);
           if(!(nOptions & NTR_SHADERS_OPTION)){
               if(nOptions & NTR_TEX_COMBINE_FLAG){
                   glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);

                   glTexEnvf(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_MODULATE);

                   glTexEnvf(GL_TEXTURE_ENV,GL_SOURCE0_RGB,GL_TEXTURE);
                   glTexEnvf(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
                   glTexEnvf(GL_TEXTURE_ENV,GL_SOURCE1_RGB,GL_PRIMARY_COLOR);
                   glTexEnvf(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);

                   glTexEnvf(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
                   glTexEnvf(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA,GL_TEXTURE);
                   glTexEnvf(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA,GL_SRC_ALPHA);
                   glTexEnvf(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA,GL_PRIMARY_COLOR);
                   glTexEnvf(GL_TEXTURE_ENV,GL_OPERAND1_ALPHA,GL_SRC_ALPHA);
               }
               else{
                   glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,color[0].get_color());
                   glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
               }
           }
           PolyData[6] = 1;
       break;
       case 0x10:
           glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
           glDepthMask(GL_TRUE);
           if(!(nOptions & NTR_SHADERS_OPTION)){
           	if(nOptions & NTR_TEX_COMBINE_FLAG){
                   glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);

                   glTexEnvf(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_INTERPOLATE);

                   glTexEnvf(GL_TEXTURE_ENV,GL_SOURCE0_RGB,GL_TEXTURE);
                   glTexEnvf(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
                   glTexEnvf(GL_TEXTURE_ENV,GL_SOURCE1_RGB,GL_PRIMARY_COLOR);
                   glTexEnvf(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);

                   glTexEnvf(GL_TEXTURE_ENV,GL_SOURCE2_RGB,GL_TEXTURE);
                   glTexEnvf(GL_TEXTURE_ENV,GL_OPERAND2_ALPHA,GL_SRC_ALPHA);

                   glTexEnvf(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_REPLACE);
                   glTexEnvf(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA,GL_PRIMARY_COLOR);
                   glTexEnvf(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA,GL_SRC_ALPHA);
               }
               else{
                   glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
                   glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,color[0].get_color());
               }
           }
           PolyData[6] = 2;
       break;
       case 0x20:
           glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
           glDepthMask(GL_TRUE);
           if(!(nOptions & NTR_SHADERS_OPTION)){
               index = color[0].get_color()[0] * 31.0f;
               f[0] = (toonColor[index] & 0xFF) / 255.0f;
               f[1] = ((toonColor[index] >> 8) & 0xFF) / 255.0f;
               f[2] = ((toonColor[index] >> 16) & 0xFF) / 255.0f;
               f[3] = color[0].get_color()[3];
               if(!bToon)
                   glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
               else{
                   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
                   glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_INTERPOLATE);
                   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB,GL_TEXTURE);
                   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB,GL_SRC_COLOR);
                   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB,GL_CONSTANT);
                   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB,GL_SRC_COLOR);
                   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB,GL_CONSTANT);
                   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB,GL_SRC_COLOR);
                   glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
                   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA,GL_TEXTURE);
                   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA,GL_SRC_ALPHA);
                   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA,GL_CONSTANT);
                   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_ALPHA,GL_SRC_ALPHA);
               }
               glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,f);
           }
           PolyData[6] = !bToon ? 3 : 4;
       break;
       case 0x30:
           PolyData[6] = 5;
           if(!PolyData[10]){
               glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
               glDepthMask(GL_FALSE);
               glEnable(GL_STENCIL_TEST);
               glStencilFunc(GL_ALWAYS,1,0xFFFFFFFF);
               glStencilOp(GL_ZERO,GL_REPLACE,GL_ZERO);
           }
           else{
               glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
               glDepthMask(GL_TRUE);
               glEnable(GL_STENCIL_TEST);
               glStencilFunc(GL_EQUAL,1,0xFFFFFFFF);
               glStencilOp(GL_ZERO,GL_ZERO,GL_KEEP);
           }
       break;
   }
   if((polyData & 0xC0) == 0xC0)
       glDisable(GL_CULL_FACE);
   else{
       glEnable(GL_CULL_FACE);
       glCullFace(((polyData & 0x40) ? GL_FRONT : (polyData & 0x80) ? GL_BACK : GL_FRONT_AND_BACK));
   }
   color[0].apply();
   color[1].apply();
   color[2].apply();
   color[3].apply();
   color[4].apply();
   color[5].apply();
   PolyData[0] = 0;
   glDisable(GL_LIGHTING);
   for(index = 0;index < 4;index++){
       light[index].enable(FALSE);
       PolyData[index + 1] = 0;
   }
   if(dataPoly & 0xF)
   	color[8] = color[2];
   polyData = -1;
   return TRUE;
}
//---------------------------------------------------------------------------
static BOOL BeginMode(int from)
{
//   float m1[16];
   int i1;
	LPTEXTURE Tex;
   float f[4];
   u64 t,t1;

	if(texData == (u32)-1)
		goto BeginMode_01;
   bAlphaTest &= 0x7F;
 	if(nBeginMode & 0x80000000){
#if defined(_DEBPRO)
       if(LINO)
   	    fprintf(LINO,"END_VTXS\n");
       comBuffer.add_Command(0x504,0,AMM_WORD);
#endif
   	glEnd();
       nBeginMode &= ~0xF0FF0000;
   }
#ifdef _DEBPRO
   comBuffer.add_Command(0x4A8,dataTex,AMM_WORD);
   comBuffer.add_Command(0x4AC,texPalette,AMM_WORD);
#endif
  	if(!texData){
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"BeginMode-0 %d %d\n",(int)from,(int)id_poly_internal);
#endif
		glBindTexture(GL_TEXTURE_2D,0);
       crTex = NULL;
       texData = (u32)-1;
       PolyData[7] &= ~2;
   	goto BeginMode_01;
   }
   if(texIndex != (u32)-1){
       if(idxList)
           glEndList();
   	if(CreateTexture(NULL,texIndex))
           texIndex = (u32)-1;
       if(idxList){
           if(glIsList(++idxList) == GL_TRUE){
               glNewList(idxList,GL_COMPILE);
               t = transPoly.pop();
               i1 = (int)(((t >> 20) & 0xFFF) + 1);
               t1 = (t >> 32) << 32;
               t &= ~0xFFF00000;
               t |= (i1 << 20);
               t |= t1;
		        transPoly.push(t);
           }
       }
   }
   for(i1=0;i1<idxTxt;i1++){
       if(tex[i1].data == texData){
           if((tex[i1].format < 7 && tex[i1].palData == texPalette && tex[i1].palmem_adr == texpalMem)
               || tex[i1].format > 6){
               BindTexture(i1);
               texData = (u32)-1;
               goto BeginMode_01;
           }
       }
   }
   for(i1=0;i1<idxTxt;i1++){
       if(tex[i1].index == 0 && tex[i1].data == 0)
           break;
   }
   Tex = &tex[i1];
	get_TextureData(Tex,texData);
   if(Tex->format == 0){
       glBindTexture(GL_TEXTURE_2D,0);
       crTex = NULL;
       texData = (u32)-1;
       goto BeginMode_01;
   }
	if((texIndex = i1) == idxTxt)
   	idxTxt++;
   if(idxList)
       glEndList();
   if(CreateTexture(NULL,texIndex))
       texIndex = (u32)-1;
   texData = (u32)-1;
   if(idxList){
       if(glIsList(++idxList) == GL_TRUE){
           glNewList(idxList,GL_COMPILE);
           t = transPoly.pop();
           i1 = (int)(((t >> 20) & 0xFFF) + 1);
           t1 = (t >> 32) << 32;
           t &= ~0xFFF00000;
           t |= (i1 << 20);
           t |= t1;
		    transPoly.push(t);
       }
   }
BeginMode_01:
   if(BeginPolygon()){
   	if(crTex == NULL){
           glBindTexture(GL_TEXTURE_2D,0);
#if defined(_DEBPRO)
           if(LINO)
               fprintf(LINO,"BeginMode-1 %d %d\n",(int)from,(int)id_poly_internal);
#endif
       }
       else
       	BindTexture(crTex);
   }
   if((nBeginMode & 0x80000000))
   	return TRUE;
//   nBeginMode &= 0xFF00FFFF;
	if(from == 2){
   	nBeginMode |= (from << 28);
       return FALSE;
   }
/*   i1 = (nBeginMode >> 16) & 0xFF;
   nBeginMode &= 0xFF00FFFF;
   if(i1++ == 0){
       if(from == 1){
           nBeginMode |= (i1 << 16);
       	if(!(nBeginMode & 0x60000000)){
       		vtx[3] = vtx[0];vtx[4] = vtx[1];vtx[5] = vtx[2];
               return FALSE;
           }
       }
       else{
       	nBeginMode |= (from << 28);
       	return FALSE;
       }
   }
   else{
   	if(from == 1)
       	nBeginMode |= (i1 << 16);
       else
			nBeginMode |= (from << 28);
   }*/
   glEnd();

   if((nOptions & NTR_ALPHA_TEST_OPTION) && ((bAlphaTest & 0x81) || (crTex != NULL && (crTex->tr || crTex->format == 1 || crTex->format > 4)))){
   	glEnable(GL_ALPHA_TEST);
   	if(bAlphaTest & 1){
           PolyData[13] = ((bAlphaTest >> 1) & 31) + 1;
           glAlphaFunc(GL_GREATER,(PolyData[13] - 1) * (1.0f / 31.0f));
       }
       else{
           PolyData[13] = 1;
           glAlphaFunc(GL_GREATER,0);
       }
   }
   else{
       glDisable(GL_ALPHA_TEST);
       PolyData[13] = 0;
   }
	if(from == 4){
   	nBeginMode |= 0x50000000;
       if((nOptions & NTR_DONT_USE_LIGHT) == 0){
           if((nOptions & NTR_USE_SOFT_LIGHT) == 0){
               glEnable(GL_LIGHTING);
               if(!(dataPoly & 0xF)){
                   f[0] = f[1] = f[2] = 1.0f;
                   f[3] = color[4].get_color()[3];
                   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,f);
               }
               PolyData[0] = 1;
               PolyData[1] = (dataPoly & 1) && (nOptions & NTR_LIGHT0_OPTION);
               PolyData[2] = (dataPoly & 2) && (nOptions & NTR_LIGHT1_OPTION);
               PolyData[3] = (dataPoly & 4) && (nOptions & NTR_LIGHT2_OPTION);
               PolyData[4] = (dataPoly & 8) && (nOptions & NTR_LIGHT3_OPTION);

               light[0].enable(PolyData[1]);
               light[1].enable(PolyData[2]);
               light[2].enable(PolyData[3]);
               light[3].enable(PolyData[4]);
           }
           else
         		glDisable(GL_LIGHTING);
       }
       if((dataPoly & 0xF) != 0 && (nOptions & NTR_SHADERS_OPTION) == 0 && crTex != NULL){
           switch(dataPoly & 0x30){
               case 0x20:
                   i1 = color[0].get_color()[0] * 31.0f;
                   f[0] = (toonColor[i1] & 0xFF) / 255.0f;
                   f[1] = ((toonColor[i1] >> 8) & 0xFF) / 255.0f;
                   f[2] = ((toonColor[i1] >> 16) & 0xFF) / 255.0f;
                   f[3] = color[0].get_color()[3];
               break;
               default:
                   f[0] = color[0].get_color()[0];
                   f[1] = color[0].get_color()[1];
                   f[2] = color[0].get_color()[2];
                   f[3] = color[0].get_color()[3];
               break;
           }
           for(i1=0;i1<4;i1++){
               if((dataPoly & (1<<i1)) && (nOptions & (1 << (10+i1)))){
                   f[0] += color[2].get_color()[0] * light[i1].get_color()[0];
                   f[1] += color[2].get_color()[1] * light[i1].get_color()[1];
                   f[2] += color[2].get_color()[2] * light[i1].get_color()[2];
                   f[3] += color[2].get_color()[3] * light[i1].get_color()[3];
               }
           }
           for(i1=0;i1<4;i1++){
               if(f[i1] > 1)
                   f[i1] = 1;
           }
           glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,f);
   	}
   }
   if(bChangeMatrix & 3)
       UpdateClipMatrix();
 	if(dataPoly & 0x4000)
       glDepthFunc(GL_EQUAL);
   else
       glDepthFunc(GL_LESS);
   if((nOptions & NTR_KEEP_TRANSLUCENT) && (dataPoly & 0x800) == 0 &&
   	((poly_alpha > 0 && poly_alpha < 31))){
       if(!(dataPoly & 0x4000))
           glDepthFunc(GL_LEQUAL);
   }
   if((bFogChanged & 6) == 2){
       glFogfv(GL_FOG_COLOR,color[7].get_color());
       if(idxList == 0)
           bFogChanged &= ~2;
   }
   if(bFogChanged & 1){
       i1 = 0x400 >> bFogShift;
       fog[0] = (((v3d.io_mem[0x360] & 0x7F) + 1) * i1) / 131072.0f;
       fog[1] = (((v3d.io_mem[0x37F] & 0x7F) + 1) * i1) / 131072.0f;

       fog[2] = ((((u16 *)&v3d.io_mem[0x35C])[0] & 0x7FFF) / 32768.0f);

       fog[0] += fog[2];
       fog[1] += fog[2];
/*       if(v3d.io_mem[0x540] & 2){
           f[0] = 1 - fog[1];
           fog[1] = 1 - fog[0];
           fog[0] = f[0];
       }*/
       f[0] = fog[1] - fog[0];
       if(*((int *)f) != 0)
           fog[3] = 31.0f / f[0];
       else{
           fog[3] = 1;
           fog[1] += 1.e-06f;
       }
       bFogChanged &= ~1;
   }
   if(bFog && !(bFogChanged & 4) && (dataPoly & 0x8000)){
       glFogf(GL_FOG_START,fog[0]);
       glFogf(GL_FOG_END,fog[1]);
   }
   if((nOptions & NTR_SHADERS_OPTION)){
       UpdateToonFogTable();
       i1 = pfn_glGetUniformLocation(shaderPrg,"PolyData");
       pfn_glUniform1iv(i1,20,PolyData);
       i1 = pfn_glGetUniformLocation(shaderPrg,"tex");
       pfn_glUniform1i(i1,0);
       i1 = pfn_glGetUniformLocation(shaderPrg,"ToonColor");
       pfn_glUniform1i(i1,1);
   }
   id_internal++;
   glBegin((nBeginMode & ~0xF0FF0000));
/*   if(((nBeginMode & 0xFF0000) >> 16) > 1){
       vtx[6] = vtx[0];vtx[7]=vtx[1];vtx[8] = vtx[2];
       vtx[0] = vtx[3];vtx[1]=vtx[4];vtx[2] = vtx[5];
		put_Vertex();
       vtx[0] = vtx[6];vtx[1]=vtx[7];vtx[2] = vtx[8];
   }*/
   if((nBeginMode & 0x20000000) && from != 2)
   	put_TexCoord();
//   if((nBeginMode & 0x40000000) && from != 4)
//   	put_Normal();
	nBeginMode &= ~0xB0FF0000;
   nBeginMode |= 0x80000000;
   return TRUE;
}
//---------------------------------------------------------------------------
static void ioALPHA_TEST_REF(u32,u32 data,u8 am) // 0x340
{
   bAlphaTest = (u8)(((data & 0x1F) << 1) | (bAlphaTest & 1));
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"ALPHA_TEST_REF 0x%04X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioCLEAR_COLOR(u32,u32 data,u8 am) // 0x350
{
	glClearColor((data & 0x1F) * (1.0f / 31.0f),((data >> 5) & 0x1F) * (1.0f / 31.0f),
       ((data >> 10) & 0x1F) * (1.0f / 31.0f),((data >> 16) & 0x1F) * (1.0f / 31.0f));
   PolyData[8] = ((data >> 24) & 63);
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"CLEAR_COLOR 0x%08X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioCLEAR_DEPTH(u32,u32 data,u8 am) // 354
{
   data &= 0x7FFF;
  	glClearDepth(data / 32767.0f);
   if(bOldSwap & 4)
       glDepthRange(0,data / 32767.0f);
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"CLEAR_DEPTH 0x%04X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioCLEAR_IMAGE(u32,u32 data,u8 am) // 354
{
   if((bCImage >> 16) != data)
       bCImage = ((bCImage & 0xFFFF) | 2) | (data << 16);
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"CLEAR_IMAGE 0x%04X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioFOG_COLOR(u32,u32 data,u8 am) // 358
{
   color[7].set((data & 0x1F),((data >> 5) & 0x1F),
   	((data >> 10) & 0x1F),((data >> 16) & 0x1F));
   bFogChanged |= 2;
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"FOG_COLOR 0x%08X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioTOON_TABLE(u32 adr,u32 data,u8 am) // 358
{
   int r,g,b;

	adr = ((adr - 0x380) >> 1);
   do{
       r = (int)ceil((data & 0x1F) * 8.22f);
       g = (int)ceil(((data >> 5) & 0x1F) * 8.22f);
       b = (int)ceil(((data >> 10) & 0x1F) * 8.22f);
   	toonColor[adr++] = RGB(r,g,b);
       data >>= 16;
       am >>= 1;
   }while(am > 2);
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"TOON_TABLE 0x%04X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioEDGE_COLOR(u32 adr,u32 data,u8 am) // 330
{
   int r,g,b;

	adr = ((adr - 0x330) >> 1);
   do{
       r = (int)ceil((data & 0x1F) * 8.22f);
       g = (int)ceil(((data >> 5) & 0x1F) * 8.22f);
       b = (int)ceil(((data >> 10) & 0x1F) * 8.22f);
   	edgeColor[adr++] = RGB(r,g,b);
       data >>= 16;
       am >>= 1;
   }while(am > 2);
   bEdgeChanged = TRUE;
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"EDGE_COLOR 0x%04X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_MODE(u32 adr,u32 data,u8 am) // 440
{
   switch((mtxMode = (data & 3))){
       case 0:
           crS = &sPM;
       break;
       case 1:
           crS = &sMV;
       break;
       case 2:
           crS = &sMP;
       break;
       case 3:
           crS = &sTX;
       break;
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_MODE 0x%04X\n",data);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_PUSH(u32 adr,u32 data,u8 am) // 444
{
   if(crS != NULL){
       crS->push();
       if(mtxMode == 2)
           sMV.push();
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_PUSH 0x%04X\n",mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_POP(u32 adr,u32 data,u8 am) // 448
{
	int index;

   if(crS != NULL){
       index = (data & 0x3F);
       if((data & 0x20))
           index |= 0xFFFFFFC0;
       crS->pop(index);
       switch(mtxMode){
           case NTR_POSITION_MATRIX:
               sMV.pop(index);
               bChangeMatrix |= 2;
           break;
           case NTR_MODELVIEW_MATRIX:
               bChangeMatrix |= 2;
           break;
           case NTR_PROJECTION_MATRIX:
               bChangeMatrix |= 1;
           break;
       }
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_POP 0x%04X %d\n",mtxMode,index);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_STORE(u32 adr,u32 data,u8 am) // 44c
{
   if(crS != NULL){
       crS->store(data & 0x1F);
       if(mtxMode == 2)
           sMV.store(data & 0x1F);
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_STORE %d 0x%04X\n",data,mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_RESTORE(u32 adr,u32 data,u8 am) // 450
{
   if(crS != NULL){
       crS->restore(data & 0x1F);
       switch(mtxMode){
           case NTR_POSITION_MATRIX:
               sMV.restore(data & 0x1F);
               bChangeMatrix |= 2;
           break;
           case NTR_MODELVIEW_MATRIX:
               bChangeMatrix |= 2;
           break;
           case NTR_PROJECTION_MATRIX:
               bChangeMatrix |= 1;
           break;
       }
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_RESTORE %d 0x%04X\n",data,mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_IDENTITY(u32 adr,u32,u8) // 454
{
   switch(mtxMode){
       case NTR_MODELVIEW_MATRIX:
     	    memcpy(mtxView,mtx4x4I,sizeof(mtx4x4I));
           bChangeMatrix |= 2;
       break;
       case NTR_POSITION_MATRIX:
           memcpy(mtxPosition,mtx4x4I,sizeof(mtx4x4I));
           memcpy(mtxView,mtx4x4I,sizeof(mtx4x4I));
           bChangeMatrix |= 2;
       break;
       case NTR_TEXTURE_MATRIX:
           memcpy(mtxTexture,mtx4x4I,sizeof(mtx4x4I));
       break;
       case NTR_PROJECTION_MATRIX:
           memcpy(mtxProjection,mtx4x4I,sizeof(mtx4x4I));
           bChangeMatrix |= 1;
       break;
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_IDENTITY 0x%04X\n",mtxMode);
   comBuffer.add_Command(adr,0,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_LOAD_4X4(u32 adr,u32 data,u8) // 458
{
  	tm44[idxLoad4x4++] = (int)data * (1.0f / 4096.0f); //.00024414063f
   if(idxLoad4x4 != 16)
       return;
  	idxLoad4x4 = 0;
   switch(mtxMode){
       case NTR_MODELVIEW_MATRIX:
           memcpy(mtxView,tm44,sizeof(tm44));
           bChangeMatrix |= 2;
       break;
       case NTR_PROJECTION_MATRIX:
           memcpy(mtxProjection,tm44,sizeof(tm44));
           bChangeMatrix |= 1;
       break;
       case NTR_TEXTURE_MATRIX:
           memcpy(mtxTexture,tm44,sizeof(tm44));
       break;
       case NTR_POSITION_MATRIX:
           memcpy(mtxPosition,tm44,sizeof(tm44));
           memcpy(mtxView,tm44,sizeof(tm44));
           bChangeMatrix |= 2;
       break;
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_LOAD_4X4 0x%04X\n",mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_LOAD_4X3(u32 adr,u32 data,u8) // 45C
{
	tm43[idxLoad4x3++] = (int)data * (1.0f / 4096.0f);
	if(idxLoad4x3 != 12)
       return;
   idxLoad4x3 = 0;
   tm44[0] = tm43[0];
   tm44[1] = tm43[1];
	tm44[2] = tm43[2];
	tm44[3] = 0;
	tm44[4] = tm43[3];
	tm44[5] = tm43[4];
	tm44[6] = tm43[5];
	tm44[7] = 0;
	tm44[8] = tm43[6];
	tm44[9] = tm43[7];
	tm44[10] = tm43[8];
	tm44[11] = 0;
	tm44[12] = tm43[9];
	tm44[13] = tm43[10];
	tm44[14] = tm43[11];
   tm44[15] = 1;
   switch(mtxMode){
       case NTR_MODELVIEW_MATRIX:
           memcpy(mtxView,tm44,sizeof(tm44));
           bChangeMatrix |= 2;
       break;
       case NTR_PROJECTION_MATRIX:
           memcpy(mtxProjection,tm44,sizeof(tm44));
           bChangeMatrix |= 1;
       break;
       case NTR_TEXTURE_MATRIX:
           memcpy(mtxTexture,tm44,sizeof(tm44));
       break;
       case NTR_POSITION_MATRIX:
           memcpy(mtxPosition,tm44,sizeof(tm44));
           memcpy(mtxView,tm44,sizeof(tm44));
           bChangeMatrix |= 2;
       break;
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_LOAD_4X3 0x%04X\n",mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_MULT_4X4(u32 adr,u32 data,u8) // 460
{
	tm44[idxMul4x4++] = (int)data * (1.0f / 4096.0f);
   if(idxMul4x4 != 16)
       return;
 	idxMul4x4 = 0;
   switch(mtxMode){
       case NTR_MODELVIEW_MATRIX:
          	pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
       case NTR_PROJECTION_MATRIX:
           pfn_mtx4x4_mult(mtxProjection,tm44);
           bChangeMatrix |= 1;
       break;
       case NTR_TEXTURE_MATRIX:
           pfn_mtx4x4_mult(mtxTexture,tm44);
       break;
       case NTR_POSITION_MATRIX:
           pfn_mtx4x4_mult(mtxPosition,tm44);
          	pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_MULT_4X4 0x%04X\n",mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_MULT_4X3(u32 adr,u32 data,u8) // 464
{
	tm43[idxMul4x3++] = (int)data * (1.0f / 4096.0f);
	if(idxMul4x3 != 12)
       return;
  	idxMul4x3 = 0;
   tm44[0] = tm43[0];
   tm44[1] = tm43[1];
	tm44[2] = tm43[2];
	tm44[3] = 0;
	tm44[4] = tm43[3];
	tm44[5] = tm43[4];
	tm44[6] = tm43[5];
	tm44[7] = 0;
	tm44[8] = tm43[6];
	tm44[9] = tm43[7];
	tm44[10] = tm43[8];
	tm44[11] = 0;
	tm44[12] = tm43[9];
	tm44[13] = tm43[10];
	tm44[14] = tm43[11];
   tm44[15] = 1;
   switch(mtxMode){
       case NTR_MODELVIEW_MATRIX:
           pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
       case NTR_PROJECTION_MATRIX:
           pfn_mtx4x4_mult(mtxProjection,tm44);
           bChangeMatrix |= 1;
       break;
       case NTR_TEXTURE_MATRIX:
           pfn_mtx4x4_mult(mtxTexture,tm44);
       break;
       case NTR_POSITION_MATRIX:
           pfn_mtx4x4_mult(mtxPosition,tm44);
           pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_MULT_4X3 0x%04X\n",mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_MULT_3X3(u32 adr,u32 data,u8) // 468
{
   mm33[idxMul3x3++] = (int)data * (1.0f / 4096.0f);
   if(idxMul3x3 != 9)
       return;
 	idxMul3x3 = 0;
   tm44[0] = mm33[0];
   tm44[1] = mm33[1];
	tm44[2] = mm33[2];
	tm44[3] = 0;
	tm44[4] = mm33[3];
	tm44[5] = mm33[4];
	tm44[6] = mm33[5];
	tm44[7] = 0;
	tm44[8] = mm33[6];
	tm44[9] = mm33[7];
	tm44[10] = mm33[8];
	tm44[11] = 0;
	tm44[12] = 0;
	tm44[13] = 0;
	tm44[14] = 0;
   tm44[15] = 1;
   switch(mtxMode){
       case NTR_MODELVIEW_MATRIX:
           pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
       case NTR_PROJECTION_MATRIX:
           pfn_mtx4x4_mult(mtxProjection,tm44);
           bChangeMatrix |= 1;
       break;
       case NTR_TEXTURE_MATRIX:
           pfn_mtx4x4_mult(mtxTexture,tm44);
       break;
       case NTR_POSITION_MATRIX:
           pfn_mtx4x4_mult(mtxPosition,tm44);
           pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_MULT_3X3 0x%04X\n",mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_SCALE(u32 adr,u32 data,u8) // 46c
{
	ts[idxScale++] = (int)data * (1.0f / 4096.0f);
   if(idxScale != 3)
       return;
  	idxScale = 0;
   memcpy(tm44,mtx4x4I,sizeof(mtx4x4I));
   tm44[0] = ts[0];
   tm44[5] = ts[1];
   tm44[10] = ts[2];
   switch(mtxMode){
       case NTR_MODELVIEW_MATRIX:
           pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
       case NTR_PROJECTION_MATRIX:
           pfn_mtx4x4_mult(mtxProjection,tm44);
           bChangeMatrix |= 1;
       break;
       case NTR_TEXTURE_MATRIX:
           pfn_mtx4x4_mult(mtxTexture,tm44);
       break;
       case NTR_POSITION_MATRIX:
          	pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_SCALE 0x%04X\n",mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioMTX_TRANS(u32 adr,u32 data,u8) // 470
{
	tt[idxTrans++] = (int)data * (1.0f / 4096.0f);
   if(idxTrans != 3)
       return;
  	idxTrans = 0;
   memcpy(tm44,mtx4x4I,sizeof(mtx4x4I));
   tm44[12] = tt[0];
   tm44[13] = tt[1];
   tm44[14] = tt[2];
   switch(mtxMode){
       case NTR_MODELVIEW_MATRIX:
           pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
       case NTR_PROJECTION_MATRIX:
 	        pfn_mtx4x4_mult(mtxProjection,tm44);
           bChangeMatrix |= 1;
       break;
       case NTR_TEXTURE_MATRIX:
           pfn_mtx4x4_mult(mtxTexture,tm44);
       break;
       case NTR_POSITION_MATRIX:
           pfn_mtx4x4_mult(mtxPosition,tm44);
          	pfn_mtx4x4_mult(mtxView,tm44);
           bChangeMatrix |= 2;
       break;
   }
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"MTX_TRANS 0x%04X\n",mtxMode);
   comBuffer.add_Command(adr,data,0);
#endif
}
//---------------------------------------------------------------------------
static void ioCOLOR(u32 adr,u32 data,u8) // 480
{
   color[0].set((data & 0x1F),((data >> 5) & 0x1F),((data >> 10) & 0x1F));
   color[6] = color[0];
//   color[0].apply();
	color[8] = color[0];//Super Mario 64 doenst work
#if defined(_DEBPRO)
	comBuffer.add_Command(adr,data,AMM_WORD);
   if(LINO)
   	fprintf(LINO,"COLOR 0x%04X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioNORMAL(u32 adr,u32 data,u8) // 484
{
   s32 x;

	x = (s32)(data & 0x3FF);
   if((x & 0x200))
     	x |= 0xFFFFFC00;
   nor[0] = x * 0.001953125f;
	x = (s32)((data >> 10) & 0x3FF);
   if((x & 0x200))
      	x |= 0xFFFFFC00;
	nor[1] = x * 0.001953125f;
	x = (s32)((data >> 20) & 0x3FF);
   if((x & 0x200))
      	x |= 0xFFFFFC00;
	nor[2] = x * 0.001953125f;
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"NORMAL %f,%f,%f\n",nor[0],nor[1],nor[2]);
#endif
   if(!BeginMode(4))
   	return;
   put_Normal();
}
//---------------------------------------------------------------------------
static void ioTEXCOORD(u32 adr,u32 data,u8) // 488
{
   st[0] = (s16)data * 0.0625f;
   st[1] = (s16)(data >> 16) * 0.0625f;
   if(!BeginMode(2))
   	return;
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"TEXCOORD %f,%f\n",st[0],st[1]);
#endif
   put_TexCoord();
}
//---------------------------------------------------------------------------
static void io_VTX_EMPTY(u32 adr,u32 data,u8) // 498
{
   int i;

   i = 0;
   ulVertices++;
   switch(*((u16 *)&v3d.io_mem[0x500]) & 3){
       case 0:
           if(ulVertices > 2){
               ulVertices = 0;
               i = 1;
           }
       break;
       case 2:
           if(ulVertices > 2){
               i = 1;
               ulVertices -= 2;
           }
       break;
       case 1:
           if(ulVertices > 3){
               ulVertices = 0;
               i = 1;
           }
       break;
       case 3:
           if(ulVertices > 3){
               ulVertices -= 3;
               i = 1;
           }
       break;
   }
   if(ulPolyRam <= 2048){
       ulPolyRam += i;
       (LISTRAM_COUNT) = (u16)ulPolyRam;
   }
   else
       *((u16 *)&v3d.io_mem[0x60]) |= 0x2000;
}
//---------------------------------------------------------------------------
static void ioVTX_XZ(u32 adr,u32 data,u8) // 498
{
	vtx[0] = (s16)data;
	vtx[2] = (s16)(data >> 16);
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"VTX_XZ %f,%f,%f\n",vtx[0],vtx[1],vtx[2]);
#endif
	if(!BeginMode(1))
   	return;
	put_Vertex();
}
//---------------------------------------------------------------------------
static void ioVTX_YZ(u32 adr,u32 data,u8) // 49C
{
	vtx[1] = (s16)data;
	vtx[2] = (s16)(data >> 16);
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"VTX_YZ %f,%f,%f\n",vtx[0],vtx[1],vtx[2]);
#endif
	if(!BeginMode(1))
   	return;
	put_Vertex();
}
//---------------------------------------------------------------------------
static void ioVTX_XY(u32 adr,u32 data,u8) // 494
{
	vtx[0] = (s16)data;
	vtx[1] = (s16)(data >> 16);
#if defined(_DEBPRO)
   if(LINO)
  		fprintf(LINO,"VTX_XY %f,%f,%f\n",vtx[0],vtx[1],vtx[2]);
#endif
	if(!BeginMode(1))
   	return;
	put_Vertex();
}
//---------------------------------------------------------------------------
static void ioVTX_DIFF(u32 adr,u32 data,u8) // 4A0
{
	s32 x;

	x = (s32)data;
   vtx[0] += ((x<<22) >> 22);
   vtx[1] += ((x<<12) >> 22);
   vtx[2] += ((x<<2) >> 22);

#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"VTX_DIFF %f,%f,%f\n",vtx[0],vtx[1],vtx[2]);
#endif
	if(!BeginMode(1))
   	return;
	put_Vertex();
}
//---------------------------------------------------------------------------
static void ioVTX_16(u32 adr,u32 data,u8) // 48C
{
	int i;

	for(i = 0;i < 2 && idxVtx < 3;i++){
		vtx[idxVtx++] = (s16)data;
       data >>= 16;
   }
	if(idxVtx != 3)
       return;
 	idxVtx = 0;
#if defined(_DEBPRO)
   if(LINO)
     	fprintf(LINO,"VTX_16 %f,%f,%f\n",vtx[0],vtx[1],vtx[2]);
#endif
	if(!BeginMode(1))
   	return;
	put_Vertex();
}
//---------------------------------------------------------------------------
static void ioVTX_10(u32 adr,u32 data,u8) // 490
{
	s32 x;

	x = (s32)data;
   vtx[0] = (((x<<22) >> 22)<<6);
   vtx[1] = (((x<<12) >> 22)<<6);
   vtx[2] = (((x<<2) >> 22)<<6);
#if defined(_DEBPRO)
   if(LINO)
     	fprintf(LINO,"VTX_10 %f,%f,%f\n",vtx[0],vtx[1],vtx[2]);
#endif
	if(!BeginMode(1))
   	return;
	put_Vertex();
}
//---------------------------------------------------------------------------
static void ioPOLYGON_ATTR(u32 adr,u32 data,u8) // 4a4
{
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"POLYGON_ATTR 0x%08X\n",data);
#endif
//   if(nBeginMode & 0x80000000)
//       return;
   bAlphaTest &= 0x7F;
   polyData = data;
   dataPoly = data;
}
//---------------------------------------------------------------------------
static void ioTEXPALETTE_BASE(u32 adr,u32 data,u8) // 4aC
{
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"TEXPALETTE_BASE 0x%08X\n",data);
#endif
	texPalette = (data & 0x1FFF);
   texData = dataTex;
}
//---------------------------------------------------------------------------
static void ioTEXIMAGE_PARAM(u32 adr,u32 data,u8) // 4a8
{
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"TEXIMAGE_PARAM 0x%08X\n",data);
#endif
   dataTex = texData = data;
}
//---------------------------------------------------------------------------
static void ioDIF_AMB(u32 adr,u32 data,u8 am) // 4c0
{
   color[1].set(data & 0x1F,(data >> 5) & 0x1F,(data >> 10) & 0x1F);
//   color[1].apply();
	if(data & 0x8000){
   	color[0].set(data & 0x1F,(data >> 5) & 0x1F,(data >> 10) & 0x1F);
   	color[0].apply();
   }
   else
       color[0] = color[6];
   color[2].set((data >> 16) & 0x1F,(data >> 21) & 0x1F,(data >> 26) & 0x1F);
//   color[2].apply();
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"DIF_AMB 0x%08X\n",data);
	comBuffer.add_Command(adr,data,am);
#endif
}
//---------------------------------------------------------------------------
static void ioSPE_EMI(u32 adr,u32 data,u8 am) // 4c4
{
	color[3].set(data & 0x1F,(data >> 5) & 0x1F,(data >>10) & 0x1F);
//   color[3].apply();
   color[4].set((data >> 16) & 0x1F,(data >> 21) & 0x1F,(data >> 26) & 0x1F);
//   color[4].apply();
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"SPE_EMI 0x%08X\n",data);
   comBuffer.add_Command(adr,data,am);
#endif
}
//---------------------------------------------------------------------------
static void ioLIGHT_VECTOR(u32 adr,u32 data,u8) // 4c8
{
   s32 x;
	float f[4];

   x = (s32)data;
   f[0] = (((x<<22) >> 22)) * -0.001953125f;
   f[1] = (((x<<12) >> 22)) * -0.001953125f;
   f[2] = (((x<<2) >> 22)) * -0.001953125f;
	f[3] = 0;
 	pfn_vet3_mtx4x4(f,mtxPosition);
	light[data >> 30].set_pos(f[0],f[1],f[2]);
#if defined(_DEBPRO)
	comBuffer.add_Command(adr,data,AMM_WORD);
	if(LINO)
   	fprintf(LINO,"LIGHT_VECTOR 0x%08X %1d %f %f %f\n",data,(int)(data >> 30),f[0],f[1],f[2]);
#endif
}
//---------------------------------------------------------------------------
static void ioLIGHT_COLOR(u32 adr,u32 data,u8) // 4cc
{
	light[data >> 30].set_color((data & 0x1F) * (1.0f / 31.0f),
       ((data >> 5) & 0x1F) * (1.0f / 31.0f),((data >> 10) & 0x1F) * (1.0f / 31.0f));
#if defined(_DEBPRO)
	comBuffer.add_Command(adr,data,AMM_WORD);
	if(LINO)
   	fprintf(LINO,"LIGHT_COLOR 0x%08X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioSHININESS(u32 adr,u32 data,u8) // 4d0
{
	if(idxShin == 124)
   	color[5].set((u8)data * 31,0,0);
   idxShin = (idxShin + 4) & 0x7F;
}
//---------------------------------------------------------------------------
static void ioBEGIN_VTXS(u32 adr,u32 data,u8) // 500
{
 	id_poly_internal++;
#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"BEGIN_VTXS %d %d\n",(int)data,(int)id_poly_internal);
#endif
	if(nBeginMode & 0x80000000)
       glEnd();
//   ulVertices = 0;
	switch(data & 3){
		case 0:
          	nBeginMode = GL_TRIANGLES;
       break;
		case 1:
          	nBeginMode = GL_QUADS;
       break;
		case 2:
          	nBeginMode = GL_TRIANGLE_STRIP;
       break;
		case 3:
          	nBeginMode = GL_QUAD_STRIP;
       break;
   }
   idxVtx = 0;
   if(polyData == (u32)-1 || polyData == 0){
#ifdef _DEBPRO
		comBuffer.add_Command(adr,data,AMM_WORD);
       comBuffer.add_Command(3,id_poly_internal,0);
#endif
       return;
	}
#ifdef _DEBPRO
   comBuffer.new_Object(polyData);
   comBuffer.add_Command(adr,data,AMM_WORD);
   comBuffer.add_Command(3,id_poly_internal,0);
#endif
   if(idxList){
       glEndList();
       BindTexture(crTex);
   }
   PolyData[10] = ((polyData >> 24) & 0x3F);//ID Poligono
   idxList = 0;
}
//---------------------------------------------------------------------------
static void ioEND_VTXS(u32 adr,u32 data,u8) // 504
{
	glEnd();
   nBeginMode &= 0xF;
   UpdatePolyRamCount();
   ulVertices = 0;
#if defined(_DEBPRO)
	comBuffer.add_Command(adr,data,AMM_WORD);
   if(LINO)
   	fprintf(LINO,"END_VTXS\n");
#endif
}
//---------------------------------------------------------------------------
static int qsortfnc(const void *a0, const void *a1)
{
   u8 a,b;

   a = (u8)((*(u64 *)a0 >> 32));
   b = (u8)((*(u64 *)a1 >> 32));
   return a - b;
}
//---------------------------------------------------------------------------
static void DeleteTranslucentPolygons()
{
   u32 i;
   u64 t;

   for(i=1;i<=transPoly.count();i++){
       t = transPoly[i];
       glDeleteLists((u32)(t & 0xFFFFF),20);
   }
   transPoly.free();
}
//---------------------------------------------------------------------------
static void DrawTranslucentPolygons(BOOL bSort)
{
   u32 i,i1,i2,i3;
   u64 t;

   if(!transPoly.count())
       return;
   if(bSort)
       qsort(transPoly.buffer(),transPoly.count(),sizeof(u64),qsortfnc);
   glPushAttrib(GL_ENABLE_BIT);
   for(i=1;i<=transPoly.count();i++){
       t = transPoly[i];
//       i1 = (u32)((t >> 48) & 0x1F);
       i2 = (u32)(t & 0xFFFFF);
       glCallList(i2);
       i1 = (u32)((t >> 20) & 0xFFF);
       for(i2++;i1 > 0;i1--,i2++)
           glCallList(i2);
   }
   glPopAttrib();
}
//---------------------------------------------------------------------------
static void ExecCommands(int mode)
{
   u64 uValue;
   u32 data,adr,i;
   u8 accessMode;
   LPINT_3DFUNC fnc;

   if(!(nOptions & NTR_USE_DIRECT_COMMAND)){
       if(mode && !bExecCommand){
           bExecCommand = TRUE;
           if(bControlChanged)
               ioDISP3DCNT(0,*((u16 *)&v3d.io_mem[0x60]),AMM_HWORD);
           for(i=1;i<=com.count();i++){
               uValue = com[i];
               data = (u32)uValue;
               adr = (u32)((uValue >> 32) & 0xFFFF);
               accessMode = (u8)((uValue >> 48) & 0xFF);
               fnc = &ioFunc[ioTable[adr]];
               if(mode == 1 || (mode == 2 && (fnc->flags & 1)))
                   fnc->pfn_1(adr,data,accessMode);
           }
           bExecCommand = FALSE;
       }
   }
   com.free();
}
//---------------------------------------------------------------------------
static void clear_buffer()
{
   u32 i,*out;

   cr_frame++;
   bUpdateOutBuffer = 0;
   if(!(bCImage & 1))
       i = GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT;
   else{
       int x,y,r;

       if(bCImage & 2){
           u16 *data;
           int a,color,x_o,y_o;

           x_o = (int)*((u16 *)&v3d.io_mem[0x356]);
           y_o = (x_o >> 8);
           x_o &= 0xFF;

           out = cimg_buffer;
           for(y=0;y<256;y++){
               u32 *out1;

               data = (u16 *)&(v3d.video_mem[128*2*1024]);
               data += (y << 8) + x_o;
               out1 = &out[(255 - y) << 8];
               for(x=0;x<(256 - x_o);x++){
                   color = *data++;
                   a = (color & 0x8000) ? 0xFF000000 : 0;
                   a |= (color & 31) << 3;
                   a |= (((color >> 5) & 31) << 3) << 8;
                   a |= (((color >> 10) & 31) << 3) << 16;
                   *out1++= a;
               }
               data = (u16 *)&(v3d.video_mem[128*2*1024]);
               data += (y << 8);
               for(;x<256;x++){
                   color = *data++;
                   a = (color & 0x8000) ? 0xFF000000 : 0;
                   a |= (color & 31) << 3;
                   a |= (((color >> 5) & 31) << 3) << 8;
                   a |= (((color >> 10) & 31) << 3) << 16;
                   *out1++= a;
               }
           }
           for(y=0;y<256;y++){
               u8 *out1;

               data = (u16 *)&(v3d.video_mem[128*3*1024]);
               data += (y << 8) + x_o;

               out1 = &cimgd_buffer[((255 - y) << 8)];
               for(x=0;x<(256 - x_o);x++){
                   int value;

                   color = *data++;
                   value = 127;
                   value -= ((color & 0x7fff) >> 7);

                   *out1++= (u8)value;
               }
               data = (u16 *)&(v3d.video_mem[128*3*1024]);
               data += (y << 8);
               for(;x<256;x++){
                   int value;

                   color = *data++;
                   value = 127;
                   value -= ((color & 0x7fff) >> 7);

                   *out1++= (u8)value;
               }
           }
           bCImage &= ~2;
       }
       {
           glGetIntegerv(GL_DEPTH_FUNC,(GLint *)&i);
           glGetIntegerv(GL_DEPTH_WRITEMASK,(GLint *)&r);
           glDepthFunc(GL_ALWAYS);
           glDepthMask(GL_TRUE);
           glDrawPixels(256,192,GL_DEPTH_COMPONENT,GL_BYTE,cimgd_buffer);
           glDepthMask(GL_FALSE);
           glDrawPixels(256,192,GL_RGBA,GL_UNSIGNED_BYTE,cimg_buffer);
           glDepthFunc(i);
           glDepthMask(r);
       }
       i = 0;//GL_DEPTH_BUFFER_BIT;
   }
   i |= GL_STENCIL_BUFFER_BIT;
   glClear(i);
}
//---------------------------------------------------------------------------
static void ReadBuffer3D(u32 nUpdateFrame)
{
   u32 i;

   if(!(u8)nUpdateFrame)
       bSwap |= 0x80;
   if(!(bSwap & 1))
       return;
   if((bSwap & 0x80) || (nOptions & NTR_SYNC_SWAP_BUFFER))
       nUpdateFrame &= 0xFFFF0000;
   if(hThread == NULL){
       flush_render_buffer(nUpdateFrame);
       render_buffer(nUpdateFrame);
   }
   else{
       WaitForSingleObject(hMutex,INFINITE);
       frame_mode = nUpdateFrame;
       command |= 1;
       SetEvent(hEvents[0]);
       ReleaseMutex(hMutex);
       WaitForSingleObject(hEvents[1],INFINITE);
   }
#ifdef __BORLANDC__
   _clear87();
   i = _control87(0x8001F, 0x8001F);
#else
   __asm__ __volatile__ (
   	"fclex\n"
   	"fstcw %0\n"
   	"movl $0x8001F,%%ecx\n"
   	"not %%ecx\n"
   	"movl %0,%%eax\n"
   	"andl %%eax,%%ecx\n"
   	"orl $0x8001F,%%ecx\n"
   	"movl %%ecx,%0\n"
   	"fldcw %0\n"
   	: : "m" (i)
   );
#endif
   if(ds.hasSSE())
       clearsse();
   if(hThread == NULL)
       clear_buffer();
   bSwap &= (u8)~0xC1;
}
//---------------------------------------------------------------------------
static void ioSWAP_BUFFERS_MC(u32 adr,u32 data,u8) // 540
{
   bOldSwap = bSwap;
   bSwap = (u8)((bSwap & 0xC0)|((data & 3) << 1)|1);
   GXSTAT |= (1 << 27);
}
//---------------------------------------------------------------------------
static void ioSWAP_BUFFERS(u32 adr,u32 data,u8) // 540
{
//   BOOL flag;     //5a432f6c
//   u64 p,p1;
//   DWORD i,i1,i2;
   id_poly_internal = 0;
	if(nBeginMode & 0x80000000)
       glEnd();
   if(idxList != 0)
       glEndList();
   idxList = 0;
	if(!(data & 2) || (nOptions & NTR_DEPTH_TEST_OPTION))
		glEnable(GL_DEPTH_TEST);
   else
		glDisable(GL_DEPTH_TEST);
   if((nOptions & NTR_USE_DIRECT_COMMAND)){
       DrawTranslucentPolygons(!(data & 1));
       DeleteTranslucentPolygons();
   }
   if((data & 3) != ((bSwap >> 1) & 3)){
       ResetTextureBuffer(TRUE);
       bFogChanged |= 7;
   }
   bSwap = (u8)((bSwap & 0xC0)|((data & 3) << 1)|1);
   bOldSwap = bSwap;   
   GXSTAT |= (1 << 27);
#if defined(_DEBPRO)
   comBuffer.swap();
   if(LINO)
   	fprintf(LINO,"SWAP_BUFFERS 0x%04X 0x%03X 0x%02X\n",data,VCOUNT,ds.Frames());
#endif
}
//---------------------------------------------------------------------------
static void InsertToBuffer(u32 adr,u32 data,u8 accessMode)
{
   LPINT_3DFUNC p;

   p = &ioFunc[ioTable[adr]];
	if(nOptions & NTR_USE_DIRECT_COMMAND)
		p->pfn_1(adr,data,accessMode);
   else{
       com.push((u64)((u64)(adr |(accessMode << 16)) << 32)|data);
       if(p->flags & 2)
           p->pfn_0(adr,data,accessMode);
   }
   if(accessMode != AMM_FIFO || adr < 0x400 || ++p->count < p->param)
       return;
   p->count = 0;                 
   gx_fifo.Func = NULL;
}
//---------------------------------------------------------------------------
static void ioVIEWPORT(u32 adr,u32 data,u8) // 580
{
	int x,y,width,height;

	y = (int)(u8)(data >> 8);
   x = (int)(u8)data;
   width = (int)(u8)(data >> 16) + 1;
   height = (int)(u8)(data >> 24) + 1;
   width -= x;
   height -= y;
   glViewport(x,y,width,height);
}
//---------------------------------------------------------------------------
static u32 gxfifostatus()
{
   u32 st;

   st = 0;
   if(gx_fifo.Counter > 255)
       st |= 0x01000000;
   else if(gx_fifo.Counter < 128)
       st |= 0x02000000;
   if(gx_fifo.Counter == 0)
       st |= 0x04000000;
//   else
//       st |= 0x08000000;
   return st;
}
//---------------------------------------------------------------------------
static void ceck_gxfifoirq(u32 status)
{
   if((GXSTAT & 0x40000000) && (status & 0x02000000))
       v3d.pfn_onirq();
   if((GXSTAT & 0x80000000) && (status & 0x04000000))
       v3d.pfn_onirq();
}
//---------------------------------------------------------------------------
static void ioGXFIFO(u32 adr,u32 data,u8 accessMode) // 400
{
   LPINT_3DFUNC p;
   u32 status;

#ifdef _DEBPRO
   p = NULL;
#endif
ioGXFIFO_1:
	if(gx_fifo.Func == NULL && gx_fifo.Index <= 0){
   	if(!(gx_fifo.Reg = data))
       	return;
#ifdef _DEBPRO
		if(LINO1 != NULL)
       	fprintf(LINO1,"%08X\n",data);
       lLino++;
#endif
		for(gx_fifo.Index = 0;data != 0;gx_fifo.Index++){
           p = &ioFunc[ioTable[0x400 + ((u8)data << 2)]];
           if((u8)data > 0x72){
				gx_fifo.Reg >>= 8;
               gx_fifo.Index--;
			}
           else
           	gx_fifo.Cycles += p->cycles;
			data >>= 8;
           gx_fifo.Counter++;
       }
       status = gxfifostatus();
//       ceck_gxfifoirq(status);
   	GXSTAT &= ~0x7FF0000;
   	GXSTAT |= ((u8)gx_fifo.Counter << 16)|status;
#ifdef _DEBPRO
       p = (LPINT_3DFUNC)1;
#endif
	}
#ifdef _DEBPRO
	if(p == NULL){
       if(LINO1 != NULL)
           fprintf(LINO1,"\t%08X\n",data);
   }
#endif
   if(gx_fifo.Func != NULL)
  		gx_fifo.Func(gx_fifo.CurrentReg,data,AMM_FIFO);
   if(gx_fifo.Func != NULL)
       return;
   gx_fifo.CurrentReg = 0x400 + ((u8)gx_fifo.Reg << 2);
   gx_fifo.Reg >>= 8;
   gx_fifo.Index--;
  	if(gx_fifo.Index >= 0){
   	if(gx_fifo.CurrentReg < 0x440){
           data = data;
   		goto ioGXFIFO_1;
       }
       p = &ioFunc[ioTable[gx_fifo.CurrentReg]];
   	gx_fifo.Func = InsertToBuffer;
       if(p->param < 1){
       	data = data;
       	goto ioGXFIFO_1;
       }
   }
}
//---------------------------------------------------------------------------
static void ioGXSTAT(u32 adr,u32 data,u8) // 600
{
	if(data & 0x8000){
       if(GXSTAT & 0x8000){
           sMV.set_index(0);
           sMP.set_index(0);
//           sPM.set_index(0);
           GXSTAT &= ~0x8000;
       }
   }
	if(data & 0x4000)
   	GXSTAT &= ~0x4000;
   GXSTAT &= ~0x7FF0000;
   GXSTAT |= ((u8)gx_fifo.Counter << 16)|gxfifostatus();
}
//---------------------------------------------------------------------------
static int boxtest_point(float planeEqs[][4],float *clip)
{
   float box[8][4],hw,hh,hd;
   int i,j,tot;

   hw = bbox[3] * .5f;hh = bbox[4] * .5f;hd = bbox[5] * .5f;

   box[0][0] = bbox[0] - hw;
   box[0][1] = bbox[1] - hh;
   box[0][2] = bbox[2] - hd;
   box[0][3] = 1;

   box[1][0] = bbox[0] - hw;
   box[1][1] = bbox[1] - hh;
   box[1][2] = bbox[2] + hd;
   box[1][3] = 1;

   box[2][0] = bbox[0] - hw;
   box[2][1] = bbox[1] + hh;
   box[2][2] = bbox[2] - hd;
   box[2][3] = 1;

   box[3][0] = bbox[0] - hw;
   box[3][1] = bbox[1] + hh;
   box[3][2] = bbox[2] + hd;
   box[3][3] = 1;
   box[4][0] = bbox[0] + hw;
   box[4][1] = bbox[1] - hh;
   box[4][2] = bbox[2] - hd;
   box[4][3] = 1;
   box[5][0] = bbox[0] + hw;
   box[5][1] = bbox[1] + hh;
   box[5][2] = bbox[2] - hd;
   box[5][3] = 1;
   box[6][0] = bbox[0] + hw;
   box[6][1] = bbox[1] - hh;
   box[6][2] = bbox[2] + hd;
   box[6][3] = 1;
   box[7][0] = bbox[0] + hw;
   box[7][1] = bbox[1] + hh;
   box[7][2] = bbox[2] + hd;
   box[7][3] = 1;

   for(i=0;i<8;i++){
       pfn_vet4_mtx4x4(box[i],clip);
       if(box[i][3]){
           box[i][0] /= box[i][3];
           box[i][1] /= box[i][3];
           box[i][2] /= box[i][3];
       }
   }
   tot = 0;
   for(i=0;i<6;i++){
       for(j=0;j<8;j++){
           if((planeEqs[i][0]*box[j][0] + planeEqs[i][1]*box[j][1] + planeEqs[i][2]*box[j][2] + planeEqs[i][3]) <= 0)
               tot++;
       }
   }
   return !(tot >= 23);
}
//---------------------------------------------------------------------------
static void ioBOXTEST(u32 adr,u32 data,u8 accessMode)
{
   float planeEqs[6][4];
   int i;
   
   i = (int)(s16)data;
   bbox[idxBoxTest++] = i * .0002441406f;
   i = (int)(s16)(data>>16);
   bbox[idxBoxTest++] = i * .0002441406f;
   if(idxBoxTest < 6)
       return;
   idxBoxTest = 0;
   EXEC_COMMANDS(TRUE)

#if defined(_DEBPRO)
	if(LINO)
   	fprintf(LINO,"BOX_TEST\n");
#endif

   if(bChangeMatrix & 3)
       UpdateClipMatrix();
   planeEqs[0][0] = mtxClip[3] - mtxClip[0];//right
   planeEqs[0][1] = mtxClip[7] - mtxClip[4];
   planeEqs[0][2] = mtxClip[11] - mtxClip[8];
   planeEqs[0][3] = mtxClip[15] - mtxClip[12];

   planeEqs[1][0] = mtxClip[3] + mtxClip[0];//left
   planeEqs[1][1] = mtxClip[7] + mtxClip[4];
   planeEqs[1][2] = mtxClip[11] + mtxClip[8];
   planeEqs[1][3] = mtxClip[15] + mtxClip[12];

   planeEqs[2][0] = mtxClip[3] + mtxClip[1];//bottom
   planeEqs[2][1] = mtxClip[7] + mtxClip[5];
   planeEqs[2][2] = mtxClip[11] + mtxClip[9];
   planeEqs[2][3] = mtxClip[15] + mtxClip[13];

   planeEqs[3][0] = mtxClip[3] - mtxClip[1];//top
   planeEqs[3][1] = mtxClip[7] - mtxClip[5];
   planeEqs[3][2] = mtxClip[11] - mtxClip[9];
   planeEqs[3][3] = mtxClip[15] - mtxClip[13];

   planeEqs[4][0] = mtxClip[3] + mtxClip[2];//front
   planeEqs[4][1] = mtxClip[7] + mtxClip[6];
   planeEqs[4][2] = mtxClip[11] + mtxClip[10];
   planeEqs[4][3] = mtxClip[15] + mtxClip[14];

   planeEqs[5][0] = mtxClip[3] - mtxClip[2];//back
   planeEqs[5][1] = mtxClip[7] - mtxClip[6];
   planeEqs[5][2] = mtxClip[11] - mtxClip[10];
   planeEqs[5][3] = mtxClip[15] - mtxClip[14];

   GXSTAT &= ~3;
   if(boxtest_point((float (*)[4])planeEqs,mtxClip))
       GXSTAT |= 2;
}
//---------------------------------------------------------------------------
static void ioFOG_OFFSET(u32 adr,u32 data,u8 am)
{
   bFogChanged |= 1;
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"FOG_OFFSET %08X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void ioFOG_TABLE(u32 adr,u32 data,u8 am)
{
   bFogChanged |= 9;
   adr -= 0x360;
   do{
       fogDensity[adr] = (u8)(data & 0x7F);
       adr++;
       data >>= 8;
       am >>= 1;
   }while(am);
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"FOG_TABLE %08X\n",data);
#endif
}
//---------------------------------------------------------------------------
static void io3DNULL(u32 adr,u32 data,u8)
{
#if defined(_DEBPRO)
   if(LINO)
   	fprintf(LINO,"%08X, %08X\n",adr,data);
#endif
}
//---------------------------------------------------------------------------
static void Render3D(int x)
{
   u32 status;

/*   if(gx_fifo.Counter == 0){
       GXSTAT &= ~0x7FF0000;
       GXSTAT |= ((u8)gx_fifo.Counter << 16)|(status = gxfifostatus());
   	return;
   }*/
  	gx_fifo.Cycles = 0;
	x /= gx_fifo.div;
  	if(gx_fifo.Counter < x)
  		gx_fifo.Counter = 0;
  	else
  		gx_fifo.Counter -= x;
   GXSTAT &= ~0x7FF0000;
   GXSTAT |= ((u8)gx_fifo.Counter << 16)|(status = gxfifostatus());
   ceck_gxfifoirq(status);
}
//---------------------------------------------------------------------------
static u32 ioCLIPMTX_RESULT(u32 adr,u8 am)
{
   EXEC_COMMANDS(TRUE)
   if(bChangeMatrix & 3)
       UpdateClipMatrix();
   return ((u32 *)&v3d.io_mem[adr])[0];
}
//---------------------------------------------------------------------------
static u32 ioDIRMTX_RESULT(u32 adr,u8 am)
{
	if(adr == 0x680){
       EXEC_COMMANDS(TRUE)
       ((u32 *)&v3d.io_mem[0x680])[0] = (u32)(mtxPosition[0] * 4096.0f);
       ((u32 *)&v3d.io_mem[0x680])[1] = (u32)(mtxPosition[1] * 4096.0f);
       ((u32 *)&v3d.io_mem[0x680])[2] = (u32)(mtxPosition[2] * 4096.0f);
       ((u32 *)&v3d.io_mem[0x680])[3] = (u32)(mtxPosition[4] * 4096.0f);
       ((u32 *)&v3d.io_mem[0x680])[4] = (u32)(mtxPosition[5] * 4096.0f);
       ((u32 *)&v3d.io_mem[0x680])[5] = (u32)(mtxPosition[6] * 4096.0f);
       ((u32 *)&v3d.io_mem[0x680])[6] = (u32)(mtxPosition[8] * 4096.0f);
       ((u32 *)&v3d.io_mem[0x680])[7] = (u32)(mtxPosition[9] * 4096.0f);
       ((u32 *)&v3d.io_mem[0x680])[8] = (u32)(mtxPosition[10] * 4096.0f);
   }
   return ((u32 *)&v3d.io_mem[adr])[0];
}
//---------------------------------------------------------------------------
static void ioPOSTEST(u32 adr,u32 data,u8 accessMode)
{
   s32 x,y;
   float v[4],mtx[16];

   for(y = 0;y < 2 && idxPosTest < 3;y++){
       x = (s32)(s16)data;
       data >>= 16;
       pos_test[idxPosTest++] = x * .0002441406f;
   }
   if(idxPosTest != 3)
       return;
   idxPosTest = 0;       
   EXEC_COMMANDS(TRUE)
   if(bChangeMatrix & 3)
       UpdateClipMatrix();
   v[0] = pos_test[0];
   v[1] = pos_test[1];
   v[2] = pos_test[2];
   v[3] = 1;
   pfn_vet4_mtx4x4(v,mtx);
   *((u32 *)(v3d.io_mem + 0x620)) = (u32)(v[0] * 4096.0f);
   *((u32 *)(v3d.io_mem + 0x624)) = (u32)(v[1] * 4096.0f);
   *((u32 *)(v3d.io_mem + 0x628)) = (u32)(v[2] * 4096.0f);
   *((u32 *)(v3d.io_mem + 0x62C)) = (u32)(v[3] * 4096.0f);
   GXSTAT &= ~1;
}
//---------------------------------------------------------------------------
static void ioVECTEST(u32 adr,u32 data,u8 accessMode)
{
	s32 x;
   float v[4];

   EXEC_COMMANDS(TRUE)
	x = (data & 0x3FF);
   if(x & 0x200)
   	x |= 0xFFFFFC00;
   v[0] = x * 0.001953125f;
	x = ((data >> 10) & 0x3FF);
   if(x & 0x200)
   	x |= 0xFFFFFC00;
   v[1] = x * 0.001953125f;
	x = ((data >> 20) & 0x3FF);
   if(x & 0x200)
   	x |= 0xFFFFFC00;
   v[2] = x * 0.001953125f;
   v[3] = 1;
   pfn_vet3_mtx4x4(v,mtxPosition);
   ((u32 *)(v3d.io_mem + 0x630))[0] = (u32)(v[0] * 4096.0f);
   ((u32 *)(v3d.io_mem + 0x630))[1] = (u32)(v[1] * 4096.0f);
   ((u32 *)(v3d.io_mem + 0x630))[2] = (u32)(v[2] * 4096.0f);
   GXSTAT &= ~1;
}
//---------------------------------------------------------------------------
static u32 ioPOLYRAM_RESULT(u32 adr,u8 am)
{
   EXEC_COMMANDS(TRUE)
   return *((u16 *)&v3d.io_mem[0x604]);
}
//---------------------------------------------------------------------------
static u32 ioVTXRAM_RESULT(u32 adr,u8 am)
{
   EXEC_COMMANDS(TRUE)
   return *((u16 *)&v3d.io_mem[0x606]);
}
//---------------------------------------------------------------------------
static void EndPipelineExec()
{
	if(!(GXSTAT & (1<<27)))
       return;
   GXSTAT &= ~(1 << 27);
   LISTRAM_COUNT = 0;
   VTXRAM_COUNT = 0;
}
//---------------------------------------------------------------------------
static void Init3DTable(/*LPIFUNC *p,LPOFUNC *p1*/)
{
   int i,i1,*p2;

   for(i=0x60;i<0x62;i++)
       v3d.pfn_writetable(0x04000000+i,(void *)InsertToBuffer,NULL);
 	for(i=0x320;i<0x6f0;i++)
   	v3d.pfn_writetable(0x04000000+i,(void *)InsertToBuffer,NULL);
 	for(i=0x540;i<0x544;i++)
       v3d.pfn_writetable(0x04000000+i,(void *)InsertToBuffer,NULL);
   for(i=0x400;i<0x440;i++)
       v3d.pfn_writetable(0x04000000+i,(void *)ioGXFIFO,NULL);
 	for(i=0x600;i<0x604;i++)
       v3d.pfn_writetable(0x04000000+i,(void *)ioGXSTAT,NULL);

 	for(i=0x640;i<0x680;i++)
       v3d.pfn_writetable(0x04000000+i,NULL,(void *)ioCLIPMTX_RESULT);
 	for(;i<0x6A4;i++)
       v3d.pfn_writetable(0x04000000+i,NULL,(void *)ioDIRMTX_RESULT);

 	for(i=0x604;i<0x606;i++)
       v3d.pfn_writetable(0x04000000+i,NULL,(void *)ioPOLYRAM_RESULT);
 	for(i=0x606;i<0x608;i++)
       v3d.pfn_writetable(0x04000000+i,NULL,(void *)ioVTXRAM_RESULT);

/*   for(i=0x5c0;i<0x5c4;i++)
       p[i] = ioBOXTEST;
   for(;i<0x5c8;i++)
       p[i] = ioPOSTEST;*/
 	for(i=0;i<128;i++){
   	ioFunc[i].pfn_1 = io3DNULL;
       ioFunc[i].pfn_0 = NULL;
		ioFunc[i].count = 0;
       ioFunc[i].param = 1;
       ioFunc[i].cycles = 0;
       ioFunc[i].flags = 0;
   }
   for(i=0;i<2048;i++)
       ioTable[i] = 0;
	p2 = ioTable;
	i1 = 1;
   p2[0x60] = p2[0x61] = i1;
   ioFunc[i1].flags = 2;
	ioFunc[i1].pfn_0 = ioDISP3DCNT_MC;
	ioFunc[i1++].pfn_1 = ioDISP3DCNT;
   for(i=0x330;i<0x340;i++)
   	p2[i] = i1;
  	ioFunc[i1++].pfn_1 = ioEDGE_COLOR;
   p2[0x340] = p2[0x341] = i1;
	ioFunc[i1++].pfn_1 = ioALPHA_TEST_REF;
 	for(i=0x350;i<0x354;i++)
   	p2[i] = i1;
  	ioFunc[i1++].pfn_1 = ioCLEAR_COLOR;
   for(;i<0x356;i++)
 		p2[i] = i1;
	ioFunc[i1++].pfn_1 = ioCLEAR_DEPTH;
   for(;i<0x358;i++)
 		p2[i] = i1;
	ioFunc[i1++].pfn_1 = ioCLEAR_IMAGE;
 	for(i=0x358;i<0x35c;i++)
 		p2[i] = i1;
	ioFunc[i1++].pfn_1 = ioFOG_COLOR;
   for(;i<0x360;i++)
 		p2[i] = i1;
	ioFunc[i1++].pfn_1 = ioFOG_OFFSET;
 	for(;i<0x380;i++)
 		p2[i] = i1;
	ioFunc[i1++].pfn_1 = ioFOG_TABLE;
 	for(i=0x380;i<0x3c0;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 11;
	ioFunc[i1++].pfn_1 = ioTOON_TABLE;
   for(i=0x400;i<0x440;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 1;
  	ioFunc[i1++].pfn_1 = ioGXFIFO;
 	for(i=0x440;i<0x444;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 1;
//   ioFunc[i1].flush = 1;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_MODE;
 	for(;i<0x448;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 0;
   ioFunc[i1].cycles = 17;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_PUSH;
 	for(;i<0x44c;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 36;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_POP;
	for(;i<0x450;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 17;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_STORE;
	for(;i<0x454;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 36;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_RESTORE;
 	for(;i<0x458;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 0;
   ioFunc[i1].cycles = 19;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_IDENTITY;
 	for(;i<0x45c;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 16;
   ioFunc[i1].cycles = 34;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_LOAD_4X4;
 	for(;i<0x460;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 12;
   ioFunc[i1].cycles = 30;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_LOAD_4X3;
 	for(;i<0x464;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 16;
   ioFunc[i1].cycles = 35;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_MULT_4X4;
 	for(;i<0x468;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 12;
   ioFunc[i1].cycles = 31;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_MULT_4X3;
 	for(;i<0x46c;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 9;
   ioFunc[i1].cycles = 28;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_MULT_3X3;
 	for(;i<0x470;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 3;
   ioFunc[i1].cycles = 22;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_SCALE;
 	for(;i<0x474;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 3;
   ioFunc[i1].cycles = 22;
   ioFunc[i1].flags = 1;
	ioFunc[i1++].pfn_1 = ioMTX_TRANS;
 	for(i=0x480;i<0x482;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 1;
	ioFunc[i1++].pfn_1 = ioCOLOR;
 	for(i=0x484;i<0x488;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 12;
	ioFunc[i1++].pfn_1 = ioNORMAL;
 	for(;i<0x48c;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 1;
	ioFunc[i1++].pfn_1 = ioTEXCOORD;
 	for(;i<0x490;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 2;
   ioFunc[i1].cycles = 9;
	ioFunc[i1++].pfn_1 = ioVTX_16;
 	for(;i<0x494;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 8;
	ioFunc[i1++].pfn_1 = ioVTX_10;
 	for(;i<0x498;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 8;
	ioFunc[i1++].pfn_1 = ioVTX_XY;
	for(;i<0x49c;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 8;
	ioFunc[i1++].pfn_1 = ioVTX_XZ;
	for(;i<0x4A0;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 8;
	ioFunc[i1++].pfn_1 = ioVTX_YZ;
	for(;i<0x4a4;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 8;
  	ioFunc[i1++].pfn_1 = ioVTX_DIFF;
 	for(;i<0x4a8;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 1;
  	ioFunc[i1++].pfn_1 = ioPOLYGON_ATTR;
 	for(;i<0x4ac;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 1;
	ioFunc[i1++].pfn_1 = ioTEXIMAGE_PARAM;
   for(;i<0x4b0;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 1;
  	ioFunc[i1++].pfn_1 = ioTEXPALETTE_BASE;
 	for(i=0x4c0;i<0x4c4;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 4;
	ioFunc[i1++].pfn_1 = ioDIF_AMB;
 	for(;i<0x4c8;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 4;
	ioFunc[i1++].pfn_1 = ioSPE_EMI;
 	for(;i<0x4cc;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 6;
	ioFunc[i1++].pfn_1 = ioLIGHT_VECTOR;
 	for(;i<0x4d0;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 1;
	ioFunc[i1++].pfn_1 = ioLIGHT_COLOR;
 	for(;i<0x4d4;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 32;
   ioFunc[i1].cycles = 32;
	ioFunc[i1++].pfn_1 = ioSHININESS;
 	for(i=0x500;i<0x504;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 1;
	ioFunc[i1++].pfn_1 = ioBEGIN_VTXS;
 	for(;i<0x508;i++)
 		p2[i] = i1;
   ioFunc[i1].param = 0;
   ioFunc[i1].cycles = 1;
	ioFunc[i1++].pfn_1 = ioEND_VTXS;
 	for(i=0x540;i<0x544;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 392;
   ioFunc[i1].flags = 2;
	ioFunc[i1].pfn_0 = ioSWAP_BUFFERS_MC;
	ioFunc[i1++].pfn_1 = ioSWAP_BUFFERS;
 	for(i=0x580;i<0x584;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 1;
	ioFunc[i1++].pfn_1 = ioVIEWPORT;

   ioFunc[i1].param = 3;
   for(i=0x5c0;i<0x5c4;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 103;
   ioFunc[i1].flags = 2;
   ioFunc[i1].pfn_0 = ioBOXTEST;
	ioFunc[i1++].pfn_1 = ioBOXTEST;

   ioFunc[i1].param = 2;
   for(;i<0x5c8;i++)
 		p2[i] = i1;
   ioFunc[i1].cycles = 9;
   ioFunc[i1].flags = 2;
   ioFunc[i1].pfn_0 = ioPOSTEST;
	ioFunc[i1++].pfn_1 = ioPOSTEST;

   ioFunc[i1].param = 1;
   for(;i<0x5cc;i++)
       p2[i] = i1;
   ioFunc[i1].cycles = 5;
   ioFunc[i1].flags = 2;
   ioFunc[i1].pfn_0 = ioVECTEST;
	ioFunc[i1++].pfn_1 = ioVECTEST;

 	for(i=0x610;i<0x614;i++)
 		p2[i] = i1;
	ioFunc[i1++].pfn_1 = ioCUTOFFDEPTH;
}
#ifdef _DEBPRO
//---------------------------------------------------------------------------
static LPTEXTURE get_TextureInfo(int index)
{
   if(index >= TEXTURE_ALLOCATED || (index >= idxTxt && index != TEXTURE_ALLOCATED-1))
       return NULL;
  	return &tex[index];
}
#endif
//---------------------------------------------------------------------------
static void *get_Stack(int stack,int index)
{
   switch(stack & 3){
       case 0:
           if(index == 0)
               return mtxProjection;
           return &sPM;
       break;
       case 1:
           if(index == 0)
               return mtxView;
           return &sMV;
       break;
       case 2:
           if(index == 0)
               return mtxPosition;
           return &sMV;
       break;
       case 3:
           return mtxTexture;
       break;
   }
}

//---------------------------------------------------------------------------
LPolygon::LPolygon()
{
   bEnable = FALSE;
}
//---------------------------------------------------------------------------
LPolygon::~LPolygon()
{
}
//---------------------------------------------------------------------------
BOOL LPolygon::Render()
{
   u32 i,index;
   u8 alpha;

   if(!bEnable || bufferCommand.count() == 0)
       goto Ex_Render;
   i = ((data >> 24) & 0x3F);
   if(i == clearID)
       goto Ex_Render;
   i >>= 3;
   alpha = (u8)((data >> 16) & 0x1F);
   glEnd();
   glEnable(GL_DEPTH_TEST);
//   glColor4f(edgeColor[i] / 248.0f,edgeColor[i+1] / 248.0f,edgeColor[i+1] / 248.0f,alpha / 31.0f);
	glPolygonMode(GL_BACK,GL_LINE);
	glLineWidth(2);
   glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_LEQUAL);
   glDisable(GL_LIGHTING);
   for(i=1;i<=bufferCommand.count();i++){
       index = bufferCommand[i];
       switch(index >> 24){
           case vertex:
               index = (index & 0xFFFFFF) << 2;
               glVertex4f(vertexs[index+1],vertexs[index+2],vertexs[index+3],vertexs[index+4]);
           break;
           case begin:
               glEnd();
               glBegin(GL_TRIANGLES/*begins[index & 0xFFFFFF]*/);
           break;
           case end:
               glEnd();
           break;
       }
   }
   glEnd();
   color[0].apply();
   if(!alpha)
       glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
   else
       glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
   if((data & 0xC0) == 0xC0)
   	glDisable(GL_CULL_FACE);
   else{
   	glEnable(GL_CULL_FACE);
       glCullFace(((data & 0x40) ? GL_FRONT : (data & 0x80) ? GL_BACK : GL_FRONT_AND_BACK));
   }
   if((data & 0x4000))
		glDepthFunc(GL_EQUAL);
   else
		glDepthFunc(GL_LESS);
   if((data & 0xF))
   	glEnable(GL_LIGHTING);
   else
     	glDisable(GL_LIGHTING);
   if((data & 1))
      	glEnable(GL_LIGHT0);
   else
      	glDisable(GL_LIGHT0);
	if((data & 2))
   	glEnable(GL_LIGHT1);
   else
      	glDisable(GL_LIGHT1);
	if((data & 4))
   	glEnable(GL_LIGHT2);
   else
      	glDisable(GL_LIGHT2);
	if((data & 8))
   	glEnable(GL_LIGHT3);
   else
     	glDisable(GL_LIGHT3);
   if(!(nOptions & 0x100))
       glDisable(GL_DEPTH_TEST);
Ex_Render:
   vertexs.free();
   begins.free();
   bufferCommand.free();
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LPolygon::add_Command(cmdType type,void *value)
{
   u32 index;

   if(!bEnable)
       return TRUE;
   switch(type){
       case vertex:
           index = vertexs.count() >> 2;
           vertexs.push(((float *)value)[0]);
           vertexs.push(((float *)value)[1]);
           vertexs.push(((float *)value)[2]);
           vertexs.push(((float *)value)[3]);
       break;
       case begin:
           index = begins.count();
           begins.push(((GLenum *)value)[0]);
       break;
       case end:
           index = 0;
       break;
   }
   bufferCommand.push((index & 0xFFFFFF)|(type << 24));
   return TRUE;
}
//---------------------------------------------------------------------------
static DWORD WINAPI ThreadFunc_01(LPVOID arg)
{
   HANDLE h[2];

   SetThreadIdealProcessor(GetCurrentThread(),((1 << dwCores) -1));
//   SetThreadAffinityMask(GetCurrentThread(),((1 << dwCores) -1));
   h[0] = hMutex;
   h[1] = hEvents[0];
   while(!(command & 0x80000000)){
       WaitForMultipleObjects(2,h,TRUE,INFINITE);
       if(command & 16){
           unsigned long i;

           glEnd();
           glEndList();
           for(i=1;i<=transPoly.count();i++)
               glDeleteLists((GLuint)(transPoly[i] & 0xFFFFF),20);
           transPoly.clear();
           ResetTextureBuffer(TRUE);
           EnableShaderExtension(FALSE);
           EnableFrameBufferExtension(FALSE);
       }
       if(command & 8)
           set_Options(command_value);
       if(command & 4)
#ifdef __WIN32__
           wglMakeCurrent(v3d.lpWnd->DC(),hRC);
#else
            glXMakeCurrent(g_pDisplay,GDK_DRAWABLE_XID(v3d.lpWnd->Handle()->window),hRC);
#endif
       if(command & 2)
           ExecCommands(TRUE);
       if(command & 0x1E){
           command &= ~0x1E;
           SetEvent(hEvents[3]);
       }
       if(command & 1){
           flush_render_buffer(frame_mode);
           SetEvent(hEvents[1]);
           render_buffer(frame_mode);
           clear_buffer();
           SetEvent(hEvents[2]);
           command &= ~1;
       }
       ReleaseMutex(h[0]);
   }
#ifdef __WIN32__
   wglMakeCurrent(NULL,NULL);
#else
    glXMakeCurrent(g_pDisplay,NULL,NULL);
#endif
   return 1;
}
//---------------------------------------------------------------------------
static BOOL EnableMultiCores(BOOL bEnable)
{
   int i;

#ifndef __WIN32__
    bEnable = FALSE;
#endif
   if(dwCores < 2)
       bEnable = FALSE;
   if(!bEnable){
       command = 0x80000000;
       if(hThread != NULL){
           if(hEvents[0] != NULL)
               SetEvent(hEvents[0]);
           if(hMutex != NULL)
               ReleaseMutex(hMutex);
           WaitForSingleObject(hThread,INFINITE);
           CloseHandle(hThread);
           hThread = NULL;
       }
       for(i=0;i<sizeof(hEvents)/sizeof(HANDLE);i++){
           if(hEvents[i] != NULL){
               CloseHandle(hEvents[i]);
               hEvents[i] = NULL;
           }
       }
       if(hMutex != NULL){
           CloseHandle(hMutex);
           hMutex = NULL;
       }
	   if(hRC != NULL && v3d.lpWnd != NULL)
#ifdef __WIN32__
			wglMakeCurrent(v3d.lpWnd->DC(),hRC);
#else        
            glXMakeCurrent(g_pDisplay,GDK_DRAWABLE_XID(v3d.lpWnd->Handle()->window),hRC);
#endif
   }
   else if(hThread == NULL){
       DWORD dw;
       LString s;

       for(i=0;i<sizeof(hEvents)/sizeof(HANDLE);i++){
           s = "OpenGLPlugin_Event_";
           s += i;
           hEvents[i] = CreateEvent(NULL,FALSE,FALSE,s.c_str());
           if(hEvents[i] == NULL){
               EnableMultiCores(FALSE);
               return FALSE;
           }
       }
       hMutex = CreateMutex(NULL,FALSE,"OpenGLPlugin_Mutex_0");
       if(hMutex == NULL){
           EnableMultiCores(FALSE);
           return FALSE;
       }
       command = 4;
       hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadFunc_01,(LPVOID)NULL,CREATE_SUSPENDED,&dw);
       if(hThread == NULL){
           EnableMultiCores(FALSE);
           return FALSE;
       }
       frame_mode = (DWORD)-1;
#ifdef __WIN32__
       wglMakeCurrent(NULL,NULL);
#else
        glXMakeCurrent(g_pDisplay,NULL,NULL);
#endif
       ResumeThread(hThread);
       SetEvent(hEvents[0]);
   }
   return TRUE;
}

//---------------------------------------------------------------------------
OpenGLPlugIn::OpenGLPlugIn()
{
   guid = OPENGL_PLUGIN;
   name = "OpenGL 3D";
	dwType = PIT_VIDEO;
   dwFlags = PIT_IS3D;
	pfn_mtx4x4_mult = mtx4x4_mult;
   pfn_vet4_mtx4x4 = vet4_mtx4x4;
	pfn_vet3_mtx4x4 = vet3_mtx4x4;
   hThread = NULL;
   ZeroMemory(hEvents,sizeof(hEvents));
   command = 3;
}
//---------------------------------------------------------------------------
OpenGLPlugIn::~OpenGLPlugIn()
{
}
//---------------------------------------------------------------------------
BOOL OpenGLPlugIn::Enable(BOOL bFlag)
{
	if(!VideoPlug::Enable(bFlag))
   	return FALSE;
   if(!bFlag)
   	return TRUE;
   Create3DDevice(&v3d);
   if(!Init3DDevice(TRUE))
   	return FALSE;
   Init3DTable();
   return Reset();
}
//---------------------------------------------------------------------------
BOOL OpenGLPlugIn::Reset()
{
	OnChangeConfig();
	if(hRC != NULL)
		Reset3DDevice();
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL OpenGLPlugIn::Destroy()
{
   ::v3d.lpWnd = NULL;
   EnableMultiCores(FALSE);
	Destroy3DDevice();
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL OpenGLPlugIn::SaveState(LStream *p)
{
   u32 i;
   LPINT_3DFUNC f;

   p->Write(&texpalMem,sizeof(texpalMem));
   p->Write(&gx_fifo,sizeof(gx_fifo));
   if(gx_fifo.Func != NULL){
       f = &ioFunc[ioTable[gx_fifo.CurrentReg]];
       i = f->count;
   }
   else
       i = 0;
   p->Write(&i,sizeof(u32));
   sPM.Save(p);
   sMV.Save(p);
   sTX.Save(p);
   sMP.Save(p);
   for(i=0;i<4;i++)
   	light[i].Save(p);
   p->Write(mtxProjection,sizeof(mtxProjection));
   p->Write(mtxView,sizeof(mtxView));
   p->Write(mtxPosition,sizeof(mtxPosition));
   p->Write(mtxTexture,sizeof(mtxTexture));
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL OpenGLPlugIn::LoadState(LStream *p,int ver)
{
   u32 i;
   LPINT_3DFUNC f;

	Reset();
   p->Read(&texpalMem,sizeof(texpalMem));
   if(ver > 33){
		i = ver < 41 ? 24 : sizeof(gx_fifo);
   	p->Read(&gx_fifo,i);
   	p->Read(&i,sizeof(u32));
   	if(gx_fifo.Func != NULL){
       	gx_fifo.Func = InsertToBuffer;
       	f = &ioFunc[ioTable[gx_fifo.CurrentReg]];
       	f->count = (u8)i;
   	}
   	sPM.Load(p,0);
   	sMV.Load(p,0);
		if(ver > 39)
       	sTX.Load(p,0);
       if(ver > 43)
           sMP.Load(p,0);
       else
           sMP = sMV;
		if(ver > 33){
   		for(i=0;i<4;i++)
       		light[i].Load(p,ver);
   	}
   	if(ver > 34){
   		p->Read(mtxProjection,sizeof(mtxProjection));
   		p->Read(mtxView,sizeof(mtxView));
   		p->Read(mtxPosition,sizeof(mtxPosition));
   		p->Read(mtxTexture,sizeof(mtxTexture));
   	}
	}
   InsertToBuffer(0x60,*((u16 *)&v3d.io_mem[0x60]),AMM_HWORD);
   InsertToBuffer(0x350,*((u32 *)&v3d.io_mem[0x350]),AMM_WORD);
   InsertToBuffer(0x354,*((u16 *)&v3d.io_mem[0x354]),AMM_HWORD);
   InsertToBuffer(0x358,*((u32 *)&v3d.io_mem[0x358]),AMM_WORD);
   InsertToBuffer(0x480,*((u16 *)&v3d.io_mem[0x480]),AMM_HWORD);
   InsertToBuffer(0x4C0,*((u32 *)&v3d.io_mem[0x4C0]),AMM_WORD);
   InsertToBuffer(0x4C4,*((u32 *)&v3d.io_mem[0x4C4]),AMM_WORD);
   InsertToBuffer(0x580,*((u32 *)&v3d.io_mem[0x580]),AMM_WORD);
 	for(i=0x360;i<0x3C0;i+=2)
 		InsertToBuffer(i,*((u16 *)&v3d.io_mem[i]),AMM_HWORD);
   bChangeMatrix = 3;
   bFogChanged = 0xF;
   bToonChanged = TRUE;
	return TRUE;
}
//---------------------------------------------------------------------------
void OpenGLPlugIn::OnChangeConfig()
{
   if(ds.hasSSE()){
       pfn_mtx4x4_mult = mtx4x4_multsse;
       pfn_vet4_mtx4x4 = vet4_mtx4x4sse;
       pfn_vet3_mtx4x4 = vet3_mtx4x4sse;
   }
   else{
       pfn_mtx4x4_mult = mtx4x4_mult;
       pfn_vet4_mtx4x4 = vet4_mtx4x4;
       pfn_vet3_mtx4x4 = vet3_mtx4x4;
   }
}
//---------------------------------------------------------------------------
DWORD OpenGLPlugIn::SetInfo(LPSETPLUGININFO p)
{
   if(p == NULL)
       return FALSE;
   if(p->dwStateMask & PIS_NOTIFYMASK){
       switch(p->dwState & PIS_NOTIFYMASK){
           case PNM_CHANGECONFIG:
               switch(p->lParam){
                   case 0:
                       OnChangeConfig();
                   break;
                   case 1:
                       p->lpNDS->get_NumberOfCores(&dwCores);
                       EnableMultiCores((nOptions & NTR_USE_MULTICORES) != 0);
                   break;
               }
           break;
           case PNM_INCREASECPUSPEED:
               gx_fifo.div = 100 + (100 * p->lParam / 100);
           break;
           case PNM_ENDLINE:
               Render3D(p->lParam);
           break;
           case 0xC00:
           	EndPipelineExec();
           break;
           case PNM_ENTERVBLANK:
               ReadBuffer3D(p->lParam);    
           break;
           case PNM_STARTFRAME:
               if(frame_mode != (DWORD)-1){
                   WaitForSingleObject(hEvents[2],INFINITE);
                   frame_mode = (DWORD)-1;
               }
           break;
           case PNMV_CHANGEVRAMCNT:
               if(LOWORD(p->lParam) < 7){
                   switch(HIWORD(p->lParam)&7){
                       case 0:
                           bMemoryChanged |= (u8)(1 << LOWORD(p->lParam));
                       break;
                       case 3:  //texture
                           switch(LOWORD(p->lParam)){
                               case 3:
                               case 2:
                                   bCImage |= 2;
                               default:
                                   if(bMemoryChanged & (1 << LOWORD(p->lParam))){
                                       ResetTextureBuffer(FALSE);
                                       bMemoryChanged &= (u8)~(1 << LOWORD(p->lParam));
                                   }
                               break;
                               case 4:
                                   if(bMemoryChanged & 0x10){
                                       ResetTextureBuffer(FALSE);
                                       bMemoryChanged &= ~0x10;
                                   }
                                   texpalMem = 0;
                               break;
                               case 5:
                                   if(bMemoryChanged & 0x20){
                                       ResetTextureBuffer(FALSE);
                                       bMemoryChanged &= ~0x20;
                                   }
                                   texpalMem = 0x10000;
                               break;
                               case 6://
                                   if(bMemoryChanged & 0x40){
                                       ResetTextureBuffer(FALSE);
                                       bMemoryChanged &= ~0x40;
                                   }
                                   texpalMem = 0x14000;
                               break;
                           }
                       break;
                   }
               }
           break;
           case PNMV_GETBUFFER:
               *((u32 *)p->lParam) = (u32)outBuffer;
           break;
           case PNMV_GETSTACK:
               ((void **)p->lParam)[0] = get_Stack((int)((void **)p->lParam)[1],(int)((void **)p->lParam)[2]);
           break;
#ifdef _DEBPRO
           case PNMV_CLEARTEXTUREBUFFER:
               ResetTextureBuffer(TRUE);
           break;
           case PNMV_GETTEXTUREINFO:
               if(((void **)p->lParam)[0] != NULL){
                   get_TextureData(&tex[TEXTURE_ALLOCATED-1],((u32 *)p->lParam)[0]);
                   ((void **)p->lParam)[0] = (void *)get_TextureInfo(TEXTURE_ALLOCATED-1);
               }
               else
                   ((void **)p->lParam)[0] = (void *)get_TextureInfo((int)((void **)p->lParam)[1]);
           break;
           case PNMV_GETTEXTURE:
               CopyMemory(&tex[TEXTURE_ALLOCATED-1],((void **)p->lParam)[1],sizeof(TEXTURE));
               if(tex[TEXTURE_ALLOCATED-1].data != -1){
                   get_TextureData(&tex[TEXTURE_ALLOCATED-1],tex[TEXTURE_ALLOCATED-1].data);
               }
               CreateTexture(((void **)p->lParam)[0],TEXTURE_ALLOCATED-1);
           break;
           case PNMV_DRAWPOLYGON:
               if(p->lParam != NULL){
                   if(((u32 *)p->lParam)[0] == (u32)-1){
                       switch(((u32 *)p->lParam)[1]){
                           case -1:
                               comBuffer.clear();
                               ((u32 *)p->lParam)[0] = 0;
                           break;
                           case -10:
                               id_poly_render_only[0] = ((u32 *)p->lParam)[2];
                           break;
                           case -11:
                               id_poly_render_only[1] = ((u32 *)p->lParam)[2];
                               ((u32 *)p->lParam)[0] = 0;
                           break;
                           case -12:
                               if(((u32 *)p->lParam)[2])
                                   nOptions |= NTR_RENDER_ONLY_POLYGONS;
                               else
                                   nOptions &= ~NTR_RENDER_ONLY_POLYGONS;
                           break;
                           case -13:
                               if(((u32 *)p->lParam)[2])
                                   nOptions |= NTR_DONT_USE_LIGHT;
                               else
                                   nOptions &= ~NTR_DONT_USE_LIGHT;
                           break;
                           default:
                               comBuffer.set_ClearBuffer(((u32 *)p->lParam)[1]);
                               ((u32 *)p->lParam)[0] = 0;
                           break;
                       }
                   }
                   else{
                       comBuffer.draw((u32 *)p->lParam);
                       wglMakeCurrent(NULL,NULL);
                       if(hRC != NULL)
                           wglMakeCurrent(v3d.lpWnd->DC(),hRC);
                   }
               }
           break;
           case PNMV_ENABLELOG:
               switch(p->lParam){
                   case 0:
                       if(LINO != NULL){
                           fprintf(LINO,"End Log\n");
                           fclose(LINO);
                           LINO = NULL;
                       }
                   break;
                   case 1:
                       if(LINO == NULL){
                       	if((LINO = fopen("f:\\3d.txt","at+")) != NULL)
                           	fprintf(LINO,"Start Log\n");
                   	}
               	break;
               	case 2:
                   	if(LINO != NULL){
                       	fclose(LINO);
                       	LINO = NULL;
                   	}
               		LINO = fopen("f:\\3d.txt","w+");
           		break;
           		case 3:
               		if(LINO1 != NULL){
                   		fclose(LINO1);
                   		LINO1 = NULL;
               		}
           		break;
           		case 4:
               		if(LINO1 == NULL)
               			LINO1 = fopen("f:\\fifo.txt","at+");
       			break;
       			case 5:
           			if(LINO1 != NULL){
               			fclose(LINO1);
               			LINO1 = NULL;
           			}
       				LINO1 = fopen("f:\\fifo.txt","wt+");
   				break;
				}
			break;
			case PNMV_CAPTURETEXTURES:
				if(*((int *)p->lParam) < 0)
	   				*((int *)p->lParam) = bSave;
				else
					bSave = (u8)*((int *)p->lParam);
			break;
#endif
		}
	}
	return TRUE;
}
//---------------------------------------------------------------------------
unsigned long I_FASTCALL OpenGLPlugIn::Run(unsigned long x,unsigned long y)
{
   int g;

   if(outBuffer == NULL)
       return -1;
   x = *((u32 *)&outBuffer[((191 - y) << 10) + (x << 2)]);
   if((x & 0xFF000000) == 0)
       return (u32)-1;
   if((u8)nOptions == 0)
       return (u32)(0x40000000 | ((x & 0xFF000000) >> 8) | ((u8)x >> 3) | (((u16)x >> 11) << 5) | (((x >> 19) & 0x1F) << 10));
   y = (int)(u8)x;
   y += (y * (u8)nOptions) >> 7;
   if(y > 255)
       y = 255;
   x >>= 8;
   g = (int)(u8)x;
   g += (g * (u8)nOptions) >> 7;
   if(g > 255)
       g = 255;
   y = (y >> 3) | ((g >> 3) << 5);
   x >>= 8;
   g = (int)(u8)x;
   g += (g * (u8)nOptions) >> 7;
   if(g > 255)
       g = 255;
   y |= (g >> 3) << 10;
   return (u32)(0x40000000 | ((x >> 8) << 16) | y);
}
//---------------------------------------------------------------------------
BOOL OpenGLPlugIn::SetProperty(LPSETPROPPLUGIN p)
{
	PROPSHEETPAGE psp;
	HPROPSHEETPAGE hpsp;

	ZeroMemory(&psp,sizeof(PROPSHEETPAGE));
   psp.dwSize = sizeof(PROPSHEETPAGE);
   psp.dwFlags = PSP_DEFAULT|PSP_USETITLE;
   psp.hInstance = langManager.get_CurrentLib();
   psp.pfnDlgProc = (DLGPROC)DialogProc;
   psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG15);
   psp.pszTitle = "OpenGL Plugin";
   hpsp = CreatePropertySheetPage(&psp);
   if(hpsp == NULL)
		return FALSE;
   return PropSheet_AddPage((HWND)p->hwndOwner,hpsp);
}
//---------------------------------------------------------------------------
BOOL CALLBACK OpenGLPlugIn::DialogProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   u64 iPos;
   int i;
   LString s1,s2,s3;
   char *p;

	switch(uMsg){
		case WM_INITDIALOG:
           glEnd();
           s3 = "OpenGL Features :\r\n------------------------------";
           p = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
           if(p != NULL)
               s2 = p;
           s3 += "\r\n  Shading Language : ";
           s3 += s2;
           s1 = (char *)glGetString(GL_EXTENSIONS);
           if(!s1.IsEmpty()){
               s3 += "\r\n\r\n  Extensions :\r\n------------------------------\r\n";
               while(1){
                   s2 = s1.NextToken(32);
                   if(s2.IsEmpty())
                       break;
                   s2.AllTrim();
                   s3 += s2;
                   s3 += "\r\n";
               }
           }
           SendDlgItemMessage(hwnd,IDC_EDIT2,WM_SETTEXT,0,(LPARAM)s3.c_str());
           SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)(iPos = (u64)(u8)nOptions));
           s1 = (int)iPos;
           SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)s1.c_str());
           SendDlgItemMessage(hwnd,IDC_WBUFFER,BM_SETCHECK,(nOptions & 0x100) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK1,BM_SETCHECK,(nOptions & NTR_LIGHT0_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK2,BM_SETCHECK,(nOptions & NTR_LIGHT1_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK3,BM_SETCHECK,(nOptions & NTR_LIGHT2_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK4,BM_SETCHECK,(nOptions & NTR_LIGHT3_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK5,BM_SETCHECK,(nOptions & NTR_TEXTURE_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK6,BM_SETCHECK,(nOptions & NTR_BLEND_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK7,BM_SETCHECK,(nOptions & NTR_ALPHA_TEST_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK8,BM_SETCHECK,(nOptions & NTR_FOG_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK14,BM_SETCHECK,(nOptions & NTR_SHADERS_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK11,BM_SETCHECK,(nOptions & 0x400000) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK12,BM_SETCHECK,(nOptions & NTR_FOG_ALPHA_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK16,BM_SETCHECK,(nOptions & NTR_KEEP_TRANSLUCENT) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK17,BM_SETCHECK,(nOptions & NTR_USE_SOFT_LIGHT) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK18,BM_SETCHECK,(nOptions & NTR_USE_DIRECT_COMMAND) ? BST_CHECKED : BST_UNCHECKED,0);
           SendDlgItemMessage(hwnd,IDC_CHECK19,BM_SETCHECK,(nOptions & NTR_USE_MULTICORES) ? BST_CHECKED : BST_UNCHECKED,0);
#ifndef __WIN32__
            EnableWindow(GetDlgItem(hwnd,IDC_CHECK19),FALSE);
#endif
           SendDlgItemMessage(hwnd,IDC_CHECK9,CB_ADDSTRING,0,(LPARAM)"None");
           SendDlgItemMessage(hwnd,IDC_CHECK9,CB_ADDSTRING,0,(LPARAM)"MultiSample 2x");
           SendDlgItemMessage(hwnd,IDC_CHECK9,CB_ADDSTRING,0,(LPARAM)"MultiSample 4x");
           if(nOptions & NTR_USE_DIRECT_COMMAND)
               EnableWindow(GetDlgItem(hwnd,IDC_CHECK19),FALSE);

           if(nOptions & NTR_MULTISAMPLE_MASK){
               EnableWindow(GetDlgItem(hwnd,IDC_CHECK9),TRUE);
               iPos = (int)(((nOptions & NTR_MULTISAMPLE_MASK) >> NTR_MULTISAMPLE_SHIFT) - 1);
               SendDlgItemMessage(hwnd,IDC_CHECK9,CB_SETCURSEL,(WPARAM)iPos,0);
           }
           else
               EnableWindow(GetDlgItem(hwnd,IDC_CHECK9),FALSE);
           if((nOptions & (NTR_FRAGMENT_SHADER_FLAG|NTR_VERTEX_SHADER_FLAG)) != (NTR_FRAGMENT_SHADER_FLAG|NTR_VERTEX_SHADER_FLAG)){
               EnableWindow(GetDlgItem(hwnd,IDC_CHECK14),FALSE);
               SendDlgItemMessage(hwnd,IDC_CHECK14,BM_SETCHECK,FALSE,0);
               EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),FALSE);
               SendDlgItemMessage(hwnd,IDC_CHECK13,BM_SETCHECK,FALSE,0);
               EnableWindow(GetDlgItem(hwnd,IDC_CHECK15),FALSE);
               SendDlgItemMessage(hwnd,IDC_CHECK15,BM_SETCHECK,FALSE,0);
           }
           else{
               SendDlgItemMessage(hwnd,IDC_CHECK14,BM_SETCHECK,(nOptions & NTR_SHADERS_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
               if(nOptions & NTR_SHADERS_OPTION){
                   EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),TRUE);
                   SendDlgItemMessage(hwnd,IDC_CHECK13,BM_SETCHECK,(nOptions & NTR_EDGE_MARKING_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
                   EnableWindow(GetDlgItem(hwnd,IDC_CHECK15),TRUE);
                   SendDlgItemMessage(hwnd,IDC_CHECK15,BM_SETCHECK,(nOptions & NTR_FRAMEBUFFER_OBJ_OPTION) ? BST_CHECKED : BST_UNCHECKED,0);
               }
               else{
                   EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),FALSE);
                   SendDlgItemMessage(hwnd,IDC_CHECK13,BM_SETCHECK,BST_UNCHECKED,0);
                   EnableWindow(GetDlgItem(hwnd,IDC_CHECK15),FALSE);
                   SendDlgItemMessage(hwnd,IDC_CHECK15,BM_SETCHECK,BST_UNCHECKED,0);
               }
           }
       break;
#ifdef __WIN32__
		case WM_CTLCOLORSTATIC:
       case WM_CTLCOLOREDIT:
   		if(GetDlgCtrlID((HWND)lParam) == IDC_EDIT1 || GetDlgCtrlID((HWND)lParam) == IDC_EDIT2){
       		SetTextColor((HDC)wParam,GetSysColor(COLOR_WINDOWTEXT));
	           	return (BOOL)GetSysColorBrush(COLOR_WINDOW);
       	}
       break;
#endif
       case WM_HSCROLL:
           switch(LOWORD(wParam)){
               case TB_THUMBTRACK:
               case TB_ENDTRACK:
                   s1 = (int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
                   switch(GetDlgCtrlID((HWND)lParam)){
                       case IDC_TRACK1:
                           SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)s1.c_str());
                       break;
                   }
               break;
               default:
               break;
           }
       break;
       case WM_NOTIFY:
           if(((LPNMHDR)lParam)->code == PSN_APPLY){
               iPos = (u64)(nOptions & (0xA3900000|NTR_RENDER_WIREFRAME|NTR_DONT_USE_LISTS|NTR_RENDER_ONLY_POLYGONS|NTR_DONT_USE_LIGHT));
               iPos |= SendDlgItemMessage(hwnd,IDC_WBUFFER,BM_GETCHECK,0,0) == BST_CHECKED ? 0x100 :0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK1,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_LIGHT0_OPTION : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK2,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_LIGHT1_OPTION : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK3,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_LIGHT2_OPTION : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK4,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_LIGHT3_OPTION : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK5,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_TEXTURE_OPTION : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK6,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_BLEND_OPTION : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK7,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_ALPHA_TEST_OPTION : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK8,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_FOG_OPTION : 0;
               if(nOptions & NTR_MULTISAMPLE_MASK){
                   i = SendDlgItemMessage(hwnd,IDC_CHECK9,CB_GETCURSEL,0,0);
                   if(i != CB_ERR)
                       iPos |= (++i << NTR_MULTISAMPLE_SHIFT);
               }
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK9,BM_GETCHECK,0,0) == BST_CHECKED ? 0x40000 : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK11,BM_GETCHECK,0,0) == BST_CHECKED ? 0x400000 : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK12,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_FOG_ALPHA_OPTION : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK14,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_SHADERS_OPTION : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK16,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_KEEP_TRANSLUCENT : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK17,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_USE_SOFT_LIGHT : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK18,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_USE_DIRECT_COMMAND : 0;
               iPos |= SendDlgItemMessage(hwnd,IDC_CHECK19,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_USE_MULTICORES : 0;
               if(iPos & NTR_SHADERS_OPTION){
                   iPos |= SendDlgItemMessage(hwnd,IDC_CHECK13,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_EDGE_MARKING_OPTION : 0;
                   iPos |= SendDlgItemMessage(hwnd,IDC_CHECK15,BM_GETCHECK,0,0) == BST_CHECKED ? NTR_FRAMEBUFFER_OBJ_OPTION : 0;
               }
               iPos |= (u8)(SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_GETPOS,0,0) * 128 / 100);

               if(iPos & NTR_USE_DIRECT_COMMAND)
                   iPos &= ~NTR_USE_MULTICORES;
               i = (iPos & NTR_USE_MULTICORES) ? 1 : 0;
               EnableMultiCores(i);
               if(hThread != NULL && ((iPos & NTR_MULTISAMPLE_MASK) != (nOptions & NTR_MULTISAMPLE_MASK))){
                   WaitForSingleObject(hMutex,INFINITE);
                   command = 16;
                   SetEvent(hEvents[0]);
                   ReleaseMutex(hMutex);
                   WaitForSingleObject(hEvents[3],INFINITE);
                   EnableMultiCores(FALSE);
               }
               if(hThread == NULL){
                   set_Options(iPos);
                   if(i)
                       EnableMultiCores(i);
               }
               else{
                   WaitForSingleObject(hMutex,INFINITE);
                   command = (command & 4) | 8;
                   command_value = iPos;
                   SetEvent(hEvents[0]);
                   ReleaseMutex(hMutex);
                   WaitForSingleObject(hEvents[3],INFINITE);
               }
           }
       break;
       case WM_COMMAND:
           switch(LOWORD(wParam)){
               case IDC_CHECK14:
                   switch(HIWORD(wParam)){
                       case BN_CLICKED:
                           i = SendDlgItemMessage(hwnd,IDC_CHECK14,BM_GETCHECK,0,0) == BST_CHECKED ? TRUE : FALSE;
                           EnableWindow(GetDlgItem(hwnd,IDC_CHECK13),i);
                           EnableWindow(GetDlgItem(hwnd,IDC_CHECK15),i);
                       break;
                   }
               break;
               case IDC_CHECK18:
                   switch(HIWORD(wParam)){
                       case BN_CLICKED:
                           i = SendDlgItemMessage(hwnd,IDC_CHECK18,BM_GETCHECK,0,0) != BST_CHECKED ? TRUE : FALSE;
                           EnableWindow(GetDlgItem(hwnd,IDC_CHECK19),i);
                       break;
                   }
               break;
           }
       break;
	}
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL color_M::Save(LStream *p)
{
   p->Write(color,sizeof(color));
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL color_M::Load(LStream *p,int ver)
{
   p->Read(color,sizeof(color));
   return TRUE;
}
//---------------------------------------------------------------------------
void color_M::apply()
{
   if(!used)
    	return;
   if(type)
       glMaterialfv(GL_FRONT_AND_BACK,type,color);
   else
      	glColor4fv(color);
}
//---------------------------------------------------------------------------
void color_M::set(int r,int g,int b)
{
   color[0] = r * 31.0f;
   color[1] = g * 31.0f;
   color[2] = b * 31.0f;
   used = TRUE;
}
//---------------------------------------------------------------------------
void color_M::set(int r,int g,int b,int a)
{
   color[0] = r * (1.0f / 31.0f);
   color[1] = g * (1.0f / 31.0f);
   color[2] = b * (1.0f / 31.0f);
   color[3] = a * (1.0f / 31.0f);
   used = TRUE;
}
//---------------------------------------------------------------------------
color_M& color_M::operator =(const color_M &C)
{
   color[0] = C.color[0];
   color[1] = C.color[1];
   color[2] = C.color[2];
//   color[3] = C.color[3];
   return *this;
}
//---------------------------------------------------------------------------
LLight::LLight()
{
	index = 0;
	for(int i=0;i<4;i++){
		color[i] = 1;
		pos[i] = 0;
	}
}
//---------------------------------------------------------------------------
LLight::~LLight()
{
	enable(FALSE);
}
//---------------------------------------------------------------------------
void LLight::reset()
{
	enable(FALSE);
	for(int i=0;i<4;i++){
		color[i] = 1;
       pos[i] = 0;
	}
}
//---------------------------------------------------------------------------
void LLight::enable(BOOL bEnable,float *mtx)
{
	GLenum l;
	float v[4];

   l = index + GL_LIGHT0;
   if(bEnable){
       v[0] = pos[0];v[1] = pos[1];v[2] = pos[2];v[3] = pos[3];
       if(mtx != NULL)
     		pfn_vet3_mtx4x4(v,mtx);
       glDisable(l);
		if((nOptions & NTR_DONT_USE_LIGHT) == 0){
           if((nOptions & NTR_USE_SOFT_LIGHT) == 0){
               glEnable(l);
               glLightfv(l,GL_POSITION,v);
               glLightfv(l,GL_AMBIENT,color);
               glLightfv(l,GL_DIFFUSE,color);
               glLightfv(l,GL_SPECULAR,color);
           }
       }
   }
   else
   	glDisable(l);
}
//---------------------------------------------------------------------------
void LLight::set_pos(float x,float y,float z)
{
   float hf[4]={0,0,-1};

/*   if(ds.hasSSE())
       vet3_mtx4x4sse(hf,mtxProjection);
   else
       vet3_mtx4x4(hf,mtxProjection);*/

	pos[0] = x;pos[1] = y;pos[2] = z;
	half_pos[0] = (pos[0] + hf[0]) * 0.5f;
   half_pos[1] = (pos[1] + hf[1]) * 0.5f;
   half_pos[2] = (pos[2] + hf[2]) * 0.5f;

}
//---------------------------------------------------------------------------
BOOL LLight::Save(LStream *p)
{
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL LLight::Load(LStream *p,int ver)
{
	return TRUE;
}
#ifdef _DEBPRO
//---------------------------------------------------------------------------
LCommandBuffer::LCommandBuffer()
{
	bEnable = TRUE;
   wObject = (u16)-1;
   nSwap = 0;
   nClearBuffer = 1;
}
//---------------------------------------------------------------------------
LCommandBuffer::~LCommandBuffer()
{
}
//---------------------------------------------------------------------------
BOOL LCommandBuffer::new_Object(u32 value)
{
	if(!bEnable)
   	return FALSE;
	wObject++;
	if(!add_Command(0x4A4,value,AMM_WORD)){
       wObject--;
   	return FALSE;
   }
}
//---------------------------------------------------------------------------
BOOL LCommandBuffer::add_Command(u32 adr,u32 data,u8 am)
{
	u64 value;

   if(wObject == (u16)-1 || !bEnable)
   	return FALSE;
   value = wObject;
   value <<= 48;
   value |= (u64)adr << 32;
   value |= (u64)(am & 0xF) << 44;
   value |= data;
   com.push(value);
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LCommandBuffer::draw(u32 *pobj)
{
   u64 value,oldOptions;
   u32 i,i1,i2;
   u16 adr,obj;
   LPINT_3DFUNC p;
	float v[4];
   LPBYTE pTexture;
	TV_INSERTSTRUCT tvi;
   HTREEITEM hParent,hPreviousTree,hTree;
   char s[100];

	if(pobj == NULL)
   	return FALSE;
   obj = (u16)*pobj;
  	*pobj = 0;
   if(pobj[8] == 0){
       wglMakeCurrent(v3d.lpWnd->DC(),hRC);
       glEnd();
       glEndList();
       idxList = 0;
       glBindTexture(GL_TEXTURE_2D,0);
       glFlush();
       glFinish();
       for(i=0;i<idxTxt;i++){
           if(tex[i].index == 0)
               continue;
           wglMakeCurrent(v3d.lpWnd->DC(),hRC);
           glBindTexture(GL_TEXTURE_2D,tex[i].index);
           pTexture = (LPBYTE)LocalAlloc(LMEM_FIXED,tex[i].h_size*tex[i].v_size*4);
           if(pTexture == NULL)
               continue;
           glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,(GLvoid *)pTexture);
           glBindTexture(GL_TEXTURE_2D,0);
           glDeleteTextures(1,&tex[i].index);

           wglMakeCurrent((HDC)pobj[3],(HGLRC)pobj[4]);
           glGenTextures(1,&tex[i].index);
           glBindTexture(GL_TEXTURE_2D,tex[i].index);
           glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,tex[i].h_size,tex[i].v_size,0,GL_RGBA,GL_UNSIGNED_BYTE,(GLvoid *)pTexture);
           glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
           glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
           glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,tex[i].fs ? GL_MIRRORED_REPEAT : tex[i].ws ? GL_REPEAT : GL_CLAMP);
           glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,tex[i].ft ? GL_MIRRORED_REPEAT : tex[i].wt ? GL_REPEAT : GL_CLAMP);

           tex[i].collect_level = 0;
           tex[i].used = 1;

           LocalFree(pTexture);
       }

       wglMakeCurrent((HDC)pobj[3],(HGLRC)pobj[4]);

       nBeginMode = 0;
       idxVtx = 0;
       bEnable = FALSE;
       oldOptions = nOptions;
       nOptions = pobj[2];
       nOptions <<= 32;
       nOptions |= pobj[1];

       for(i=0;i<8;i++)
           color[i].set_used(TRUE);
       if(obj == (u16)-1){
       }
       else{
           for(i=1;i<=back.count();i++){
               value = back[i];
               if((value >> 48) != obj)
                   continue;
               i1 = (u32)value;
               adr = (u16)((value >> 32) & 0xFFF);
               if(adr == 0x4A4){
                   *pobj = i1;
                   i1 |= 0x1F0000;
               }
               switch(adr){
                   case 0:
                       v[0] = (float)(int)value / 16384.0f;
                       for(i2=1,i++;i2<4;i2++,i++)
                           v[i2] = (float)(int)back[i] / 16384.0f;
                       --i;
                       if(BeginMode(1))
                           glVertex4fv(v);
                   break;
                   case 1:
                       v[0] = (float)(int)value / 16384.0f;
                       for(i2=1,i++;i2<3;i2++,i++)
                           v[i2] = (float)(int)back[i] / 16384.0f;
                       --i;
                       BeginMode(4);
                       glNormal3fv(v);
                   break;
                   case 2:
                       v[0] = (float)(int)value / 16384.0f;
                       for(i2=1,i++;i2<2;i2++,i++)
                           v[i2] = (float)(int)back[i] / 16384.0f;
                       --i;
                       if(BeginMode(2))
                           glTexCoord2fv(v);
                       else{
                           st[0] = v[0];
                           st[1] = v[1];
                           crTex = NULL;
                       }
                   break;
					default:
                   	if(((value >> 44) & 0xF) != 0){
                           p = &ioFunc[ioTable[adr]];
                           p->pfn_1((u32)adr,i1,(u8)((value >> 44) & 0xF));
                       }
					break;
               }
           }
       }
       bEnable = TRUE;
       glEnd();
       if(*pobj){
           glFlush();
           glFinish();
           GdiFlush();
           glReadBuffer(GL_BACK);
           glReadPixels(0,0,pobj[6],pobj[7],GL_BGRA,GL_UNSIGNED_BYTE,(GLvoid *)pobj[5]);
       }
       glBindTexture(GL_TEXTURE_2D,0);
       for(i=0;i<idxTxt;i++){
           if(tex[i].index == 0)
               continue;
           wglMakeCurrent((HDC)pobj[3],(HGLRC)pobj[4]);
           glBindTexture(GL_TEXTURE_2D,tex[i].index);
           pTexture = (LPBYTE)LocalAlloc(LMEM_FIXED,tex[i].h_size*tex[i].v_size*4);
           if(pTexture == NULL)
               continue;
           glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,(GLvoid *)pTexture);
           glBindTexture(GL_TEXTURE_2D,0);
           glDeleteTextures(1,&tex[i].index);

           wglMakeCurrent(v3d.lpWnd->DC(),hRC);
           glGenTextures(1,&tex[i].index);
           glBindTexture(GL_TEXTURE_2D,tex[i].index);
           glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,tex[i].h_size,tex[i].v_size,0,GL_RGBA,GL_UNSIGNED_BYTE,(GLvoid *)pTexture);
           glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
           glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
           LocalFree(pTexture);
       }
       nBeginMode = 0;
       idxVtx = 0;
       nOptions = oldOptions;
   }
	else{
   	TreeView_DeleteAllItems((HWND)pobj[8]);
   	memset(&tvi,0,sizeof(TV_INSERTSTRUCT));
       hParent = NULL;
       for(i=1;i<=back.count();i++){
           value = back[i];
           if((value >> 48) != obj)
               continue;
           i1 = (u32)value;
           adr = (u16)((value >> 32) & 0xFFF);
           if(adr == 0x4A4){
               *pobj = i1;
               i1 |= 0x1F0000;
               tvi.hParent = TVI_ROOT;
               tvi.hInsertAfter = TVI_LAST;
               tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               wsprintf(s,"Polygon %d - Blend %1X",obj+1,(i1 >> 4) & 3);
               tvi.item.pszText = s;
               tvi.item.lParam = i;
               hParent = TreeView_InsertItem((HWND)pobj[8],&tvi);
           }
           switch(adr){
               case 0:
                   v[0] = (float)(int)value / 16384.0f;
                   for(i2=1,i++;i2<4;i2++,i++)
                       v[i2] = (float)(int)back[i] / 16384.0f;
                   --i;
                   sprintf(s,"VERTEX %f,%f,%f,%f",v[0],v[1],v[2],v[3]);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i-3;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 1:
                   v[0] = (float)(int)value / 16384.0f;
                   for(i2=1,i++;i2<3;i2++,i++)
                       v[i2] = (float)(int)back[i] / 16384.0f;
                   --i;
                   sprintf(s,"NORMAL %f,%f,%f",v[0],v[1],v[2]);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i-2;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 2:
                   v[0] = (float)(int)value / 16384.0f;
                   for(i2=1,i++;i2<2;i2++,i++)
                       v[i2] = (float)(int)back[i] / 16384.0f;
                   --i;
                   sprintf(s,"TEXCOOR %f,%f",v[0],v[1]);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i-1;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 3:
               	if(hPreviousTree != NULL){
                   	sprintf(s,"Internal Polygon ID 0x%08X",i1);
                       tvi.hParent = hPreviousTree;
                       tvi.hInsertAfter = TVI_LAST;
                       tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
                       tvi.item.pszText = s;
                       tvi.item.lParam = 0x80000000|i1;
                       TreeView_InsertItem((HWND)pobj[8],&tvi);
                   }
               break;
               case 0x440:
                   sprintf(s,"MTX_MODE ");
                   switch(i1){
                   	case 0:
                       	lstrcat(s,"Projection");
                       break;
                       case 1:
                       	lstrcat(s,"View");
                       break;
                       case 2:
                       	lstrcat(s,"Position & View");
                       break;
                       case 3:
                       	lstrcat(s,"Texture");
                       break;
                   }
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
				break;
               case 0x444:
                   sprintf(s,"MTX_PUSH 0x%02X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x448:
                   sprintf(s,"MTX_POP 0x%02X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x44C:
                   sprintf(s,"MTX_STORE 0x%02X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x450:
                   sprintf(s,"MTX_RESTORE 0x%02X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x454:
                   sprintf(s,"MTX_IDENTITY");
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
				break;
               case 0x458:
                   sprintf(s,"MTX_LOAD_4X4");
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x45C:
                   sprintf(s,"MTX_LOAD_4X3");
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
				break;
               case 0x460:
                   sprintf(s,"MTX_MULT_4X4");
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
				break;
               case 0x464:
                   sprintf(s,"MTX_MULT_4X3");
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x468:
                   sprintf(s,"MTX_MULT_3X3");
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x46C:
                   sprintf(s,"MTX_SCALE");
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
				break;
               case 0x470:
                   sprintf(s,"MTX_TRANS");
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
				break;
               case 0x480:
                   sprintf(s,"COLOR 0x%02X 0x%02X 0x%02X",(i1 & 0x1F),((i1 >> 5) & 0x1F),((i1 >> 10) & 0x1F));
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x4A8:
                   sprintf(s,"TEXIMAGE_PARAM 0x%08X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	hTree = TreeView_InsertItem((HWND)pobj[8],&tvi);
					if(hTree != NULL){
                        tvi.hParent = hTree;
                        tvi.hInsertAfter = TVI_LAST;
                        tvi.item.mask = TVIF_TEXT;
                        tvi.item.pszText = s;
                        sprintf(s,"Width %d",(8 << (((i1 >> 20) & 7))));
                        TreeView_InsertItem((HWND)pobj[8],&tvi);
                        sprintf(s,"Height %d",(8 << (((i1 >> 20) & 7))));
                        TreeView_InsertItem((HWND)pobj[8],&tvi);
                        sprintf(s,"Format %d",((i1 >> 26) & 7));
                        TreeView_InsertItem((HWND)pobj[8],&tvi);
					}
				break;
				case 0x4AC:
                   sprintf(s,"PLTT_BASE 0x%04X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
				break;
               case 0x4C0:
                   sprintf(s,"DIF_AMB 0x%08X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x4C4:
                   sprintf(s,"SPE_EMI 0x%08X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x4C8:
                   sprintf(s,"LIGHT_VECTOR 0x%08X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x4CC:
                   sprintf(s,"LIGHT_COLOR 0x%08X",i1);
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
               case 0x500:
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
                   sprintf(s,"BEGIN_VTX ");
                   switch(i1){
                       case 0:
                       	lstrcat(s,"TRIANGLE");
                       break;
                       case 1:
                       	lstrcat(s,"QUAD");
                       break;
                       case 2:
                       	lstrcat(s,"TRIANGLE_STRIP");
                       break;
                       case 3:
                       	lstrcat(s,"QUAD STRIP");
                       break;
                   }
               	tvi.item.pszText = s;
                   tvi.item.lParam = i;
               	hPreviousTree = TreeView_InsertItem((HWND)pobj[8],&tvi);
               break;
				case 0x504:
					tvi.hParent = hParent;
               	tvi.hInsertAfter = TVI_LAST;
               	tvi.item.mask = TVIF_TEXT|TVIF_PARAM;
               	tvi.item.pszText = "END_VTX";
                   tvi.item.lParam = i;
               	TreeView_InsertItem((HWND)pobj[8],&tvi);
  					hParent = TreeView_GetRoot((HWND)pobj[8]);
               break;
               default:
               break;
           }
       }
		hParent = TreeView_GetRoot((HWND)pobj[8]);
       if(hParent != NULL)
       	TreeView_Expand((HWND)pobj[8],hParent,TVE_EXPAND);
   }
	return TRUE;
}
//---------------------------------------------------------------------------
void LCommandBuffer::swap()
{
	back = com;
 	nSwap++;
   if(nSwap > nClearBuffer){
       wObject = (u16)-1;
       com.free();
       nSwap = 0;
   }
}
//---------------------------------------------------------------------------
void LCommandBuffer::clear()
{
	com.clear();
	back.clear();
   wObject = (u16)-1;
}
//---------------------------------------------------------------------------
void LCommandBuffer::reset()
{
	clear();
   nSwap = 0;
}
#endif




