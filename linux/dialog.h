#ifndef __DIALOGH__
#define __DIALOGH__

#ifndef __WIN32__

const WORD *DIALOG_Get32(const WORD *p,LPDLG_INFO info);
const WORD *DIALOG_GetControl32( const WORD *p, DLG_CONTROL_INFO *info,BOOL dialogEx);
HWND DIALOG_CreateControl(DLG_CONTROL_INFO *info,gint xBaseUnit,gint yBaseUnit,HWND hDlg);
void set_DialogNotifyParent(HWND hDlg);

#endif

#endif


