#include "ideastypes.h"
#include "dstype.h"

#ifndef __CARDH__
#define __CARDH__

void InitCARDTable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3);
void CARD_reset();
u8 CARD_is_enable();
void OnSelectEEPROM(WORD wID);
WORD get_EEPROMID();
void EEPROM_reset();
BOOL EEPROM_Save(LStream *pFile);
BOOL EEPROM_Load(LStream *pFile,int ver);
BOOL EEPROM_Save(char *fileName);
BOOL EEPROM_Load();
BOOL EEPROM_Import(const char *fileName,WORD wID);
BOOL EEPROM_IsModified();
void EEPROM_AutoSave(BOOL bForce);

extern CARD card;
#endif
