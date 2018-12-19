#include "plugin.h"
#include "videoplug.h"
#include "graphicsplug.h"
#include "audioplug.h"
#include "wifiplugin.h"
#include "inputplug.h"
#include "dldiplugin.h"

#ifndef __pluginctnH__
#define __pluginctnH__

#define PIL_AUDIO		2
#define PIL_VIDEO		4
#define PIL_GRAPHICS	8
#define PIL_INPUT		16
#define PIL_WIFI		64
#define PIL_DLDI		128
#define PIL_WEBCAM		256

//---------------------------------------------------------------------------
class PlugInContainer : public IPlugInManager
{
public:
   PlugInContainer();
   virtual ~PlugInContainer();
   BOOL Init(DWORD flags);
   void Destroy();
   void NotifyState(DWORD item,DWORD dwState,DWORD dwStateMask = 0,LPARAM lParam = (LPARAM)-1);
   void Reset();
   inline int GetLastError(){return iLastError;};
   PlugInList *get_PlugInList(DWORD type);
   BOOL SaveState(LStream *p);
   BOOL LoadState(LStream *p,int ver);
   PlugInList *Add(PlugIn *p);
   BOOL InitSetPlugInInfo(PlugIn *plugin,LPSETPLUGININFO p,DWORD dwState = 0,DWORD dwStateMask = 0);
   PlugIn *get_PlugInFromGUID(GUID *guid);
   //IPlugInManager Methods
   int NotifyState(DWORD items,LPVOID ns);
   int get_PlugInFormat(DWORD type,LPVOID object,LPDWORD dwSize);
   int get_PlugInInterface(GUID *guid,LPVOID *ppvObject);
   int get_ActivePlugIn(int pid,LPVOID *ppvObject);
   BOOL Create(DWORD flags);
protected:
   int iLastError;
   VideoPlugList *pVideoPlugList;
   AudioPlugList *pAudioPlugList;
   GraphicsPlugList *pGraphicsPlugList;
   LWFPlugList *pWifiPlugList;
   LInputPlugList *pInputPlugList;
   DLDIPlugList *pDLDIPlugList;
   WebCamPlugList *pWebCamPlugList;
};

extern PlugInContainer *pPlugInContainer;

#endif
