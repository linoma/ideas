#include "dstype.h"

//---------------------------------------------------------------------------
#ifndef __cbdsH__
#define __cbdsH__
//---------------------------------------------------------------------------

extern const u8 CBSData[];
BOOL encrypt_arm9(u32 cardheader_gamecode, unsigned char *data);
BOOL decrypt_arm9(u32 cardheader_gamecode, unsigned char *data);

#endif

 