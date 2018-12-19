#include "ideastypes.h"
#include "cpu_0.h"

//---------------------------------------------------------------------------
#ifndef __SYSCNTH__
#define __SYSCNTH__
//---------------------------------------------------------------------------
struct _sysrgn
{
	u32 address_start,address_end;
   u8 cnt,ap[4];
   u32 size;
};
typedef struct _sysrgn ISYSRGN;
typedef struct _sysrgn *LPSYSRGN;
//---------------------------------------------------------------------------
typedef struct _syscnt
{
	struct _sysrgn rgn[8];
   u32 itcm_address,dtcm_address,itcm_end,dtcm_end,itcm_pu_rgn_start,itcm_pu_rgn_end;
   u32 itcm_size,dtcm_size;
   u32 irq_address;
	u32 reg1,reg5[4],reg0;
   u8 dtcm_enable,itcm_enable;
   u32 lastAddress,lastAddressCount;
   s32 loop;
} SYSCNT,*LPSYSCNT;
//---------------------------------------------------------------------------
void syscnt_write(u32 opcode);
u32 syscnt_read(u32 opcode);
void syscnt_reset();
u8 I_FASTCALL syscnt_can_operate(u32 address,u8 oper);
BOOL syscnt_Save(LStream *pFile);
BOOL syscnt_Load(LStream *pFile,int ver);

//u32 I_FASTCALL syscnt_remap_tcm(u32 address,u8 oper);

#if defined(_DEBPRO)
void syscnt_show_reg9(TV_ITEM *p,HWND hwndTV);
void syscnt_show_reg1(TV_ITEM *p,HWND hwndTV);
void syscnt_show_reg6(TV_ITEM *p,HWND hwndTV);
void syscnt_show_reg5(TV_ITEM *p,HWND hwndTV);
#endif
//---------------------------------------------------------------------------
extern SYSCNT sysCnt;
//---------------------------------------------------------------------------
#endif
