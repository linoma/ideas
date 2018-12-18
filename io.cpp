#include "io.h"
#include "lds.h"
#include "util.h"
#include "timers.h"
#include "dma.h"
#include "cpm.h"
#include "spi.h"
#include "rtc.h"
#include "pxi.h"
#include "card.h"
#include "pluginctn.h"

//---------------------------------------------------------------------------
LPIFUNC i_func[0xB000];
LPIFUNC i_func7[0xB000];
LPIFUNC *crI;
LPOFUNC o_func[0xB000];
LPOFUNC o_func7[0xB000];
LPOFUNC *crO;
//---------------------------------------------------------------------------
void nullIOFunc(u32,u32,u8)
{
//   _EAX = _EAX;
}
//---------------------------------------------------------------------------
static void vramIOFunc(u32 adr,u32 data,u8 am)
{
   adr -= 0x240;
   do{
       MMUMainremapVRAM((u8)adr,(u8)data);
       if(pPlugInContainer != NULL)
   	    pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_CHANGEVRAMCNT,PIS_NOTIFYMASK,MAKELPARAM(adr,(u8)data));
       adr++;
       data >>= 8;
       am >>= 1;
   }while(am);
}
//---------------------------------------------------------------------------
static void PMIOFunc(u32 address,u32 data,u8 am)
{
   if(pPlugInContainer != NULL)
       pPlugInContainer->NotifyState(PIL_VIDEO,PNM_POWCNT1,PIS_NOTIFYMASK,(LPARAM)data);
   ds.OnDisplaySwap((u16)(data & 0x8000));
//   *((u16 *)(io_mem+0x304)) &= ~0x8000;
}
//---------------------------------------------------------------------------
static void stopIOFunc(u32 address,u32 data,u8 am)
{
   if(ds.get_EmuMode() != EMUM_GBA)
       arm7.w_stop((u8)(data & 0x80 ? 1 : 2));
   else
       arm7.w_stop((u8)(data & 0x80 ? 2 : 1));
}
//---------------------------------------------------------------------------
static void EXTKEYINFunc(u32 address,u32 data,u8 am)
{
   if(ds.is_Hinge())
       *((u16 *)&io_mem7[0x136]) |= 0x80;
   else
       *((u16 *)&io_mem7[0x136]) &= ~0x80;
}
//---------------------------------------------------------------------------
static void EXMEMCNT9Func(u32 address,u32 data,u8 am)
{
	u16 *p;

   p = (u16 *)&io_mem7[0x204];
	*p = (u16)((*p & 0x7F) | (*((u16 *)&io_mem[0x204]) & 0xFF80));
}
//---------------------------------------------------------------------------
static void EXMEMCNT7Func(u32 address,u32 data,u8 am)
{
   EXMEMCNT9Func(address,data & 0x7F,am);
}
//---------------------------------------------------------------------------
static void POWCNTIOFunc(u32 address,u32 data,u8 am)
{
	switch(address){
   	case 0x304:
       case 0x305:
			pPlugInContainer->NotifyState(PIL_WIFI|PIL_AUDIO,PNMW_POWCNT2,PIS_NOTIFYMASK,(LPARAM)data);
       break;
       case 0x206:
       case 0x207:
			pPlugInContainer->NotifyState(PIL_WIFI,PNMW_WIFIWAITCNT,PIS_NOTIFYMASK,(LPARAM)data);
       break;
   }
}
//---------------------------------------------------------------------------
static void ioDISPSTAT9(u32 address,u32 data,u8 am)
{
   ds.OnChangeDISPSTAT(9,io_mem,data);
//   downlcdIOFunc(address,data,am);
}
//---------------------------------------------------------------------------
static void ioDISPSTAT7(u32 address,u32 data,u8 am)
{
   ds.OnChangeDISPSTAT(7,io_mem7,data);
}
//---------------------------------------------------------------------------
void setIOTable()
{
	int i;

   for(i=0;i<0xB000;i++){
   	i_func[i] = nullIOFunc;
   	i_func7[i] = nullIOFunc;
       o_func[i] = o_func7[i] = NULL;
   }
   i_func[0x4] = i_func[0x5] = ioDISPSTAT9;
   i_func7[0x4] = i_func7[0x5] = ioDISPSTAT7;
   for(i=0x240;i<0x24a;i++)
       i_func[i] = vramIOFunc;
   i_func7[0x136] = EXTKEYINFunc;
   i_func7[0x137] = EXTKEYINFunc;

	i_func[0x204] = i_func[0x205] = EXMEMCNT9Func;
	i_func7[0x204] = i_func7[0x205] = EXMEMCNT7Func;
   i_func7[0x206] = POWCNTIOFunc;
   i_func7[0x207] = POWCNTIOFunc;
   i_func7[0x304] = POWCNTIOFunc;
   i_func7[0x305] = POWCNTIOFunc;
   i_func7[0x301] = stopIOFunc;
   for(i=0x304;i<0x306;i++)
       i_func[i] = PMIOFunc;
	InitDmaTable(i_func,i_func7,o_func,o_func7);
	InitTimersTable(i_func,i_func7,o_func,o_func7);
   arm9.InitTable(i_func,i_func7);
   arm7.InitTable(i_func,i_func7);
   InitPXITable(i_func,i_func7,o_func,o_func7);
   InitSPITable(i_func,i_func7,o_func,o_func7);
   InitRtcTable(i_func,i_func7,o_func,o_func7);
   InitCPMTable(i_func,i_func7,o_func,o_func7);
   InitCARDTable(i_func,i_func7,o_func,o_func7);
}

