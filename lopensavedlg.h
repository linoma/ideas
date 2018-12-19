#ifndef LOPENSAVEDLG_H
#define LOPENSAVEDLG_H

#include "ldlg.h"
#include "llist.h"
#include "dstype.h"

class LOpenSaveDlg
{
public:
	LOpenSaveDlg();
	~LOpenSaveDlg();
	BOOL ShowOpen(char *caption,char *lpstrFilter,HWND hWndParent,char *fileName,int nMaxFile,DWORD dwFlags);
	BOOL ShowSave(char *caption,char *lpstrFilter,HWND hWndParent,char *fileName,LPFNCSAVEDLG pFnc = NULL);
};

#endif
