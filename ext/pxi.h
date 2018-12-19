#include "ideastypes.h"
#include "dstype.h"

#ifndef __PXIH__
#define __PXIH__

void InitPXITable(LPIFUNC *p,LPIFUNC *p1,LPOFUNC *p2,LPOFUNC *p3);
void PXI_reset();
BOOL PXI_Save(LStream *pFile);
BOOL PXI_Load(LStream *pFile,int ver);
#endif

