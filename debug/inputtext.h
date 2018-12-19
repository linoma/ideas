#include "ideastypes.h"

//---------------------------------------------------------------------------
#ifndef inputtextH
#define inputtextH
//---------------------------------------------------------------------------
BOOL InputText(HWND parent,LPRECT rcPos,DWORD dwStyle,char *string,int maxlen);
BOOL InputCombo(HWND parent,LPRECT rcPos,DWORD dwStyle,char *string,int maxlen);

#endif
