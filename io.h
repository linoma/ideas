#include "ideastypes.h"
#include "dsmem.h"

//---------------------------------------------------------------------------
#ifndef __ioH__
#define __ioH__
//---------------------------------------------------------------------------
void setIOTable();
void nullIOFunc(u32,u32,u8);

extern LPIFUNC i_func[0xB000];
extern LPIFUNC i_func7[0xB000];
extern LPIFUNC *crI;

extern LPOFUNC o_func[0xB000];
extern LPOFUNC o_func7[0xB000];
extern LPOFUNC *crO;

#endif
 
