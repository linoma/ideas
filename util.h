#include "ideastypes.h"
#include "lds.h"

//---------------------------------------------------------------------------
#ifndef __utilH__
#define __utilH__

#define MMX_SUPPORTED			0x00800000
#define SSE_SUPPORTED			0x02000000
#define SSE2_SUPPORTED			0x04000000
#define AMD_3DNOW_SUPPORTED   	0x80000000
#define AMD_3DNOW_EX_SUPPORTED	0x40000000
#define AMD_MMX_EX_SUPPORTED	0x00400000

//---------------------------------------------------------------------------
DWORD StrToHex(char *string);
BOOL ControlMemoryBP(unsigned long address,unsigned long access,unsigned long value=0);
BOOL ShowOpenDialog(char *lpstrTitle,char *lpstrFilter,HWND hwnd,char *lpstrFileName,int nMaxFile = MAX_PATH,DWORD dwFlags = 0);
BOOL ShowSaveDialog(char *lpstrTitle,char *lpstrFileName,char *lpstrFilter,HWND hwnd,LPFNCSAVEDLG pFnc = NULL);
BOOL SaveBitmap(HDC hdc,HBITMAP bit,char *fileName);
u16 CalcCrc16(u16 *data,int length,u16 startValue = 0xFFFF);
void EnterDebugMode(BOOL bArm7);
u8 I_FASTCALL log2(u32 value);
BOOL SaveTextureAsTGA(u8 *image,u16 width,u16 height,u32 data);
BOOL CheckSSE();
BOOL CheckMMX();
#if defined(_DEBPRO)
#define WriteMsgConsolle floatDlg.get_ConsolleView()->WriteMessageConsole
#define UpdateControlProcessor floatDlg.UpdateCP
#elif defined(_DEBUG)
#define WriteMsgConsolle floatDlg.get_ConsolleView()->WriteMessageConsole
#define UpdateControlProcessor //
#else
#define WriteMsgConsolle //
#define UpdateControlProcessor //
#endif


#endif

