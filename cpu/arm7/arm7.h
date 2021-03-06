#include "ideastypes.h"
#include "lvec.hpp"
#include "cpu_0.h"

//---------------------------------------------------------------------------
#ifndef arm7H
#define arm7H
//---------------------------------------------------------------------------
typedef struct _arm7 : public IARM7
{
   u8 r_stop();
   void w_stop(u8 value);
   u8 r_cf();
   void w_cf(u8 value);
   u8 r_nf();
   void w_nf(u8 value);
   u8 r_zf();
   void w_zf(u8 value);
   u8 r_vf();
   void w_vf(u8 value);
   u8 r_qf();
   void w_qf(u8 value);
   LVector<unsigned long> *r_callstack();
   void decode_ins(u8 mode,u32 adress, char *dest);
   u32 r_opcode();
	u32 r_gpreg(int index);
   void w_gpreg(int index,u32 value);
   BOOL Init();
   void Reset();
   void InitPipeLine(u32 offset);
	u32 r_cpsr();
	void w_cpsr(u32 value);
   inline int r_index(){return 7;};
   LPMEMORYINFO r_memmap();
   inline int r_memmapsize();
   u32 r_modereg(int mode,int index);
   u32 remap_address(u32 address);
	u32 read_mem(u32 address,u8 mode);
   void write_mem(u32 address,u32 data,u8 mode);
   BOOL Save(LStream *pFile);
   BOOL Load(LStream *pFile,int ver);
   BOOL EnableCheats(void **func);
   void InitTable(LPIFUNC *p,LPIFUNC *p1);
   u32 get_CurrentPC();
   u32 get_Stopped();
   void set_Stopped();
   int enter_dmaloop();   
} CPU_ARM7;

void arm7irq(void);
extern u16 (*exec7)(void);
extern u32 nIRQ7;
extern void (*pfn_arm7irq)(void);
#endif
 
