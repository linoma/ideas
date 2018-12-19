#include "ideastypes.h"
#include "ldlg.h"
#include "dstype.h"
#include "llist.h"

//---------------------------------------------------------------------------
#ifndef __ioviewerH__
#define __ioviewerH__
//---------------------------------------------------------------------------
typedef struct _tagInfoMem{
   u32 adr;
   u8 flags;
   char descr[20];
   char descrex[100];
   char descrbit[1000];
} INFOMEM,*LPINFOMEM;
//---------------------------------------------------------------------------
class LIOViewer : public LDlg
{
public:
   LIOViewer();
   ~LIOViewer();
   BOOL Show(HWND parent = NULL);
   BOOL Destroy();
   void Update();
protected:
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   void DestroyControl();
   void FillComboBox2();
   void OnSelChangeComboBox2(BOOL bUpdate = FALSE);
   void OnClickCkeckBits(int id);
   void OnChangeEditValue();
   u32 access_mem(u32 adr,u32 data,u8 flags,u8 oper);
   BOOL OnInitDialog();
   HWND hWnd[64];
   LList *pInfoMemList;
   LPINFOMEM currentInfoMem;
};
#endif
