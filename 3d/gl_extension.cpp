#include "gl_extension.h"

PFNGLBINDBUFFERPROC                    pfn_glBindBuffer;
PFNGLDELETEBUFFERSPROC                 pfn_glDeleteBuffers;
PFNGLGENBUFFERSPROC                    pfn_glGenBuffers;
PFNGLBUFFERDATAPROC                    pfn_glBufferData;
PFNGLMAPBUFFERPROC                     pfn_glMapBuffer;
PFNGLUNMAPBUFFERPROC                   pfn_glUnmapBuffer;
PFNGLISBUFFERPROC                      pfn_glIsBuffer;
PFNGLCREATESHADERPROC                  pfn_glCreateShader;
PFNGLCOMPILESHADERPROC                 pfn_glCompileShader;
PFNGLSHADERSOURCEPROC                  pfn_glShaderSource;
PFNGLATTACHSHADERPROC                  pfn_glAttachShader;
PFNGLCREATEPROGRAMPROC                 pfn_glCreateProgram;
PFNGLLINKPROGRAMPROC                   pfn_glLinkProgram;
PFNGLUSEPROGRAMPROC                    pfn_glUseProgram;
PFNGLDELETESHADERPROC                  pfn_glDeleteShader;
PFNGLDELETEPROGRAMPROC                 pfn_glDeleteProgram;
PFNGLUNIFORM1IVPROC                    pfn_glUniform1iv;
PFNGLGETUNIFORMLOCATIONPROC            pfn_glGetUniformLocation;
PFNGLUNIFORM1FVPROC                    pfn_glUniform1fv;
PFNGLUNIFORM1IPROC                     pfn_glUniform1i;
PFNGLACTIVETEXTUREPROC                 pfn_glActiveTexture;
PFNGLBINDFRAMEBUFFEREXTPROC            pfn_glBindFramebufferEXT;
PFNGLGENFRAMEBUFFERSEXTPROC            pfn_glGenFramebuffersEXT;                          
PFNGLDRAWBUFFERSPROC                   pfn_glDrawBuffers;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC    pfn_glFramebufferRenderbufferEXT;
PFNGLGENRENDERBUFFERSEXTPROC           pfn_glGenRenderbuffersEXT;
PFNGLBINDFRAMEBUFFEREXTPROC            pfn_glBindRenderbufferEXT;
PFNGLRENDERBUFFERSTORAGEEXTPROC        pfn_glRenderbufferStorageEXT;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC       pfn_glFramebufferTexture2DEXT;
PFNGLGETSHADERIVPROC                   pfn_glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC              pfn_glGetShaderInfoLog;
PFNGLGETPROGRAMIVPROC                  pfn_glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC             pfn_glGetProgramInfoLog;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC     pfn_glCheckFramebufferStatusEXT;
PFNGLDELETERENDERBUFFERSEXTPROC        pfn_glDeleteRenderbuffersEXT;
PFNGLDELETEFRAMEBUFFERSEXTPROC         pfn_glDeleteFramebuffersEXT;
PFNGLFOGCOORDFPROC 					pfn_glFogCoordf;
//---------------------------------------------------------------------------
BOOL IsExtensionSupported(const char *ext,char *supported)
{
   int extlen;
   char *p;

   if(supported == NULL)
       supported = (char*)glGetString(GL_EXTENSIONS);
   if(supported == NULL)
       return FALSE;
   extlen = lstrlen(ext);
   for(p = supported;;p++){
		p = strstr(p,ext);
		if(p == NULL)
			return FALSE;
		if(((p == supported || p[-1] == ' ') && (p[extlen] == '\0' || p[extlen] == ' ')))
			return TRUE;
	}
}
//---------------------------------------------------------------------------
BOOL printProgramInfoLog(GLuint obj)
{
   int infologLength,charsWritten;
   char *infoLog;
   BOOL res;

	infologLength = charsWritten = 0;
   res = TRUE;
	pfn_glGetProgramiv(obj,GL_VALIDATE_STATUS,&infologLength);
	if(infologLength == GL_FALSE){
        res = FALSE;
        pfn_glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&infologLength);
        if(infologLength > 1) {
            if((infoLog = (char *)LocalAlloc(LPTR,infologLength)) != NULL){
                pfn_glGetProgramInfoLog(obj,infologLength, &charsWritten,infoLog);
                MessageBox(NULL,infoLog,"iDeaS Emulator",MB_OK|MB_ICONERROR);
                LocalFree(infoLog);
            }
        }
	}
   return res;
}
//---------------------------------------------------------------------------
BOOL printShaderInfoLog(GLuint obj)
{
   int charsWritten,infologLength;
   char *infoLog;
   BOOL res;

   res = TRUE;
   pfn_glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
   if (infologLength > 1) {
       infoLog = (char *)LocalAlloc(LPTR,infologLength);
       pfn_glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
       MessageBox(NULL,infoLog,"iDeaS Emulator",MB_OK|MB_ICONERROR);
       LocalFree(infoLog);
       res = FALSE;
   }
   return res;
}
//---------------------------------------------------------------------------
BOOL checkFramebufferStatus()
{
   GLenum status;
   char s[100],s1[300];

   status = (GLenum)pfn_glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   switch(status){
       case GL_FRAMEBUFFER_COMPLETE_EXT:
           return TRUE;
       case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
           lstrcpy(s,"Framebuffer incomplete, incomplete attachment");
       break;
       case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
           lstrcpy(s,"Framebuffer incomplete, missing attachment");
       break;
       case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
           lstrcpy(s,"Framebuffer incomplete, attached images must have same dimensions");
       break;
       case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
           lstrcpy(s,"Framebuffer incomplete, attached images must have same format");
       break;
       case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
           lstrcpy(s,"Framebuffer incomplete, missing draw buffer");
       break;
       case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
           lstrcpy(s,"Framebuffer incomplete, missing read buffer");
       break;
       case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
           lstrcpy(s,"Unsupported framebuffer format");
       break;
       default:
           lstrcpy(s,"Unknow Error");
       break;
   }
   lstrcpy(s1,"FrameBufferObject info\n------------------------------\n");
   lstrcat(s1,s);
   MessageBox(NULL,s1,"iDeaS Emulator",MB_OK|MB_ICONERROR);
   return FALSE;
}
//---------------------------------------------------------------------------
DWORD init_gl_extension()
{
   DWORD res;

   res = 0;
   pfn_glActiveTexture         = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
   if(IsExtensionSupported("GL_ARB_vertex_shader",NULL))
       res |= NTR_VERTEX_SHADER_FLAG;
   if(IsExtensionSupported("GL_ARB_fragment_shader",NULL))
       res |= NTR_FRAGMENT_SHADER_FLAG;
   if(IsExtensionSupported("GL_EXT_fog_coord",NULL))
       res |= NTR_FOGCOORD_FLAG;

	pfn_glFogCoordf 			= (PFNGLFOGCOORDFPROC)wglGetProcAddress("glFogCoordf");
   pfn_glCreateShader          = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
   pfn_glCompileShader         = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
   pfn_glShaderSource          = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
   pfn_glAttachShader          = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
   pfn_glCreateProgram         = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
   pfn_glLinkProgram           = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
   pfn_glUseProgram            = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
   pfn_glDeleteShader          = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
   pfn_glDeleteProgram         = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
   pfn_glUniform1iv            = (PFNGLUNIFORM1IVPROC)wglGetProcAddress("glUniform1iv");
   pfn_glGetUniformLocation    = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
   pfn_glUniform1fv            = (PFNGLUNIFORM1FVPROC)wglGetProcAddress("glUniform1fv");
   pfn_glUniform1i             = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
   pfn_glGetShaderiv           = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
   pfn_glGetShaderInfoLog      = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
   pfn_glGetProgramiv          = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
   pfn_glGetProgramInfoLog     = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");

   if(pfn_glCreateShader == NULL || pfn_glCompileShader == NULL || pfn_glShaderSource == NULL
       || pfn_glAttachShader == NULL || pfn_glCreateProgram == NULL || pfn_glLinkProgram == NULL
       || pfn_glUseProgram == NULL || pfn_glDeleteShader == NULL || pfn_glDeleteProgram == NULL
       || pfn_glUniform1iv == NULL || pfn_glGetUniformLocation == NULL || pfn_glUniform1i == NULL)
           res &= ~(NTR_FRAGMENT_SHADER_FLAG|NTR_VERTEX_SHADER_FLAG);
   if(IsExtensionSupported("GL_ARB_texture_env_combine",NULL))
       res |= NTR_TEX_COMBINE_FLAG;
   if(IsExtensionSupported("GL_ARB_vertex_buffer_object",NULL)){
       if((pfn_glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBufferARB")) == NULL)
           pfn_glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
       if((pfn_glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferDataARB")) == NULL)
           pfn_glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
       if((pfn_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffersARB")) == NULL)
           pfn_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
       if((pfn_glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffersARB")) == NULL)
           pfn_glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
       if((pfn_glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBufferARB")) == NULL)
           pfn_glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
       if((pfn_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBufferARB")) == NULL)
           pfn_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");
       if((pfn_glIsBuffer = (PFNGLISBUFFERPROC)wglGetProcAddress("glIsBufferARB")) == NULL)
           pfn_glIsBuffer = (PFNGLISBUFFERPROC)wglGetProcAddress("glIsBuffer");
       if(pfn_glBindBuffer != NULL && pfn_glBufferData != NULL && pfn_glDeleteBuffers != NULL &&
           pfn_glGenBuffers != NULL && pfn_glMapBuffer != NULL && pfn_glUnmapBuffer != NULL &&
           pfn_glIsBuffer != NULL)
           res |= NTR_ASYNC_READ_FLAG;
   }
   if(IsExtensionSupported("GL_EXT_framebuffer_object",NULL)){
       pfn_glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
       pfn_glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
       pfn_glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
       pfn_glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
       pfn_glBindRenderbufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
       pfn_glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
       pfn_glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
       pfn_glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers");
       pfn_glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
       pfn_glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
       pfn_glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");

       if(pfn_glGenFramebuffersEXT != NULL && pfn_glBindFramebufferEXT != NULL &&
           pfn_glFramebufferRenderbufferEXT != NULL && pfn_glGenRenderbuffersEXT != NULL &&
           pfn_glGenRenderbuffersEXT != NULL && pfn_glBindRenderbufferEXT != NULL &&
           pfn_glRenderbufferStorageEXT != NULL && pfn_glFramebufferTexture2DEXT != NULL &&
           pfn_glDrawBuffers != NULL)
               res |= NTR_FRAMEBUFFER_OBJ_FLAG;
   }
   return res;
}
