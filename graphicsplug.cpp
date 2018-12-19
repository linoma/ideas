#include "ideastypes.h"
#include "graphicsplug.h"
#include "pluginctn.h"
#include "resource.h"
#include "dsmem.h"
#include "gpu.h"
#include "io.h"
#include "lds.h"

//---------------------------------------------------------------------------
GraphicsPlug::GraphicsPlug() : PlugIn()
{
	dwType = PIT_GRAPHICS;
}
//---------------------------------------------------------------------------
GraphicsPlug::~GraphicsPlug()
{
}
//---------------------------------------------------------------------------
BOOL GraphicsPlug::Run(unsigned short *p,unsigned short *p1)
{
   RUNPARAM rp;

   if(!isComplex)
       return ((u32 I_STDCALL (*)(unsigned short,unsigned short *,unsigned short *))pRunFunc)(index,p,p1);
   rp.graphics.mem1 = p;
   rp.graphics.mem2 = p1;
   return PlugIn::Run(&rp);
}
//---------------------------------------------------------------------------
GraphicsPlugList::GraphicsPlugList() : PlugInList("Renderer")
{
}
//---------------------------------------------------------------------------
GraphicsPlugList::~GraphicsPlugList()
{
}
//---------------------------------------------------------------------------
BOOL GraphicsPlugList::Run(char *p,char *p1)
{
	GraphicsPlug *pPlugIn;
   DWORD dwPos,i;

   dwPos = i = 0;
   if((pPlugIn = (GraphicsPlug *)GetFirstItem(&dwPos)) ==  NULL)
       return FALSE;
   do{
		if(pPlugIn->IsEnable()){
       	pPlugIn->Run((unsigned short *)p,(unsigned short *)p1);
           i++;
       }
   }while((pPlugIn = (GraphicsPlug *)GetNextItem(&dwPos)) != NULL);
   return i ? TRUE : FALSE;
}
