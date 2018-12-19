#include "ideastypes.h"
#include "gl_extension.h"
#include "dstype.h"
#include "lvec.hpp"
#include "pluginmain.h"

#ifndef _3DH_
#define _3DH_

#ifndef NTR_POSITION_MATRIX
#define NTR_PROJECTION_MATRIX  0
#define NTR_POSITION_MATRIX    2
#define NTR_MODELVIEW_MATRIX   1
#define NTR_TEXTURE_MATRIX     3

#endif

#define TEXTURE_ALLOCATED 	4096
//---------------------------------------------------------------------------
typedef struct
{
	u32 data,adr,pal_adr,tex_adr,num_pixels,palmem_adr;
   GLuint index;
   int h_size,v_size,collect_level,idxList;
   u8 ws,wt,fs,ft,used,tr,format,coordMode,slot;
   u16 palData,num_cols;
} TEXTURE,*LPTEXTURE;
//---------------------------------------------------------------------------
typedef struct
{
	u8 param;
   u8 count;
   u16 cycles;
   LPIFUNC pfn_0;
   LPIFUNC pfn_1;
   u32 flags;
} INT_3DFUNC,*LPINT_3DFUNC;
//---------------------------------------------------------------------------
typedef struct
{
   LPIFUNC Func;
   u32 Reg;
   u32 Counter;
   u32 CurrentReg;
   u32 Cycles;
   int Index;
   u32 div;
} GX_FIFO,*LPGX_FIFO;
//---------------------------------------------------------------------------
typedef struct
{
   int value;
   u8 type,id,flags,stencil;
   u32 id_internal,cr_frame;
} Z_BUFFER,*LPZ_BUFFER;
//---------------------------------------------------------------------------
class stack_MTX
{
public:
	stack_MTX();
   virtual ~stack_MTX();
   virtual void init(int size);
	virtual void reset();
   virtual void push();
   virtual BOOL pop(int index);
   virtual void store(int);
   virtual void restore(int);
   inline void set_type(GLenum t){type = t;};
   inline float *get_matrix(){return mtx;};
   inline int get_size(){return maxElem;};
   inline int get_index(){return idxStack;};
   inline void set_index(int index){idxStack = index;};
   BOOL Save(LStream *pFile);
   BOOL Load(LStream *pFile,int ver);
   void operator=(stack_MTX &v);
protected:
   int idxStack,maxElem;
   GLenum type;
   float *mtx;
};
//---------------------------------------------------------------------------
class stack_PM : public stack_MTX
{
public:
	stack_PM() : stack_MTX(){};
   void push();
   BOOL pop(int index);
   void store(int);
   void restore(int);
};
//---------------------------------------------------------------------------
class stack_MV : public stack_MTX
{
public:
	stack_MV() : stack_MTX(){};
   void push();
   BOOL pop(int index);
   void store(int);
   void restore(int);
};
//---------------------------------------------------------------------------
class color_M
{
public:
	color_M(){color[3] = 1;type=0;};
	inline void reset(){used = FALSE;color[3] = 1.0f;};
   inline void set_alpha(int a){color[3] = a / 31.0f;};
   inline void set_type(GLenum t){type = t;};
   inline void set_used(BOOL b = FALSE){used = b;};
   inline float *get_color(){return color;};
   void apply();
   void set(int r,int g,int b);
   void set(int r,int g,int b,int a);
   color_M& operator =(const color_M &C);
   BOOL Save(LStream *p);
   BOOL Load(LStream *p,int ver);
protected:
	BOOL used;
   GLenum type;
	float color[4];
};
//---------------------------------------------------------------------------
class LLight
{
public:
	LLight();
   ~LLight();
   void enable(BOOL bEnable,float *mtx=NULL);
   void reset();
   void set_pos(float x,float y,float z);
   void set_color(float r,float g,float b){color[0] = r;color[1]= g;color[2] = b;};
   inline float *get_color(){return color;};
   inline float *get_pos(){return pos;};
   inline float *get_halfpos(){return half_pos;};
   inline void set_Index(int i){index = i;};
   BOOL Save(LStream *p);
   BOOL Load(LStream *p,int ver);
protected:
	float pos[4],color[4],half_pos[4];
	int index;
};
//---------------------------------------------------------------------------
class LPolygon
{
public:
	LPolygon();
   ~LPolygon();
   BOOL Render();
	inline void set(u32 value){data = value;};
   inline void set_ClearID(u8 value){clearID = value;};
   enum cmdType {vertex,begin,end};
   BOOL add_Command(cmdType type,void *value);
   inline void set_Enable(BOOL value){bEnable = FALSE;};
protected:
   u32 data;
   u8 clearID;
   LVector<u32> bufferCommand;
   LVector<float> vertexs;
   LVector<GLenum> begins;
   BOOL bEnable;
};
//---------------------------------------------------------------------------
class LCommandBuffer
{
public:
	LCommandBuffer();
   ~LCommandBuffer();
   BOOL new_Object(u32 value);
   BOOL add_Command(u32 adr,u32 data,u8 am);
   BOOL draw(u32 *pobj);
   void reset();
   void swap();
   int get_ClearBuffer(){return nClearBuffer;};
   void set_ClearBuffer(int value){nClearBuffer = value;nSwap = 0;};
   void clear();
protected:
	BOOL bEnable;
   LVector<u64>com,back;
   u16 wObject;
   int nClearBuffer,nSwap;
};
#endif


