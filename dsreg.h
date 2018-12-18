#include "dsmem.h"

#ifndef __DSREGH__
#define __DSREGH__

#define GXSTAT         *((u32 *)(io_mem + 0x600))
#define DISPCAPCNT     *((u32 *)(io_mem + 0x64))
#define LISTRAM_COUNT  *((u16 *)(io_mem + 0x604))
#define VTXRAM_COUNT   *((u16 *)(io_mem + 0x606))
#define POWCNT1        *((u16 *)(io_mem + 0x304))
#define DISP3DCNT		*((u16 *)(io_mem + 0x60))

#endif








