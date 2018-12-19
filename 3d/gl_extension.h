#include "ideastypes.h"
//---------------------------------------------------------------------------
#ifndef __OPENGLEXTENSION__
#define __OPENGLEXTENSION__

//---------------------------------------------------------------------------
#define NTR_DEPTH_TEST_OPTION	    0x00000100
#define NTR_KEEP_TRANSLUCENT		0x00000200
#define NTR_LIGHT0_OPTION		    0x00000400
#define NTR_LIGHT1_OPTION		    0x00000800
#define NTR_LIGHT2_OPTION		    0x00001000
#define NTR_LIGHT3_OPTION		    0x00002000
#define NTR_TEXTURE_OPTION		    0x00004000
#define NTR_BLEND_OPTION		    0x00008000
#define NTR_ALPHA_TEST_OPTION	    0x00010000
#define NTR_FOG_OPTION			    0x00020000
#define NTR_MULTISAMPLE_SUPPORTED	0x00040000
#define NTR_MULTISAMPLE_MASK       0x000C0000
#define NTR_MULTISAMPLE_SHIFT      18
#define NTR_ASYNC_READ_FLAG	    0x00100000
#define NTR_ASYNC_READ_OPTION	    0x00200000
#define NTR_SYNC_SWAP_BUFFER		0x00400000
#define NTR_TEX_COMBINE_FLAG	    0x00800000
#define NTR_VERTEX_SHADER_FLAG     0x01000000
#define NTR_FRAGMENT_SHADER_FLAG   0x02000000
#define NTR_FOG_ALPHA_OPTION       0x04000000
#define NTR_EDGE_MARKING_OPTION    0x08000000
#define NTR_SHADERS_OPTION         0x10000000
#define NTR_FRAMEBUFFER_OBJ_FLAG   0x20000000
#define NTR_FRAMEBUFFER_OBJ_OPTION 0x40000000
#define NTR_FOGCOORD_FLAG			0x80000000
#ifdef __BORLANDC__
	#define NTR_RENDER_WIREFRAME		0x100000000
	#define NTR_DONT_USE_LISTS			0x200000000
	#define NTR_RENDER_ONLY_POLYGONS	0x400000000
	#define NTR_DONT_USE_LIGHT			0x800000000
	#define NTR_USE_SOFT_LIGHT			0x1000000000
   #define NTR_USE_DIRECT_COMMAND		0x2000000000
   #define NTR_USE_MULTICORES  		0x4000000000
#else
	#define NTR_RENDER_WIREFRAME		0x100000000LLU
	#define NTR_DONT_USE_LISTS			0x200000000LLU
	#define NTR_RENDER_ONLY_POLYGONS	0x400000000LLU
	#define NTR_DONT_USE_LIGHT			0x800000000LLU
	#define NTR_USE_SOFT_LIGHT			0x1000000000LLU
   #define NTR_USE_DIRECT_COMMAND		0x2000000000LLU
   #define NTR_USE_MULTICORES  		0x4000000000LLU      
#endif

extern PFNGLBINDBUFFERPROC         			pfn_glBindBuffer;
extern PFNGLDELETEBUFFERSPROC      			pfn_glDeleteBuffers;
extern PFNGLGENBUFFERSPROC         			pfn_glGenBuffers;
extern PFNGLBUFFERDATAPROC         			pfn_glBufferData;
extern PFNGLMAPBUFFERPROC          			pfn_glMapBuffer;
extern PFNGLUNMAPBUFFERPROC        			pfn_glUnmapBuffer;
extern PFNGLISBUFFERPROC           			pfn_glIsBuffer;
extern PFNGLCREATESHADERPROC       			pfn_glCreateShader;
extern PFNGLCOMPILESHADERPROC      			pfn_glCompileShader;
extern PFNGLSHADERSOURCEPROC       			pfn_glShaderSource;
extern PFNGLATTACHSHADERPROC       			pfn_glAttachShader;
extern PFNGLCREATEPROGRAMPROC      			pfn_glCreateProgram;
extern PFNGLLINKPROGRAMPROC        			pfn_glLinkProgram;
extern PFNGLUSEPROGRAMPROC         			pfn_glUseProgram;
extern PFNGLDELETESHADERPROC       			pfn_glDeleteShader;
extern PFNGLDELETEPROGRAMPROC      			pfn_glDeleteProgram;
extern PFNGLUNIFORM1IVPROC         			pfn_glUniform1iv;
extern PFNGLGETUNIFORMLOCATIONPROC 			pfn_glGetUniformLocation;
extern PFNGLUNIFORM1FVPROC         			pfn_glUniform1fv;
extern PFNGLUNIFORM1IPROC          			pfn_glUniform1i;
extern PFNGLACTIVETEXTUREPROC      			pfn_glActiveTexture;
extern PFNGLBINDFRAMEBUFFEREXTPROC 			pfn_glBindFramebufferEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC 			pfn_glGenFramebuffersEXT;
extern PFNGLDRAWBUFFERSPROC        			pfn_glDrawBuffers;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC 	pfn_glFramebufferRenderbufferEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC 			pfn_glGenRenderbuffersEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC 			pfn_glBindRenderbufferEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC 		pfn_glRenderbufferStorageEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC 		pfn_glFramebufferTexture2DEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC    		pfn_glDeleteRenderbuffersEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC     		pfn_glDeleteFramebuffersEXT;
extern PFNGLFOGCOORDFPROC 					   	pfn_glFogCoordf;

BOOL IsExtensionSupported(const char *ext,char *supported);
BOOL printProgramInfoLog(GLuint obj);
BOOL printShaderInfoLog(GLuint obj);
BOOL checkFramebufferStatus();
DWORD init_gl_extension();

#ifndef __WIN32__
#define wglGetProcAddress(a) glXGetProcAddress((GLubyte *)a)

#endif

#endif
