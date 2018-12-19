#ifndef __PLUGINMAINH__
#define __PLUGINMAINH__

#ifndef PURE
	#define PURE = 0
#endif

#ifndef __DSTYPEH__
	typedef unsigned char u8;
	typedef unsigned short u16;
	typedef unsigned long u32;
	typedef signed char s8;
	typedef signed short s16;
	typedef signed long s32;
#if defined(__BORLANDC__) || defined(__WATCOMC__)
   typedef signed __int64 s64;
   typedef unsigned __int64 u64;
#else
   typedef signed long long int s64;
   typedef unsigned long long int u64;
#endif

#endif

#ifndef AMM_ALL
	#define AMM_BYTE	1
	#define AMM_HWORD 	2
	#define AMM_WORD	8
	#define AMM_READ	0x10
   #define AMM_WRITE	0x20
   #define AMM_CNT		0x40
  	#define AMM_ALL		0xff
#endif

#ifndef I_FASTCALL
	#ifdef __GNUC__
      	#define I_FASTCALL __attribute__ ((regparm(2)))
  	#elif defined(__WATCOMC__)
  		#define I_FASTCALL __watcall
   #else
       #define I_FASTCALL __fastcall
   #endif
#endif

#ifndef I_STDCALL
	#ifdef __GNUC__
		#define I_STDCALL __attribute__ ((stdcall))
	#else
		#define I_STDCALL __stdcall
	#endif
#endif

#ifndef I_CDECL
	#ifdef __GNUC__
		#define I_CDECL __attribute__ ((cdecl))
	#else
		#define I_CDECL __cdecl
	#endif
#endif

#ifndef I_EXPORT
	#ifdef __WIN32__
		#define I_EXPORT __declspec(dllexport)
	#else
		#define I_EXPORT
	#endif
#endif

#ifdef _MSC_VER
	#define PFN_I_STDCALL(ret,name,param) ret (I_STDCALL *name)param
	#define TYPEDEF_I_STDCALL(ret,name,param) typedef PFN_I_STDCALL(ret,name,param)

	#define PFN_I_FASTCALL(ret,name,param) ret (I_FASTCALL *name)param
	#define TYPEDEF_I_FASTCALL(ret,name,param) typedef PFN_I_FASTCALL(ret,name,param)
#else
	#define PFN_I_STDCALL(ret,name,param) ret I_STDCALL (*name)param
	#define TYPEDEF_I_STDCALL(ret,name,param) typedef PFN_I_STDCALL(ret,name,param)

	#define PFN_I_FASTCALL(ret,name,param) ret I_FASTCALL (*name)param
	#define TYPEDEF_I_FASTCALL(ret,name,param) typedef PFN_I_FASTCALL(ret,name,param)
#endif

#ifndef __LStream__
#define __LStream__
struct LStream
{
public:
	virtual BOOL I_CDECL Open(DWORD dwStyle = GENERIC_READ,DWORD dwCreation = OPEN_EXISTING,DWORD dwFlags = 0) PURE;
   virtual void I_CDECL Close() PURE;
   virtual DWORD I_CDECL Read(LPVOID lpBuffer,DWORD dwBytes) PURE;
   virtual DWORD I_CDECL Write(LPVOID lpBuffer,DWORD dwBytes) PURE;
   virtual DWORD I_CDECL Seek(LONG dwDistanceToMove = 0,DWORD dwMoveMethod = FILE_BEGIN) PURE;
   virtual BOOL I_CDECL SeekToBegin() PURE;
   virtual BOOL I_CDECL SeekToEnd() PURE;
   virtual DWORD I_CDECL Size(LPDWORD lpHigh = NULL) PURE;
   virtual BOOL I_CDECL SetEndOfFile(DWORD dw) PURE;
   virtual DWORD I_CDECL GetCurrentPosition() PURE;
	virtual BOOL I_CDECL IsOpen() PURE;
   virtual void I_CDECL Release() PURE;
};
#endif

#ifndef __IWnd__
#define __IWnd__

#define IID_DOWNLCD   1
#define IID_UPLCD     2

struct IWnd
{
public:
	virtual HDC I_CDECL DC() PURE;
   virtual HWND I_CDECL Handle() PURE;
   virtual void I_CDECL Release() PURE;
   virtual BOOL I_CDECL Create() PURE;
};

#endif

#ifndef __IFat__
#define __IFat__

#define IID_IFAT   10

struct IFat
{
public:
	virtual DWORD I_CDECL Read(DWORD,BYTE) PURE;
   virtual bool I_CDECL Seek(DWORD,BYTE) PURE;
   virtual IFat * I_CDECL Create(const char *) PURE;
   virtual void I_CDECL Release() PURE;
   virtual int I_CDECL get_Path(char *,int) PURE;
};
#endif

