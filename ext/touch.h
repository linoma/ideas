#include "ideastypes.h"
#include "dstype.h"
//---------------------------------------------------------------------------
#ifndef __touchH__
#define __touchH__

//---------------------------------------------------------------------------
void Touch_reset();
void Touch_set(int x,int y,BOOL bDown);
void Touch_up();
void Touch_down();
void Touch_Settings();
u32 Touch_DATAO(u32 address,u8);
void Touch_CONTROLI(u32 address,u32 data,u8);
void Touch_DATAI(u32 address,u32 data,u8);
void Power_Reset();
u32 Power_ioDATAO(u32 address,u8 am);
void Power_ioDATAI(u32 addres,u32 data,u8 am);
void Power_ioCONTROLI(u32 address,u32 data,u8 am);
BOOL POWER_Save(LStream *pFile);
BOOL POWER_Load(LStream *pFile,int ver);
//---------------------------------------------------------------------------
#endif








