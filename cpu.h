#include "ideastypes.h"
#include "arm7.h"
#include "arm9.h"
#include "syscnt.h"

//---------------------------------------------------------------------------
#ifndef cpuH
#define cpuH
//---------------------------------------------------------------------------

class LCPU
{
public:
	LCPU();
   ~LCPU();
   void Reset();
   BOOL Init();
protected:
};

extern CPU_ARM7 arm7;
extern CPU_ARM9 arm9;

#endif

