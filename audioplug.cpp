#define INITGUID

#include "audioplug.h"
#include "pluginctn.h"
#include "resource.h"
#include "dsmem.h"
#include "gpu.h"
#include "io.h"
#include "sound.h"
#include "lds.h"
#include "lregkey.h"
#ifdef _MICAUDIOINTERNAL
#include "micguid.h"
#include "capture.h"
#endif
//---------------------------------------------------------------------------
extern SOUNDPARAM spStruct;
extern u32 nOptions;
extern int nResample,nFrames;
extern INDS *lpNDS;
extern IPlugInManager *lpPlugInManager;
#ifdef _AUDIOINTERNAL
DEFINE_GUID(DSAUDIO_PLUGIN,0x914C7FFB,0x5AE1,0x4A02,0x81,0x60,0x3D,0x3A,0xC5,0x31,0xF6,0x3B);
#endif

extern void nullIOFunc ( u32,u32,u8 );
static PlugIn *pFirstHookPlugIn;
//---------------------------------------------------------------------------
static void I_STDCALL WriteTable(u32 adr,void *I,void *O)
{
	if((adr & 0x0FF00000) != 0x04000000)
		return;
	adr &= 0xFFF;
	if(adr < 0x400 || adr > 0x51F)
		return;
	if(I != NULL)
		i_func7[adr] = (LPIFUNC) I;
	o_func7[adr] = (LPOFUNC) O;
}
//---------------------------------------------------------------------------
static void AudioInputFunc(u32 adr,u32 data,u8 am)
{
	EXEC_INPUTHOOK
}
//---------------------------------------------------------------------------
static u32 AudioOutputFunc(u32 adr,u8 am)
{
	EXEC_OUTPUTHOOK
}
//---------------------------------------------------------------------------
AudioPlug::AudioPlug() : PlugIn()
{
	dwType = PIT_AUDIO;
}
//---------------------------------------------------------------------------
u32 AudioPlug::CallOutputFunc(u32 adr,u8 am)
{
	int index;

	if(adr < 0x400 || adr > 0x4FF)
		return 0;
	index = adr - 0x400;
	if(pOutputFunc[index] != NULL)
		return pOutputFunc[index](adr,am);
	return 0;
}
//---------------------------------------------------------------------------
void AudioPlug::CallInputFunc(u32 adr,u32 data,u8 am)
{
	int index;

	if(adr < 0x400 || adr > 0x4FF)
		return;
	index = adr - 0x400;
	pInputFunc[index](adr,data,am);
}
//---------------------------------------------------------------------------
int AudioPlug::ResetTable()
{
	int i;

	for(i=0x400;i<0x520;i++){
		i_func7[i] = nullIOFunc;
		o_func7[i] = NULL;
	}
   return S_OK;
}
//---------------------------------------------------------------------------
BOOL AudioPlug::InitSetPlugInInfo(LPSETPLUGININFO p,DWORD dwState,DWORD dwStateMask )
{
	if(!PlugIn::InitSetPlugInInfo(p,dwState,dwStateMask))
		return FALSE;
	if(!IsAttribute(PIT_ISFILTER|PIT_ISMICROPHONE)){
		p->lParam = (LPARAM)&sp;
		sp.io_mem = io_mem7;
		sp.pfn_rb = read_byte;
		sp.pfn_rhw = read_hword;
		sp.pfn_rw = read_word;
		sp.pfn_writetable = ::WriteTable;
	}
	return TRUE;
}
//---------------------------------------------------------------------------
void AudioPlug::EnableHook(BOOL bFlag)
{
	int i;

	if(bFlag){
		if(pInputFunc == NULL){
			if((pInputFunc = (LPIFUNC *)LocalAlloc(LPTR,0x100 * sizeof(LPIFUNC))) == NULL)
				return;
		}
		if(pOutputFunc == NULL){
			if((pOutputFunc = (LPOFUNC *)LocalAlloc(LPTR,0x100 * sizeof(LPOFUNC))) == NULL)
				return;
		}
		if(pFirstHookPlugIn == NULL)
			pFirstHookPlugIn = this;
		for(i=0x400;i<0x500;i++)
			pInputFunc[i-0x400] = i_func7[i];
		for(i=0x320;i<0x6F0;i++)
			pOutputFunc[i-0x400] = o_func7[i];
		for(i=0x400;i<0x500;i++){
           if(i_func7[i] != nullIOFunc)
				i_func7[i] = AudioInputFunc;
			o_func7[i] = AudioOutputFunc;
		}
	}
	else{
		if(pInputFunc != NULL){
			for(i=0x400;i<0x500;i++)
				i_func7[i] = pInputFunc[i-0x400];
			LocalFree(pInputFunc);
			pInputFunc = NULL;
		}
		if(pOutputFunc != NULL){
			for(i=0x400;i<0x500;i++)
				o_func7[i] = pOutputFunc[i-0x400];
			LocalFree(pOutputFunc);
			pOutputFunc = NULL;
		}
	}
}
//---------------------------------------------------------------------------
int AudioPlug::WriteTable(DWORD adr,LPVOID I,LPVOID O)
{
	if(IsAttribute(PIT_ISMICROPHONE) || (adr & 0x0FF00000) != 0x04000000)
		return E_FAIL;
	adr &= 0xFFF;
	if(adr < 0x400 || adr > 0x51F)
		return E_FAIL;
	if(I != NULL)
		i_func7[adr] = (LPIFUNC) I;
	o_func7[adr] = (LPOFUNC) O;
	return S_OK;
}
//---------------------------------------------------------------------------
BOOL AudioPlug::OnEnableComplex()
{
	return FALSE;
}
//---------------------------------------------------------------------------
AudioPlugList::AudioPlugList() : PlugInList("Audio Render")
{
	pFirstHookPlugIn = NULL;
   pActiveMicPlugIn = NULL;
}
//---------------------------------------------------------------------------
AudioPlugList::~AudioPlugList()
{
   LRegKey reg;
   GUID guid={0};
   GETPLUGININFO pi;

	if(pActiveMicPlugIn != NULL){
       ZeroMemory(&pi,sizeof(GETPLUGININFO));
       pi.cbSize = sizeof(GETPLUGININFO);
       if(pActiveMicPlugIn->GetInfo(&pi))
       	memcpy(&guid,&pi.guidID,sizeof(GUID));
   }
   reg.Open("Software\\iDeaS");
   reg.WriteBinaryData("Audio Microphone Render",(char *)&guid,sizeof(GUID));
   reg.Close();
}
//---------------------------------------------------------------------------
AudioPlug *AudioPlugList::BuildPlugIn(char *path)
{
	AudioPlug *p;

	if(path == NULL || (p = new AudioPlug()) == NULL)
		return NULL;
	p->SetLibraryPath(path);
	return p;
}
//---------------------------------------------------------------------------
BOOL AudioPlugList::PreLoad(WORD *wID)
{
#ifdef _AUDIOINTERNAL
	AudioPlug *pAudioPlug;

	pAudioPlug = new DSAudioPlugIn();
	if(pAudioPlug != NULL){
		pAudioPlug->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(pAudioPlug)){
       	(*wID)--;
	        delete pAudioPlug;
       }
   }
