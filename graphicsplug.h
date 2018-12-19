#include "ideastypes.h"
#include "plugin.h"

//---------------------------------------------------------------------------
#ifndef __GRAPHICSPLUGINH__
#define __GRAPHICSPLUGINH__

class GraphicsPlug : public PlugIn
{
public:
	GraphicsPlug();
   virtual ~GraphicsPlug();
   BOOL Run(unsigned short *p,unsigned short *p1);
protected:
};
//---------------------------------------------------------------------------
class GraphicsPlugList : public PlugInList
{
public:
	GraphicsPlugList();
   virtual ~GraphicsPlugList();
   BOOL Run(char *p,char *p1);
protected:

};
#endif

