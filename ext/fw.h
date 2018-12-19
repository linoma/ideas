#include "ideastypes.h"
#include "dstype.h"

//---------------------------------------------------------------------------
#ifndef __fwH__
#define __fwH__
//---------------------------------------------------------------------------
void FirmWare_Reset();
u32 FirmWare_ioDATAO(u32 adr,u8);
void FirmWare_ioDATAI(u32 adr,u32 data,u8 am);
void FirmWare_ioCONTROLI(u32 adr,u32 data,u8 am);
void Firmware_Adjust();
void FirmWare_set_LanguageSettings(int index);
int FirmWare_get_LanguageSettings();
char *FirmWare_get_UserName();
void FirmWare_set_UserName(const char *name);

#endif
 