#ifndef __IPlugInInterface__
#define __IPlugInInterface__

#define OID_IO_MEMORY9		    1
#define OID_IO_MEMORY7		    2
#define OID_VRAM_MEMORY	    3
#define OID_PALETTE_MEMORY	    4
#define OID_OAM_MEMORY		    5
#define OID_EXTERN_WRAM	    6
#define OID_FIRMWARE_MEMORY    7
#define OID_READ_BYTE		    10
#define OID_READ_HWORD		    11
#define OID_READ_WORD		    12
#define OID_RUNFUNC		    13
#define OID_POWERMNG           14

struct IPlugInInterface
{
public:
   virtual int I_CDECL get_Format(DWORD,LPVOID,LPDWORD) PURE;
   //Address,Input Func,OutPut Func
   //Input Func void I_CDECL (u32 adr,u32 data,u8 accessMode)
   //Output Func u32 I_CDECL (u32 adr,u8 accessMode)
   virtual int I_CDECL WriteTable(DWORD,LPVOID,LPVOID) PURE;
   virtual int I_CDECL ResetTable() PURE;
   virtual int I_CDECL TriggerIRQ() PURE;
   virtual int I_CDECL get_Object(int,LPVOID *) PURE;
};

#endif


#ifndef __IPlugInManager__
#define __IPlugInManager__

#define IID_IPLUGINMANAGER     3

typedef struct
{
   u32 type;
   u16 index;
   union{
       struct {
           void *mem1;
           u32 len1;
           void *mem2;
           u32 len2;
           int syncro;
       } audio;
       struct {
           u16 *mem1;
           u16 *mem2;
           int render;
       } graphics;
       struct {
           int x;
           int y;
   	} render;
       struct {
           u32 *value;
       } pad;
   };
} NOTIFYCONTENT,*LPNOTIFYCONTENT;

struct IPlugInManager
{
public:
   virtual int I_CDECL NotifyState(DWORD,LPVOID) PURE;
   virtual int I_CDECL get_PlugInFormat(DWORD,LPVOID,LPDWORD) PURE;
   virtual int I_CDECL get_PlugInInterface(GUID *,LPVOID *) PURE;
   virtual int I_CDECL get_ActivePlugIn(int,LPVOID *) PURE;
};

#endif

#ifndef __INDS__
#define __INDS__

#ifndef PT_SAVEGAME

#define PT_SAVEGAME	1
#define PT_SAVESTATE	2
#define PT_SCREENSHOT	3
#define PT_CHEAT		4

#endif

struct INDS
{
public:
   virtual int I_CDECL QueryInterface(int,LPVOID *) PURE;
   virtual int I_CDECL get_FramesCount(LPDWORD) PURE;
   virtual int I_CDECL get_CurrentFileName(char *,int *) PURE;
   virtual int I_CDECL CreateFileName(int,char *,int *,char *,int) PURE;
   virtual int I_CDECL WriteConsole(char *) PURE;
   virtual int I_CDECL get_IsHomebrew(int *) PURE;
   virtual int I_CDECL get_EmulatorMode(int *) PURE;
   virtual int I_CDECL get_NumberOfCores(LPDWORD) PURE;
   virtual int I_CDECL get_DutyCycles(LPDWORD) PURE;
};
#endif

/*****************************************************************************************
* Type Request Info
*****************************************************************************************/
#define PIR_PLUGIN                     0
#define PIR_FORMAT                     1
#define PIR_STATUS                     2
/*****************************************************************************************
* Type
*****************************************************************************************/
#define PIT_AUDIO              	    1	//Audio
#define PIT_VIDEO              	    2	//Video 3d e 2d
#define PIT_GRAPHICS				    3
#define PIT_PAD					    4	//Input Controller
#define PIT_TOUCH					    5	//Touch not implemented
#define PIT_WIFI					    6	//Wifi
#define PIT_DLDI						7 	//DLDI PlugIn
#define PIT_WEBCAM						8
/*****************************************************************************************
* Flags of plugin
*****************************************************************************************/
#define PIT_DYNAMIC            	    0x100	//The plugin is loaded only in memory when enabled otherwise it is resident.
#define PIT_NOEXCLUSIVE        	    0x200	//If enabled it doesn't exclude the other pluginses of the group.
#define PIT_HOOKBEFORE                 0x400   //The plugin is a hook.
#define PIT_NOMODIFY           	    0x800	//It doesn't modify the content of the sent buffer.
#define PIT_HOOKAFTER                  0x1000  //The plugin is a hook.
#define PIT_IS3D					    0x2000	//It is a plugin for 3d graphics.
#define PIT_ISMICROPHONE				0x2000	//It is a plugin for microphone.
#define PIT_ENABLERUN		   		    0x4000	//iDeaS call the function RunFunc when it's enabled.
#define PIT_NOMENU			   		    0x8000	//It is not included in the menu of the group
#define PIT_NORUN   				    0x10000	//
#define PIT_ISFILTER                   0x20000	//It's a plugin for graphics.
#define PIT_SLOT2						0x2000
/*****************************************************************************************
* State of plugin
*****************************************************************************************/
#define PIS_ENABLE             	    0x1
#define PIS_RUN                	    0x2
#define PIS_PAUSE              	    0x4
/*****************************************************************************************
* Notify Message of plugin
*****************************************************************************************/
#define PNM_ENDLINE                    0x100
#define PNM_ENDFRAME                   0x200
#define PNM_ENTERVBLANK                0x300
#define PNM_POWCNT1                    0x400
#define PNM_STARTFRAME					0x500
#define PNM_ONACTIVATEWINDOW			0x600
#define PNM_COUNTFRAMES                0x700
#define PNM_OPENFILE                   0x800
#define PNM_INCREASECPUSPEED			0x900
#define PNM_INITTABLE					0xA00
#define PNM_CHANGECONFIG				0xB00

