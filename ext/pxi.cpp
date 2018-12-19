#include "ideastypes.h"
#include "lds.h"
#include "util.h"

static PXI pxi9,pxi7;
//---------------------------------------------------------------------------
void PXI_reset()
{
	pxi9.control = 0;
   pxi9.data = 0;
   pxi9.index = 0;
	pxi7.control = 0;
   pxi7.data = 0;
   pxi7.index = 0;
}
//---------------------------------------------------------------------------
static void iCONTROL7(u32 adr,u32 data,u8 am)
{
   if(data & 8)
       pxi7.control |= 1;
   pxi7.control = (u16)((pxi7.control & 0x303)|(data & ~0x430B));
   //Fix me
   if((data & 4)){
       if(pxi7.index == 0){
           pxi7.control |= 1;//Send Empty
           ds.set_IRQ(0x20000,FALSE,2);
       }
   }
}
//---------------------------------------------------------------------------
static u32 oCONTROL7(u32 adr,u8 am)
{
   return pxi7.control;
}
//---------------------------------------------------------------------------
static void iCONTROL9(u32 adr,u32 data,u8 am)
{
   if(data & 8)
       pxi9.control |= 1;
   pxi9.control = (u16)((pxi9.control & 0x303)|(data & ~0x430B));
   //Fix me
   if((data & 4)){
       if(pxi9.index == 0){
           pxi9.control |= 1;//Send Empty
           ds.set_IRQ(0x20000,FALSE,1);
       }
   }
}
//---------------------------------------------------------------------------
static u32 oCONTROL9(u32 adr,u8 am)
{
   return pxi9.control;
}
//---------------------------------------------------------------------------
static void iID9(u32 adr,u32 data,u8 am)
{
   pxi9.data = (u16)data;
   if((pxi9.data & 0x4000))
		ds.set_IRQ(0x10000,FALSE,2);
}
//---------------------------------------------------------------------------
static void iID7(u32 adr,u32 data,u8 am)
{
   pxi7.data = (u16)data;
   if((pxi7.data & 0x4000))
		ds.set_IRQ(0x10000,FALSE,1);
}
//---------------------------------------------------------------------------
static u32 oID9(u32 adr,u8 am)
{
   return (u32)((pxi9.data & 0xFFF0) | ((pxi7.data >> 8) & 0xF));
}
//---------------------------------------------------------------------------
static u32 oID7(u32 adr,u8 am)
{
   return (u32)((pxi7.data & 0xFFF0) | ((pxi9.data >> 8) & 0xF));
}
//---------------------------------------------------------------------------
static u32 oReceive9(u32 adr,u8 am)
{
//   if(pxi9.index == 0)
//       return 0;
   adr = pxi9.buffer[0];
   for(int i = 1;i<16;i++)
       pxi9.buffer[i-1] = pxi9.buffer[i];
   pxi9.index--;
   pxi9.index &= 15;
   if(pxi9.index == 0){
       pxi9.control |= 0x100;
       pxi7.control |= 1;//Send Empty
       if(pxi9.control & 4)
           ds.set_IRQ(0x20000,FALSE,1);
   }
   pxi7.control &= ~2;
   //   if(adr == 0x40000f || adr == 0x8E5E480A)
       //   	EnterDebugMode(FALSE);
   return adr;
}
//---------------------------------------------------------------------------
static void iSend9(u32 adr,u32 data,u8 am)
{
   WriteMsgConsolle(&arm9,"%cPXI ARM9 0x%08X",MSGT_PXI9,data);
   pxi7.control &= ~0x100;
   pxi7.buffer[pxi7.index++] = data;
   pxi7.index &= 15;
   pxi9.control &= ~1;//Send not empty
   if(pxi7.control & 0x400)
       ds.set_IRQ(0x40000,FALSE,2);
}
//---------------------------------------------------------------------------
static u32 oReceive7(u32 adr,u8 am)
{
	adr = pxi7.buffer[0];
   for(int i = 1;i<16;i++)
		pxi7.buffer[i-1] = pxi7.buffer[i];
   pxi7.index--;
   pxi7.index &= 15;
	if(pxi7.index == 0){
   	pxi7.control |= 0x100;
       pxi9.control |= 1;//Send Empty
       if(pxi7.control & 4)
           ds.set_IRQ(0x20000,FALSE,2);
   }
   pxi9.control &= ~2;
//	if(adr == 0x0040000F)
//   	EnterDebugMode(TRUE);
   return adr;
}
//---------------------------------------------------------------------------
static void iSend7(u32 adr,u32 data,u8 am)
{
   WriteMsgConsolle(&arm7,"%cPXI ARM7 0x%08X",MSGT_PXI7,data);
   pxi9.control &= ~0x100;
   pxi9.buffer[pxi9.index++] = data;
 	pxi9.index &= 15;
   pxi7.control &= ~1;//Send not empty
   if(pxi9.control & 0x400)
       ds.set_IRQ(0x40000,FALSE,1);
}
//---------------------------------------------------------------------------
void InitPXITable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3)
{
   int i;

   for(i=0x180;i<0x182;i++){
       p[i] = iID9;
       p1[i] = iID7;
       p2[i] = oID9;
       p3[i] = oID7;
   }
   for(i=0x184;i<0x186;i++){
       p[i] = iCONTROL9;
       p1[i] = iCONTROL7;
       p2[i] = oCONTROL9;
       p3[i] = oCONTROL7;
   }
   p[0x188] = iSend9;
   p1[0x188] = iSend7;
   p2[0x2000] = oReceive9;
   p3[0x2000] =  oReceive7;
}
//---------------------------------------------------------------------------
BOOL PXI_Save(LStream *pFile)
{
   pFile->Write(&pxi7,sizeof(pxi7));
   pFile->Write(&pxi9,sizeof(pxi9));
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL PXI_Load(LStream *pFile,int ver)
{
   pFile->Read(&pxi7,sizeof(pxi7)); //1744
   pFile->Read(&pxi9,sizeof(pxi9)); //1672
   return TRUE;
}
