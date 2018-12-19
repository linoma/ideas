#include "ideastypes.h"
#include "spi.h"
#include "touch.h"
#include "fw.h"
#include "util.h"

static u16 Control;
//---------------------------------------------------------------------------
static u32 ioDATAO(u32 address,u8 am)
{
   switch((Control >> 8) & 3){
      	case 2:
           return Touch_DATAO(address,am);
       case 1:
           return FirmWare_ioDATAO(address,am);
       case 0:
           return Power_ioDATAO(address,am);
       default:
           return 0;
   }
}
//---------------------------------------------------------------------------
static u32 ioCONTROLO(u32 address,u8)
{
   return (u32)Control;
}
//---------------------------------------------------------------------------
static void ioDATAI(u32 address,u32 data,u8 am)
{
   switch((Control >> 8) & 3){
      	case 2:
          	Touch_DATAI(address,data,am);
       break;
       case 1:
           FirmWare_ioDATAI(address,data,am);
       break;
       case 0:
           Power_ioDATAI(address,data,am);
       break;
       case 3:
       break;
   }
}
//---------------------------------------------------------------------------
static void ioCONTROLI(u32 address,u32 data,u8 am)
{
   WriteMsgConsolle(&arm7,"%cSPI Device : %d Value : 0x%04X",MSGT_SPI,(int)((Control >> 8) & 3),data);
   Control = (u16)data;
   switch((Control >> 8) & 3){
       case 0:
           Power_ioCONTROLI(address,data,am);
       break;
       case 1:
           FirmWare_ioCONTROLI(address,data,am);
       break;
      	case 2:
          	Touch_CONTROLI(address,data,am);
       break;
       case 3:
           data = data;
       break;
   }
   if(am == AMM_WORD)
       ioDATAI(address,(u32)(data >> 16),AMM_WORD);
}
//---------------------------------------------------------------------------
void InitSPITable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3)
{
   p1[0x1c0] = ioCONTROLI;
   p3[0x1c0] = ioCONTROLO;
   p1[0x1C2] = ioDATAI;
   p3[0x1C2] = ioDATAO;
}