#endif
#ifdef _MICAUDIOINTERNAL
	AudioPlug *pAudioPlug;

	pAudioPlug = new DSMicAudioPlugin();
	if(pAudioPlug != NULL){
		pAudioPlug->SetMenuID(*wID);
   	(*wID)++;
		if(!Add(pAudioPlug)){
       	(*wID)--;
	        delete pAudioPlug;
       }
   }

#endif
	return TRUE;
}
//---------------------------------------------------------------------------
void AudioPlugList::LoadSetConfig()
{
   LRegKey reg;
   GUID guid;

   PlugInList::LoadSetConfig();
   reg.Open("Software\\iDeaS");
   reg.ReadBinaryData("Audio Microphone Render",(char *)&guid,sizeof(GUID));
   Enable(&guid,TRUE);
   reg.Close();
}
//---------------------------------------------------------------------------
PlugIn *AudioPlugList::get_ActivePlugIn(DWORD type)
{
   if(type & PIT_ISMICROPHONE)
       return pActiveMicPlugIn;
   return pActivePlugIn;
}
//---------------------------------------------------------------------------
PlugIn *AudioPlugList::SetActivePlugIn(PlugIn *ele,DWORD dwType)
{
   PlugIn *p;

   p = pActivePlugIn;
   PlugInList::SetActivePlugIn(ele);
   if((ele != NULL && !ele->IsAttribute(PIT_ISMICROPHONE)) || (ele == NULL && !(dwType & PIT_ISMICROPHONE)))
       ds.set_ActiveAudioPlugIn((AudioPlug *)pActivePlugIn);
   else if((ele != NULL && ele->IsAttribute(PIT_ISMICROPHONE)) || (ele == NULL && (dwType & PIT_ISMICROPHONE))){
       p = pActiveMicPlugIn;
       pActiveMicPlugIn = pActivePlugIn;
//       ds.set_Active2DPlugIn((Render2DPlugIn *)p2DActivePlugIn);
       pActivePlugIn = ds.get_ActiveAudioPlugIn();
   }
   return p;
}
//---------------------------------------------------------------------------
BOOL AudioPlugList::CheckTypePlugIn(DWORD dwPlugInType,DWORD Type)                     
{
	if(!PlugInList::CheckTypePlugIn(dwPlugInType,Type))
   	return FALSE;
   if(Type == 0)
   	return TRUE;
	return (BOOL)((dwPlugInType & PIT_ISMICROPHONE) == (Type & PIT_ISMICROPHONE));
}
//---------------------------------------------------------------------------
BOOL AudioPlugList::CheckIsComplex(PlugIn *p)
{
	return p != NULL;
}                                                           
#ifdef _AUDIOINTERNAL
//---------------------------------------------------------------------------
DSAudioPlugIn::DSAudioPlugIn() : AudioPlug()
{
	guid = DSAUDIO_PLUGIN;
	name = "DirectSound PlugIn";
	dwType = PIT_AUDIO;
	dwFlags = PIT_DYNAMIC;
}
//---------------------------------------------------------------------------
DSAudioPlugIn::~DSAudioPlugIn()
{
}
//---------------------------------------------------------------------------
BOOL DSAudioPlugIn::Enable(BOOL bFlag)
{
   SETPLUGININFO p;
   IPlugInInterface *plugin;
   IPlugInManager *manager;

   p.lpNDS = (INDS *)&ds;
   ds.QueryInterface(IID_IPLUGINMANAGER,(LPVOID *)&manager);
   manager->get_PlugInInterface(&guid,(LPVOID *)&plugin);
   plugin->WriteTable(0,0,0);
   plugin->get_Format(0,(LPVOID *)15,(DWORD *)15);
   p.lParam = (LPARAM)&sp;
	sp.io_mem = io_mem7;
	sp.pfn_rb = read_byte;
	sp.pfn_rhw = read_hword;
	sp.pfn_rw = read_word;
	sp.pfn_writetable = ::WriteTable;
	if(!AudioPlug::Enable(bFlag))
		return FALSE;
	if(!bFlag )
		return TRUE;
	if(!InitSoundDevice(&p))
		return FALSE;
	if(!EnableSound())
		return FALSE;
	InitSoundTable();
	return Reset();
}
//---------------------------------------------------------------------------
void DSAudioPlugIn::Stop(unsigned long value)
{
	StopSoundSystem(value);
}
//---------------------------------------------------------------------------
DWORD  DSAudioPlugIn::Run(BOOL bSyncro)
{
	return UpdateDSChannel();
}
//---------------------------------------------------------------------------
BOOL DSAudioPlugIn::Reset()
{
	ResetSoundSystem();
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL DSAudioPlugIn::Destroy()
{
	DestroySoundSystem();
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL DSAudioPlugIn::SaveState(LStream *p)
{
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL DSAudioPlugIn::LoadState(LStream *p,int ver)
{
	return ::LoadState(p,ver);
}
//---------------------------------------------------------------------------
BOOL DSAudioPlugIn::NotifyState(DWORD dwState,DWORD dwStateMask,LPARAM lParam)
{
	if(dwStateMask & PIS_NOTIFYMASK){
		if(dwState == PNM_ENDLINE)
			RenderDSChannel(lParam);
       else if(dwState == PNM_COUNTFRAMES)
           nResample = (int)(((lParam >> 2) << 2) / 59.89f * (float)SND_DECIMAL);
	}
	if(dwStateMask & PIS_RUNMASK){
		if((dwState & PIS_RUNMASK) == 4 || !(dwState & PIS_RUNMASK)){
           StopSoundSystem((BOOL)((dwState & PIS_RUNMASK) ? TRUE : FALSE));
       }
	}
	return TRUE;
}
//---------------------------------------------------------------------------
static int ValueToDB ( int val )
{
	return val - 100;
}
//---------------------------------------------------------------------------
BOOL DSAudioPlugIn::SetProperty(LPSETPROPPLUGIN p)
{
	PROPSHEETPAGE psp;
	HPROPSHEETPAGE hpsp;

	ZeroMemory ( &psp,sizeof ( PROPSHEETPAGE ) );
	psp.dwSize = sizeof ( PROPSHEETPAGE );
	psp.dwFlags = PSP_DEFAULT|PSP_USETITLE;
	psp.hInstance = GetModuleHandle(NULL);
	psp.pfnDlgProc = (DLGPROC)DialogProc;
	psp.pszTemplate = MAKEINTRESOURCE ( IDD_DIALOG17 );
	psp.pszTitle = name.c_str();
	hpsp = CreatePropertySheetPage(&psp);
	if(hpsp == NULL)
		return FALSE;
	return PropSheet_AddPage((HWND)p->hwndOwner,hpsp);
}
//---------------------------------------------------------------------------
BOOL CALLBACK DSAudioPlugIn::DialogProc ( HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam )
{
	char s[30];
	int iPos;

	switch(uMsg){
       case WM_INITDIALOG:
		    if(!((nOptions >> 6) & 1))
			    SendDlgItemMessage(hwnd,IDC_RADIO1,BM_SETCHECK,BST_CHECKED,0);
			else
				SendDlgItemMessage(hwnd,IDC_RADIO2,BM_SETCHECK,BST_CHECKED,0);
			if(!((nOptions >> 6) & 2))
				SendDlgItemMessage(hwnd,IDC_RADIO4,BM_SETCHECK,BST_CHECKED,0);
			else
				SendDlgItemMessage(hwnd,IDC_RADIO3,BM_SETCHECK,BST_CHECKED,0);
			SendDlgItemMessage(hwnd,IDC_SYNCRONIZE,CB_ADDSTRING,0,(LPARAM)"No Syncronize (slow quality)" );
			SendDlgItemMessage(hwnd,IDC_SYNCRONIZE,CB_ADDSTRING,0,(LPARAM)"Append (medium quality)" );
			SendDlgItemMessage(hwnd,IDC_SYNCRONIZE,CB_ADDSTRING,0,(LPARAM)"Syncronize (best quality)" );
			SendDlgItemMessage(hwnd,IDC_SYNCRONIZE,CB_SETCURSEL,(nOptions & 7),0 );
			SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETRANGE,TRUE,MAKELPARAM( 0,100 ) );
			SendDlgItemMessage(hwnd,IDC_TRACK1,TBM_SETPOS,(WPARAM)TRUE,(iPos = get_Volume() ) );
			wsprintf(s,"%d",ValueToDB(iPos));
			SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,(LPARAM)s );
			SendDlgItemMessage(hwnd,IDC_TRACK2,TBM_SETRANGE,TRUE,MAKELPARAM(0,7));
			SendDlgItemMessage(hwnd,IDC_TRACK2,TBM_SETPOS,(WPARAM)TRUE,(iPos = (nOptions >> 3 ) & 7 ) );
			wsprintf(s,"%d",iPos);
			SendDlgItemMessage(hwnd,IDC_EDIT2,WM_SETTEXT,0, ( LPARAM ) s );
			break;
#ifdef __WIN32__
		case WM_CTLCOLORSTATIC:
			if( GetDlgCtrlID ( ( HWND ) lParam ) == IDC_EDIT1 || GetDlgCtrlID ( ( HWND ) lParam ) == IDC_EDIT2 )
			{
				SetTextColor ( ( HDC ) wParam,GetSysColor ( COLOR_WINDOWTEXT ) );
				return ( BOOL ) GetSysColorBrush ( COLOR_WINDOW );
			}
			break;
#endif
		case WM_HSCROLL:
			switch ( LOWORD ( wParam ) )
			{
				case TB_THUMBTRACK:
				case TB_ENDTRACK:
					switch ( GetDlgCtrlID ( ( HWND ) lParam ) )
					{
						case IDC_TRACK1:
							iPos = ( int ) SendMessage ( ( HWND ) lParam,TBM_GETPOS,0,0 );
							wsprintf ( s,"%d",ValueToDB ( iPos ) );
							SendDlgItemMessage ( hwnd,IDC_EDIT1,WM_SETTEXT,0, ( LPARAM ) s );
							break;
						case IDC_TRACK2:
							iPos = ( int ) SendMessage ( ( HWND ) lParam,TBM_GETPOS,0,0 );
							wsprintf ( s,"%d",iPos );
							SendDlgItemMessage ( hwnd,IDC_EDIT2,WM_SETTEXT,0, ( LPARAM ) s );
							break;
					}
					break;
				default:
					break;
			}
			break;
		case WM_NOTIFY:
			if ( ( ( LPNMHDR ) lParam )->code == PSN_APPLY )
			{
				nOptions &= ~0x1FF;
				nOptions |= ( u32 ) SendDlgItemMessage ( hwnd,IDC_SYNCRONIZE,CB_GETCURSEL,0,0 );
				nOptions |= ( u32 ) ( SendDlgItemMessage ( hwnd,IDC_TRACK2,TBM_GETPOS,0,0 ) << 3 );
				if ( SendDlgItemMessage ( hwnd,IDC_RADIO2,BM_GETCHECK,0,0 ) == BST_CHECKED )
					nOptions |= 0x40;
				if ( SendDlgItemMessage ( hwnd,IDC_RADIO3,BM_GETCHECK,0,0 ) == BST_CHECKED )
					nOptions |= 0x80;
				set_SoundOptions ( nOptions );
				iPos = ( int ) SendDlgItemMessage ( hwnd,IDC_TRACK1,TBM_GETPOS,0,0 );
				set_Volume ( iPos );
			}
			break;
	}
	return FALSE;
}
#endif
//---------------------------------------------------------------------------
#ifdef _MICAUDIOINTERNAL
static LSoundCapture *plugin;
//---------------------------------------------------------------------------
DSMicAudioPlugin::DSMicAudioPlugin() : AudioPlug()
{
	guid = MICGUID;
	name = "DirectSound Microphone PlugIn";
	dwType = PIT_AUDIO;
	dwFlags = PIT_DYNAMIC|PIT_ISMICROPHONE;
}
//---------------------------------------------------------------------------
DSMicAudioPlugin::~DSMicAudioPlugin()
{
}
//---------------------------------------------------------------------------
BOOL DSMicAudioPlugin::Enable(BOOL bFlag)
{
	if(!AudioPlug::Enable(bFlag))
		return FALSE;
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL DSMicAudioPlugin::Reset()
{
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL DSMicAudioPlugin::Destroy()
{
   return TRUE;
}
//---------------------------------------------------------------------------
DWORD DSMicAudioPlugin::Run(BOOL bSyncro)
{
	return (DWORD)plugin->get_Sample();
}
//---------------------------------------------------------------------------
BOOL DSMicAudioPlugin::NotifyState(DWORD dwState,DWORD dwStateMask,LPARAM lParam)
{
   if((dwState & PIS_ENABLE) && (dwStateMask & PIS_ENABLEMASK)){
       if(plugin == NULL){
			    plugin = new LSoundCapture();
			    if(plugin == NULL)
				    return FALSE;
       }
       if(!plugin->Init())
           return FALSE;
   }

   if(dwStateMask & PIS_RUNMASK){
//       if((p->dwState & PIS_RUNMASK) == 4 || !(p->dwState & PIS_RUNMASK))
//		   StopSoundSystem((BOOL)((p->dwState & PIS_RUNMASK) ? TRUE : FALSE));
   }
   if(dwStateMask & PIS_NOTIFYMASK){
       switch(dwState){
           case PNM_ENDLINE:
               if(plugin)
                   plugin->OnEndLine(lParam);
           break;
           case PNM_COUNTFRAMES:
               if(plugin)
                   plugin->OnFramesCount(lParam);
           break;
           case PNM_ENDFRAME:
               if(plugin)
                   plugin->OnEndFrame();
           break;
           case PNMP_POWERCNT:
               if(plugin != NULL){
                   switch(LOWORD(lParam)){
                       case 2:
                           if(HIWORD(lParam))
                               plugin->Start();
                           else
                               plugin->Stop();
                       break;
                       case 3:
                       break;
                   }
               }
           break;
       }
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL DSMicAudioPlugin::SaveState(LStream *p)
{
   return plugin->Save(p);
}
//---------------------------------------------------------------------------
BOOL DSMicAudioPlugin::LoadState(LStream *p,int ver)
{
   return plugin->Load(p,ver);
}
//---------------------------------------------------------------------------
BOOL CALLBACK DSMicAudioPlugin::DialogProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   return plugin->DialogProc(hwndDlg,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
BOOL DSMicAudioPlugin::SetProperty(LPSETPROPPLUGIN p)
{
	PROPSHEETPAGE psp;
	HPROPSHEETPAGE hpsp;

	ZeroMemory(&psp,sizeof(PROPSHEETPAGE));
	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.dwFlags = PSP_DEFAULT|PSP_USETITLE;
	psp.hInstance = GetModuleHandle(NULL);
	psp.pfnDlgProc = (DLGPROC)DialogProc;
	psp.pszTemplate = MAKEINTRESOURCE(122);
	psp.pszTitle = name.c_str();
	hpsp = CreatePropertySheetPage(&psp);
	if(hpsp == NULL)
		return FALSE;
	return PropSheet_AddPage((HWND)p->hwndOwner,hpsp);
}
#endif

