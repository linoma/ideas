#include "fw.h"
#include "dsmem.h"
#include "util.h"
#include "lregkey.h"

static FIRMWARE firmware;
static u32 uOptions;
static char userName[11];
//---------------------------------------------------------------------------
void FirmWare_set_LanguageSettings(int index)
{
	if(index < 0 || index > 5)
   	index = 1;
   uOptions = (uOptions & ~7) | index;
   Firmware_Adjust();
}
//---------------------------------------------------------------------------
char *FirmWare_get_UserName()
{
   return userName;
}
//---------------------------------------------------------------------------
void FirmWare_set_UserName(const char *name)
{
   lstrcpyn(userName,name,10);
   Firmware_Adjust();
}
//---------------------------------------------------------------------------
int FirmWare_get_LanguageSettings()
{
	return (int)(uOptions & 7);
}
//---------------------------------------------------------------------------
LString Firmware_CheckLoad()
{
   LString s;
   LRegKey key;

   if(ds.get_Optimize() & NDSO_LOAD_FIRMWARE){
       if(key.Open("Software\\iDeaS\\Settings")){
           s = key.ReadString("firmware","");
           key.Close();
       }
   }
   return s;
}
//---------------------------------------------------------------------------
void FirmWare_Reset()
{
   int i;
   LFile *pFile;
	BOOL bLoad;
   LString s;

   ZeroMemory(&firmware,sizeof(FIRMWARE));
   bLoad = TRUE;
#ifdef _DEBPRO
//	ds.set_Optimize(ds.get_Optimize()|NDSO_LOAD_FIRMWARE);
#endif
   s = Firmware_CheckLoad();
#ifdef _DEBPRO
//       s = "C:\\Documents and Settings\\Lino\\Desktop\\firmware.bin";
#endif
   if(!s.IsEmpty()){
       pFile = new LFile(s.c_str());
       if(pFile != NULL){
           if(pFile->Open()){
               i = pFile->Size();
               if(i > 0x40000)
                   i = 0x40000;
               if(pFile->Read(fw_mem,i) != 0)
                   bLoad = FALSE;
           }
           delete pFile;
       }
   }
   if(bLoad){
        fw_mem[6] = 0x7a;
        fw_mem[7] = 0x2c;
        fw_mem[0x20] = 0xC0;
        fw_mem[0x21] = 0x7F;
        fw_mem[0x22] = 0xC0;
        fw_mem[0x23] = 0x7E;
        fw_mem[0x1D] = 0xFF;//Console type
        *((u32 *)&fw_mem[0x8]) = 'MACP';
        fw_mem[0x3FE00] = 5;
        fw_mem[0x3FF00] = 5;
        for(i=0;i<0x8c;i++)
            fw_mem[0x3FE74+i] = fw_mem[0x3FF74+i] = 0xFF;
        *((u16 *)&fw_mem[0x3FEB0]) = 0xFFFF;
        *((u16 *)&fw_mem[0x3FFB0]) = 0xFFFF;
        *((u16 *)&fw_mem[0x28]) = 0xFFFF;
        fw_mem[0x1D] = 0xFF;
	}
	Firmware_Adjust();
}
//---------------------------------------------------------------------------
void Firmware_Adjust()
{
   int i;
   
   *((u16 *)&fw_mem[0x3FE64]) = (u16)(uOptions & 7);
   for(i=0;i<20;i+=2){
       fw_mem[0x3FE06+i] = userName[i>>1];
       fw_mem[0x3FE06+i+1] = 0;
   }
   *((u16 *)&fw_mem[0x3FE1A]) = (u16)lstrlen(userName);
   *((u16 *)&fw_mem[0x3FE70]) = 0x70;
   *((u16 *)&fw_mem[0x3FE72]) = CalcCrc16((u16 *)&fw_mem[0x3FE00],*((u16 *)&fw_mem[0x3FE70]));
   CopyMemory(&fw_mem[0x3FF00],&fw_mem[0x3FE00],0xFF);
   if(Firmware_CheckLoad().Length() < 1){
       if(pPlugInContainer != NULL)
           pPlugInContainer->NotifyState(PIL_WIFI,PNMW_INITFIRMWARE,PIS_NOTIFYMASK,(LPARAM)fw_mem);
       *((u16 *)&fw_mem[0x2C]) = 0x138;
       *((u16 *)&fw_mem[0x2A]) = CalcCrc16((u16 *)&fw_mem[0x2c],*((u16 *)&fw_mem[0x2c]),0);
   }
/*   else{
       if(pPlugInContainer != NULL)
           pPlugInContainer->NotifyState(PIL_WIFI,PNMW_INITFIRMWARE,PIS_NOTIFYMASK,(LPARAM)0);
   }*/
   CopyMemory(&ext_mem[ulMMMask - 0x37F],&fw_mem[0x3FE00],0x6F);   //0x37F
   ext_mem[ulMMMask - 0x3BF] = 1;  
}
//---------------------------------------------------------------------------
u32 FirmWare_ioDATAO(u32 adr,u8 data)
{
   switch(firmware.command){
       case 3:
           if(--firmware.dataOut)
               return (u32)fw_mem[firmware.dataIdx++];
       break;
       case 5:
           return firmware.dataOut;
   }
   return 0;
}
//---------------------------------------------------------------------------
void FirmWare_ioDATAI(u32 adr,u32 data,u8 am)
{
	if(!firmware.dataIn){
   	if(!data)
       	return;
       firmware.command = (u16)data;
       switch(data){
       	case 0xA:
           case 0x3:
          		firmware.address = 0;
               firmware.dataIn = 3;
           break;
           case 0x5:
           	firmware.dataOut = firmware.status;
           break;
           case 6:
               firmware.status = 2;
           break;
           case 4:
               firmware.status = 0;
           break;
           default:
           	data = data;
           break;
       }
       WriteMsgConsolle(&arm7,"%cFirmware 0x%02X 0%08X %04X",MSGT_FIRMWARE,firmware.command,firmware.address,data);
   }
   else if(firmware.dataIn == 4){
   	switch(firmware.command){
       	case 0xA:
           	fw_mem[firmware.dataIdx++] = (u8)data;
           break;
       }
   }
   else if(firmware.dataIn > 0){
		firmware.dataIn--;
       firmware.address = (firmware.address << 8) | ((u8)data);
       if(firmware.dataIn == 0){
       	firmware.dataIn = -1;
        	firmware.dataOut = 1;
           firmware.dataIdx = firmware.address;
          	firmware.status = 0;
           switch(firmware.command){
           	case 0x3:
               	firmware.dataOut = 0x40000 - firmware.address;
               break;
           	case 0xA:
               	firmware.dataIn = 4;
                   firmware.dataOut = 256;
               break;
           }
           WriteMsgConsolle(&arm7,"%cFirmware 0x%02X 0%08X %04X",MSGT_FIRMWARE,firmware.command,firmware.address,data);
       }
   }
}                      //83de600a
//---------------------------------------------------------------------------
void FirmWare_ioCONTROLI(u32 adr,u32 data,u8 am)
{
   firmware.control = (u16)(data & ~0x80);
   if((firmware.control & 0x8800) == 0x8800){
//   if(!(firmware.control & 0x800)){
       firmware.address = 0;
       firmware.dataIn = 0;
   }
}