#define PNMW_INITFIRMWARE              0x4000
#define PNMW_POWCNT2					0x4100
#define PNMW_WIFIWAITCNT				0x4200
#define PNMV_CHANGEVRAMCNT             0x4000
#define PNMV_GETBUFFER                 0x4100
#define PNMV_GETTEXTUREINFO            0x4200
#define PNMV_GETTEXTURE                0x4300
#define PNMV_GETSTACK                  0x4400
#define PNMV_ENABLELOG                 0x4500
#define PNMV_DRAWPOLYGON				0x4600
#define PNMV_CHANGEVCOUNT				0x4700
#define PNMV_SHOWLAYERS				0x4800
#define PNMV_PALETTECHANGED			0x4900
#define PNMV_OAMCHANGED				0x4A00
#define PNMV_CLEARTEXTUREBUFFER		0x4B00
#define PNMV_CAPTURETEXTURES			0x4C00
#define PNMP_POWERCNT					0x4D00

#define PNMC_CONTENTCHANGE             0x8000
#define PNMC_AUDIOCHANGE               (PNMC_CONTENTCHANGE|(PIT_AUDIO << PIS_NOTIFYSHIFT))
#define PNMC_FORMATCHANGE              0x9000
#define PNMC_FORMATAUDIOCHANGE         (PNMC_FORMATCHANGE|(PIT_AUDIO << PIS_NOTIFYSHIFT))

/*****************************************************************************************
* State mask of plugin
*****************************************************************************************/
#define PIS_RUNMASK            	    0x6
#define PIS_ENABLEMASK         	    0x1
#define PIS_NOTIFYMASK				    0x0000FF00
#define PIS_NOTIFYSHIFT                0x8
/*****************************************************************************************
* Macros
*****************************************************************************************/
#define PLUGINISENABLE(p)      	    ((p->dwState & PIS_ENABLE) && (p->dwStateMask & PIS_ENABLEMASK))
#define PLUGINISSTOP(p)        	    (!(p->dwState & PIS_RUN)&& (p->dwStateMask & PIS_RUNMASK))
#define PLUGINISRUN(p)         	    ((p->dwState & PIS_RUN) && (p->dwStateMask & PIS_RUNMASK))
#define PLUGINISPAUSE(p)       	    ((p->dwState & PIS_PAUSE) && (p->dwStateMask & PIS_RUNMASK))
/*****************************************************************************************
* Macros
*****************************************************************************************/
#define PIR_DLDI_NULL					    0
#define PIR_DLDI_READ	    			    3
#define PIR_DLDI_WRITE		    		    4
#define PIR_DLDI_CONTROL				    5
#define PIR_DLDI_ERROR					    0xFFFF
#define PIR_DLDI_UNSUPPORTED			    0xFFFE

