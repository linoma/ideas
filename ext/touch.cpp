#include "ideastypes.h"
#include "touch.h"
#include "dsmem.h"
#include "lds.h"
#include "fw.h"
#include "util.h"

//---------------------------------------------------------------------------
POWERMNG pwr;
static TOUCH touch;
//d8
static u16 touch_preset[3][8]={
   {0x00FC,0x00FC,0x2020,0x0ED8,0x0F08,0xA0E,0xFC01,0x7E05},
	{0x28c,0x2f0,0x2020,0xd28,0xcb4,0xa0e0,0xfc01,0x200},
   {0x0700,0x06CC,0x2020,0x08C0,0x0914,0xA0E,0xFC01,0x7E05}
};
//---------------------------------------------------------------------------
void Touch_Settings()
{
	int value;
	u16 *p;

   value = ds.get_TouchScreenSettings();
   if(value < 0 || value > 2)
   	value = 1;
   CopyMemory(&fw_mem[0x3FF58],&touch_preset[value],sizeof(touch_preset[0]));
   CopyMemory(&fw_mem[0x3FE58],&touch_preset[value],sizeof(touch_preset[0]));
	p = &touch_preset[value][0];
	touch.xscale = (((u8)p[5] - (u8)p[2]) << 19) / (p[3] - p[0]);
   touch.yscale = ((HIBYTE(p[5]) - HIBYTE(p[2])) << 19) / (p[4] - p[1]);
	touch.xoffset = ((p[0] + p[3]) * touch.xscale  - (((u8)p[2] + (u8)p[5]) << 19)) / 2;
   touch.yoffset = ((p[1] + p[4]) * touch.yscale  - ((HIBYTE(p[2]) + HIBYTE(p[5])) << 19)) / 2;
	Firmware_Adjust();   
}
//---------------------------------------------------------------------------
void Touch_reset()
{
   touch.x = 0;
   touch.y = 0;
   touch.dwTick = (DWORD)-1;
   Touch_Settings();
}
//---------------------------------------------------------------------------
void Touch_set(int x,int y,BOOL bDown)
{
  	if(!bDown){
 		((u16 *)(io_mem7 + 0x136))[0] |= 0x40;
//   	touch.x = 0;
//   	touch.y = 255;
   	return;
   }
 	((u16 *)(io_mem7 + 0x136))[0] &= ~0x40;
   if(touch.dwTick == (DWORD)-1)
		touch.dwTick = GetTickCount();
	if(x != touch.x || y != touch.y)
		touch.dwTick = GetTickCount();
   touch.x = x;
   touch.y = y;
}
//---------------------------------------------------------------------------
void Touch_up()
{
//   touch.x = touch.y = 0;
 	((u16 *)(io_mem7 + 0x136))[0] |= 0x40;
   touch.dwTick = (DWORD)-1;
}
//---------------------------------------------------------------------------
void Touch_down()
{
   touch.dwTick = (DWORD)-1;
}
//---------------------------------------------------------------------------
u32 Touch_DATAO(u32 address,u8)
{
   if(!touch.mode){
       address = touch.dataOut << 3;  //3804e14
	    if(touch.dataIdx++ == 0)
   	    return (address & 0xFF00) >> 8;
       touch.dataIdx = 0;
	    return (u32)((u8)address);
   }
   else{
       if(touch.dataIdx++ == 0)
   	    return (u32)(touch.dataOut >> 1);
       touch.dataIdx = 0;
	    return (u32)(u8)((touch.dataOut & 1) << 7);
   }
}
//---------------------------------------------------------------------------
void Touch_CONTROLI(u32 address,u32 data,u8)
{
   touch.control = (u16)data;
   if((touch.dataIn = (u16)((data & 0x800) >> 11)))
       touch.dataOut = 0;
}
//---------------------------------------------------------------------------
void Touch_DATAI(u32 address,u32 data,u8)
{
   if(!(touch.control & 0x8000) || !(data & 0x80))
		return;
	touch.mode = (u8)((data & 8) >> 3);
  	switch((data >> 4) & 7){
  		case 0:
      		touch.dataOut = 600;
      	break;
      	case 1:
  			touch.dataOut = (u16)(((touch.y << 19) - (touch.yscale >> 1) + touch.yoffset) / touch.yscale);
      	break;
       case 2:
           touch.dataOut = 0;
       break;
      	case 3:
      		touch.dataOut = 4;
      	break;
      	case 4:
      		if(touch.dwTick != (DWORD)-1)
      			touch.dataOut = (u16)((GetTickCount() - touch.dwTick) >> 5);
      		else
      			touch.dataOut = 0;
       break;
     	case 5:
			touch.dataOut = (u16)(((touch.x << 19) - (touch.xscale >> 1) + touch.xoffset) / touch.xscale);
      	break;
       case 6:
       	AudioPlug *p;
			AudioPlugList *list;

			data = 0;
           list = (AudioPlugList *)pPlugInContainer->get_PlugInList(PIL_AUDIO);
           if(list != NULL){
				p = (AudioPlug *)list->get_ActiveMicPlugin();
               if(p != NULL)
               	data = p->Run(FALSE);
           }
           touch.dataOut = (u16)data;
       break;
      	case 7:
			touch.dataOut = 744;
      	break;
   }
   touch.command = (u16)((data >> 4) & 7);
}
//---------------------------------------------------------------------------
void Power_Reset()
{
	ZeroMemory(&pwr,sizeof(POWERMNG));
   pwr.regValue[0] = 0xF;
   pwr.regValue[1] = 0;
   pwr.regValue[2] = 0;
   pwr.regValue[3] = 1;
}
//---------------------------------------------------------------------------
u32 Power_ioDATAO(u32 address,u8 am)
{
   return pwr.regValue[pwr.reg & 7];
}
//---------------------------------------------------------------------------
void Power_ioDATAI(u32 addres,u32 data,u8 am)
{
   if(pwr.control & 0x800)
       pwr.reg = (u8)data;
   else if(!(pwr.reg & 0x80)){
       if((pwr.reg & 7) == 1)
           pwr.regValue[1] = 0;
       else{
           pwr.regValue[pwr.reg & 7] = (u8)data;
           switch(pwr.reg & 7){
               case 0:
               	upLcd.set_StatusLed(((data & 0x30) >> 4));
               default:
                   pPlugInContainer->NotifyState(PIL_VIDEO|PIL_AUDIO,PNMP_POWERCNT,PIS_NOTIFYMASK,(LPARAM)MAKELPARAM((pwr.reg & 7),data));
               break;
           	case 2:
                   pPlugInContainer->NotifyState(PIL_AUDIO,PNMP_POWERCNT,PIS_NOTIFYMASK,(LPARAM)MAKELPARAM(2,data));
               break;
               case 3:
                   pPlugInContainer->NotifyState(PIL_AUDIO,PNMP_POWERCNT,PIS_NOTIFYMASK,(LPARAM)MAKELPARAM(3,data));
               break;
           }
       }
   }
   WriteMsgConsolle(&arm7,"%cWrite Power Management Reg : %d Value : 0x%02X",MSGT_POWER,(int)(pwr.reg & 7),data);   
}
//---------------------------------------------------------------------------
void Power_ioCONTROLI(u32 address,u32 data,u8 am)
{
   pwr.control = (u16)data;
}
//---------------------------------------------------------------------------
BOOL POWER_Save(LStream *pFile)
{
   pFile->Write(&pwr,sizeof(POWERMNG));
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL POWER_Load(LStream *pFile,int ver)
{
   if(ver > 48)
       pFile->Read(&pwr,sizeof(POWERMNG));
   pPlugInContainer->NotifyState(PIL_AUDIO,PNMP_POWERCNT,PIS_NOTIFYMASK,(LPARAM)MAKELPARAM(2,pwr.regValue[2]));
   pPlugInContainer->NotifyState(PIL_AUDIO,PNMP_POWERCNT,PIS_NOTIFYMASK,(LPARAM)MAKELPARAM(3,pwr.regValue[3]));
   return TRUE;
}