#define PIR_DLDI_ONLYDECODE				0x80000000
#define PIR_DLDI_ADJUSTIOCONTROL           0x40000000
#define PIR_DLDI_TRIGGERIRQ                0x20000000
#define PIR_DLDI_DATAISVALID               0x10000000
#define PIR_DLDI_ADJUSTIOCONTROL_CHECK     0x08000000
#define PIR_DLDI_FAT                       0x04000000
#define PIR_DLDI_SETLENGTH                 0x02000000
#define PIR_DLDI_SEEK                      0x01000000
#define PIR_DLDI_TRIGGERIRQ_MC				0x00800000
//---------------------------------------------------------------------------
// Struct for GetInfoFunc
//---------------------------------------------------------------------------
typedef struct _get_pluginfo{
   u32 cbSize;
   u32 dwType;
   u16 wIndex;
   u16 wType;
   wchar_t* pszText;
   s32 cchTextMax;
   GUID guidID;
   u32 dwLanguageID;
   u32 lParam;
} GETPLUGININFO,*LPGETPLUGININFO;
//---------------------------------------------------------------------------
// Struct for SetInfoFunc
//---------------------------------------------------------------------------
typedef struct _set_pluginfo{
   u32 cbSize;
   u16 wIndex;
   u32 dwState;
   u32 dwStateMask;
   u32 lParam;
   INDS *lpNDS;
} SETPLUGININFO,*LPSETPLUGININFO;
//---------------------------------------------------------------------------
// Struct for SetPropertyFunc
//---------------------------------------------------------------------------
typedef struct {
   void* hwndOwner;
   GETPLUGININFO info;
} SETPROPPLUGIN,*LPSETPROPPLUGIN;
//---------------------------------------------------------------------------
// Struct for Video 3D plugin
//---------------------------------------------------------------------------
typedef struct {
 	u8  *video_mem;						//Pointer at vram memory
	u8  *io_mem;						//Pointer at IO memory
	IWnd *lpWnd;                        //Deprecated
	PFN_I_STDCALL(void,pfn_writetable,(u32,void *,void *));
   PFN_I_STDCALL(void,pfn_onirq,(void));
} VIDEO3DPARAM,*LPVIDEO3DPARAM;
//---------------------------------------------------------------------------
// Struct for Sound plugin
//---------------------------------------------------------------------------
typedef struct {
   u8  *io_mem;						//Pointer at IO memory
  	PFN_I_FASTCALL(u32,pfn_rb,(u32));
	PFN_I_FASTCALL(u32,pfn_rhw,(u32));
	PFN_I_FASTCALL(u32,pfn_rw,(u32));
   PFN_I_STDCALL(void,pfn_writetable,(u32,void *,void *));
} SOUNDPARAM,*LPSOUNDPARAM;
//---------------------------------------------------------------------------
//Struct for WiFi plugin
//---------------------------------------------------------------------------
typedef struct {
	u8 *io_mem;
	PFN_I_STDCALL(void,pfn_writetable,(u32,void *,void *));
   PFN_I_STDCALL(void,pfn_onirq,(void));
} WIFIPARAM,*LPWIFIPARAM;
//---------------------------------------------------------------------------
//Struct for DLDI plugin
//---------------------------------------------------------------------------
typedef struct {
   u8 *io_mem;
	IFat *lpFat;
} DLDIPARAM,*LPDLDIPARAM;

//---------------------------------------------------------------------------
TYPEDEF_I_STDCALL(u32,LPPLUGINRESET,());
TYPEDEF_I_STDCALL(u32,LPPLUGINDELETE,());
TYPEDEF_I_STDCALL(u32,LPPLUGINGETINFO,(LPGETPLUGININFO));
TYPEDEF_I_STDCALL(u32,LPPLUGINSETINFO,(LPSETPLUGININFO));
TYPEDEF_I_STDCALL(u32,LPPLUGINSETPROPERTY,(LPSETPROPPLUGIN));
TYPEDEF_I_STDCALL(u32,LPPLUGINSAVESTATE,(LStream *));
TYPEDEF_I_STDCALL(u32,LPPLUGINLOADSTATE,(LStream *,int));
//---------------------------------------------------------------------------
typedef NOTIFYCONTENT RUNPARAM;
typedef LPNOTIFYCONTENT LPRUNPARAM;
//---------------------------------------------------------------------------
TYPEDEF_I_STDCALL(u32,LPINPUTPLUGINRUN,(u32 *));
TYPEDEF_I_STDCALL(u32,LPSOUNDPLUGINRUN,(void));
TYPEDEF_I_STDCALL(u32,LP3DPLUGINRUN,(u32,u32));
TYPEDEF_I_STDCALL(u32,LPDLDIPLUGINRUN,(u16 index,u32 adr,u32 *data,u8 am));
TYPEDEF_I_STDCALL(u32,LPGRAPHICSPLUGINRUN,(u16,u16 *,u16 *));
TYPEDEF_I_STDCALL(u32,LPDEFAULTPLUGINRUN,(LPRUNPARAM));
typedef LPDEFAULTPLUGINRUN LP2DPLUGINRUN;

TYPEDEF_I_FASTCALL(u32,LPREADBYTE,(u32));
TYPEDEF_I_FASTCALL(u32,LPREADHWORD,(u32));
TYPEDEF_I_FASTCALL(u32,LPREADWORD,(u32));

#define MAKE_PIRESULT(info,a) (((info & 0xF) << 28) | (a & 0x0FFFFFFF))

#endif

