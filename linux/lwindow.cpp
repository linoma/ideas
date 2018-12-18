#include "ideastypes.h"
#include "lwnd.h"
#include "dialog.h"
#include "lstring.h"
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <cairo/cairo-xlib.h>
#include <gio/gdesktopappinfo.h>

#ifndef __WIN32__
extern int GetResourceMemory(char **buf,unsigned long *size);

static GtkClipboard* clipboard = NULL;
static cairo_user_data_key_t cr_data;
static LCaret *currentCaret = NULL;
//---------------------------------------------------------------------------
static void OnEnumChild(GtkWidget *widget,gpointer data)
{
   if(((LPDWORD)data)[1] != 0)
       return;
	if(GTK_IS_CONTAINER(widget))
		gtk_container_foreach((GtkContainer *)widget,OnEnumChild,data);
	if(GetWindowLong(widget,GWL_ID) == ((LPDWORD)data)[0])
		((LPDWORD)data)[1] = (DWORD)widget;
}
//---------------------------------------------------------------------------
void strlwr(char *s)
{
	if(s == NULL)
		return;
	for(int i = 0;i < strlen(s);i++)
		s[i] = tolower(s[i]);
}
//---------------------------------------------------------------------------
DWORD GetFileAttributes(const char *file)
{
 	DWORD res;
	struct stat sb;

	res = 0xFFFFFFFF;
	if(stat(file,&sb) != -1){
		res = 0;
		if(S_ISDIR(sb.st_mode))
			res |= FILE_ATTRIBUTE_DIRECTORY;
		if(S_ISREG(sb.st_mode)){
			res = 0;
			if(sb.st_mode & S_IFDIR)
				res |= FILE_ATTRIBUTE_DIRECTORY;
		}
	}
	return res;
}
//---------------------------------------------------------------------------
void SetRect(LPRECT p,int x,int y,int width,int height)
{
	p->left = x;
	p->top = y;
	p->right = width;
	p->bottom = height;
}
//---------------------------------------------------------------------------
void CopyRect(LPRECT p,LPRECT p1)
{
	memcpy(p,p1,sizeof(RECT));
}
//---------------------------------------------------------------------------
BOOL InflateRect(LPRECT lprc,int dx,int dy)
{
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL OffsetRect(LPRECT lprc,int dx,int dy)
{
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL PtInRect(CONST RECT *lprc,POINT pt)
{
	if(lprc == NULL)
		return FALSE;
	if(pt.x < lprc->left || pt.x > lprc->right || pt.y < lprc->top || pt.y > lprc->bottom)
		return FALSE;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL IsRectEmpty(const RECT *lprc)
{
    if(lprc == NULL)
        return true;
    if(lprc->right <= lprc->left || lprc->bottom <= lprc->top)
        return true;
    return FALSE;
}
//---------------------------------------------------------------------------
DWORD GetLastError()
{
	return 0;
}
//---------------------------------------------------------------------------
BOOL FindClose(HANDLE hFindFile)
{
	if(hFindFile == INVALID_HANDLE_VALUE || hFindFile == NULL)
		return FALSE;
	closedir(*((DIR **)hFindFile));
	LocalFree(hFindFile);
}
//---------------------------------------------------------------------------
HANDLE FindFirstFile(LPCSTR lpFileName,LPWIN32_FIND_DATA lpFindFileData)
{
	DIR *handle;
	struct dirent *ent;
	char *res,*path,*p,*filter;
	BOOL bFilter;

	if(lpFileName == NULL || lpFindFileData == NULL)
		return (HANDLE)INVALID_HANDLE_VALUE;
	ZeroMemory(lpFindFileData,sizeof(WIN32_FIND_DATA));
	path = (char *)LocalAlloc(LPTR,MAX_PATH+1);
	if(path == NULL)
		return (HANDLE)INVALID_HANDLE_VALUE;
	res = strrchr(lpFileName,'/');
	if(res != NULL)
		strncpy(path,lpFileName,(int)res - (int)lpFileName + 1);
	else
		strcpy(path,"./");
	res = NULL;
	if((handle = opendir(path)) != NULL){
		res = (char *)LocalAlloc(LPTR,sizeof(DIR *)+(MAX_PATH * 2) +1);
		*((DIR **)res) = handle;
		filter = lpFileName;
   		if((p = strrchr(filter,'/')) == NULL)
       		p = strrchr(filter,'\\');
		lstrcpy(&res[sizeof(DIR *)],path);
   		if(p != NULL)
       		filter = ++p;
		bFilter = TRUE;
		if(*filter == '*' || *filter == '?'){
			if(lstrlen(filter) > 1){
				p = filter + 1;
				if(p[1] == '*')
					bFilter = FALSE;
			}
			else
				bFilter = FALSE;
		}
		else
			p = filter;
		if(bFilter)
			lstrcat(&res[sizeof(DIR *) + lstrlen(path) + 1],p);
		if(!FindNextFile((HANDLE)res,lpFindFileData)){
			LocalFree(res);
			closedir(handle);
			handle = (DIR *)INVALID_HANDLE_VALUE;
		}
		else
			handle = (DIR *)res;
	}
	else
		handle = (DIR *)INVALID_HANDLE_VALUE;
	LocalFree(path);
	return (HANDLE)handle;
}
//---------------------------------------------------------------------------
BOOL FindNextFile(HANDLE hFindFile,LPWIN32_FIND_DATA lpFindFileData)
{
	struct dirent *ent;
	DIR *dir;
	BOOL bGood;
	char *filter,*path,file[MAX_PATH+1];
	struct stat sb;

	if(hFindFile == INVALID_HANDLE_VALUE || hFindFile == NULL)
		return FALSE;
	ZeroMemory(lpFindFileData,sizeof(WIN32_FIND_DATA));
	dir = ((DIR **)hFindFile)[0];
	path = ((char *)hFindFile) + sizeof(DIR *);
	filter = path + lstrlen(path)+1;
	do{
		ent = readdir(dir);
		if(ent == NULL)
			return FALSE;
		bGood = TRUE;
		if(*filter){
			bGood = FALSE;
			if(strcasestr(ent->d_name,filter) != NULL)
				bGood = TRUE;
		}
		if(bGood){
			lstrcpyn(lpFindFileData->cFileName,ent->d_name,MAX_PATH);
		//	lpFindFileData->nFileSizeLow =
			if(lstrlen(ent->d_name) > 11){
				lstrcpyn(lpFindFileData->cAlternateFileName,ent->d_name,6);
				lstrcat(lpFindFileData->cAlternateFileName,"~");
				lstrcat(lpFindFileData->cAlternateFileName,&ent->d_name[lstrlen(ent->d_name)-4]);
			}
			if(ent->d_type == DT_DIR)
				lpFindFileData->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
			else
				lpFindFileData->dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
			lstrcpy(file,path);
			lstrcat(file,ent->d_name);
			if(stat(file,&sb) != -1){
				lpFindFileData->nFileSizeLow = sb.st_size;
			}
			return TRUE;
		}
	}while(1);
}
//---------------------------------------------------------------------------
BOOL DeleteFile(LPCSTR lpFileName)
{
	if(remove(lpFileName) == 0)
		return TRUE;
	return FALSE;
}
//---------------------------------------------------------------------------
UINT GetTempFileName(LPCSTR lpPathName,LPCSTR lpPrefixString,UINT uUnique,LPSTR lpTempFileName)
{
	char *p,s[20];

	if(lpPathName == NULL || lpTempFileName == NULL)
		return 0;
	p = (char *)LocalAlloc(LPTR,MAX_PATH+1);
	if(p == NULL)
		return 0;
	lstrcpy(p,lpPathName);
	if(p[lstrlen(p)] != '/')
		lstrcat(p,"/");
	if(lpPrefixString != NULL)
		lstrcat(p,lpPrefixString);
	if(uUnique == 0)
		uUnique = GetTickCount();
	wsprintf(s,"%08X.tmp",uUnique);
	lstrcat(p,s);
	lstrcpy(lpTempFileName,p);
	LocalFree(p);
	return uUnique;
}
//---------------------------------------------------------------------------
DWORD GetTempPath(DWORD nBufferLength,LPSTR lpBuffer)
{
	char *p;
	DWORD dwLen;

	p = getenv("HOME");
	if(p == NULL)
		return 0;
	dwLen = lstrlen(p);
	if(lpBuffer == NULL || nBufferLength < dwLen)
		return dwLen;
	lstrcpyn(lpBuffer,p,dwLen+1);
	return dwLen;
}
//---------------------------------------------------------------------------
SHORT GetAsyncKeyState(int vKey)
{
	return 0;
}
//--------------------------------------------------------------------------------
int GetObject(void *obj,int buflen,LPVOID buf)
{
	if(obj == NULL || buf == NULL)
		return 0;
	if(GDK_IS_PIXBUF(obj)){
		if(buflen != sizeof(BITMAP))
			return sizeof(BITMAP);
		((LPBITMAP)buf)->bmWidth = gdk_pixbuf_get_width((GdkPixbuf *)obj);
		((LPBITMAP)buf)->bmHeight = gdk_pixbuf_get_height((GdkPixbuf *)obj);
		((LPBITMAP)buf)->bmPlanes = 1;
		((LPBITMAP)buf)->bmWidthBytes = gdk_pixbuf_get_rowstride((GdkPixbuf *)obj);
		((LPBITMAP)buf)->bmBitsPixel = (((LPBITMAP)buf)->bmWidthBytes / ((LPBITMAP)buf)->bmWidth) * 8;
		((LPBITMAP)buf)->bmBits = gdk_pixbuf_get_pixels((GdkPixbuf *)obj);
		return sizeof(BITMAP);
	}
	return 0;
}
//--------------------------------------------------------------------------------
HWND CreateDialogParam(HINSTANCE instance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	void *hRsrc;
	LPWORD info;
	DLG_CONTROL_INFO ctrlInfo,ctrlInfoCopy;
    DLG_INFO dlgInfo;
	char temp[300];
	HWND hDlg;
	HWND vbox,fixed;
	UINT xBaseUnit,yBaseUnit;
	HWND hwnd,hwnd1,hwndToolbar,hwnd2;
	HMENU menu;
	RECT rc;
	int i,x,y,i1;
	GList *pList;
	GtkToolItem *btn;
	GList *list;

    if (!(hRsrc = FindResource( instance, lpTemplateName,RT_DIALOG )))
		return 0;
    if ((info = (LPWORD)LockResource(hRsrc)) == NULL)
		return 0;
   	info = (WORD *)DIALOG_Get32(info,&dlgInfo);
	WideCharToMultiByte(0,0,dlgInfo.caption,-1,temp,300,NULL,FALSE);
	fixed = gtk_fixed_new();
	gtk_widget_show(fixed);
	if(!(dlgInfo.style & DS_CONTROL)){
		hDlg = gtk_dialog_new_with_buttons(temp,(GtkWindow *)hWndParent,(GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_NO_SEPARATOR),NULL);
		if(hDlg == NULL)
			return NULL;
        gtk_widget_set_events(hDlg,(GDK_ALL_EVENTS_MASK & ~(GDK_SCROLL_MASK|GDK_SUBSTRUCTURE_MASK|GDK_PROXIMITY_OUT_MASK|GDK_PROXIMITY_IN_MASK)));
        g_signal_connect((gpointer)hDlg,"event",G_CALLBACK(on_window_event),NULL);

		vbox = GTK_DIALOG(hDlg)->vbox;
		SetWindowLong(hDlg,GWL_FIXED,(LONG)fixed);
		gtk_window_set_resizable(GTK_WINDOW (hDlg),(dlgInfo.style & WS_THICKFRAME) ? TRUE : FALSE);
		gtk_widget_show(vbox);
		SetWindowLong(hDlg,GWL_WNDPROC,(LONG)lpDialogFunc);
		SetWindowLong(hDlg,GWL_LPARAM,(LONG)dwInitParam);
		SetWindowLong(hDlg,GWL_STYLE,(LONG)dlgInfo.style);
	}
	else{
		vbox = NULL;
		hDlg = fixed;
	}
	xBaseUnit = GetDialogBaseUnits();
	yBaseUnit = xBaseUnit >> 16;
	xBaseUnit &= 0xFFFF;
//	gtk_window_set_type_hint (GTK_WINDOW (hDlg), GDK_WINDOW_TYPE_HINT_DIALOG);
	if(dlgInfo.menuName != NULL){
		menu = LoadMenuBar(instance,(char *)dlgInfo.menuName);
		SetMenu(hDlg,menu);
		gtk_box_pack_start(GTK_BOX (vbox),(HWND)menu,FALSE,FALSE,0);
		::GetWindowRect((HWND)menu,&rc);
		dlgInfo.cy += ((rc.bottom - rc.left) * 8) / yBaseUnit;
	}
	hwndToolbar = NULL;
	while(dlgInfo.nItems--){
		info = (LPWORD)DIALOG_GetControl32(info,&ctrlInfo,dlgInfo.dialogEx);
		hwnd = DIALOG_CreateControl(&ctrlInfo,xBaseUnit,yBaseUnit,hDlg);
		if(hwnd != NULL){
			if(GTK_IS_SPIN_BUTTON(hwnd) && (ctrlInfo.style & UDS_AUTOBUDDY)){
				if(hwnd1 != NULL && GTK_IS_ENTRY(hwnd1)){
					if(fixed != NULL){
						memcpy(&ctrlInfo,&ctrlInfoCopy,sizeof(DLG_CONTROL_INFO));
						gtk_widget_set_size_request(hwnd,ctrlInfo.cx*xBaseUnit/4,ctrlInfo.cy*yBaseUnit/8);
						SetWindowLong(hwnd,GWL_ID2,GetWindowLong(hwnd1,GWL_ID));
						if(ctrlInfo.style & WS_DISABLED)
							gtk_widget_set_sensitive(hwnd,false);
						gtk_container_remove((GtkContainer *)fixed,hwnd1);
						gtk_widget_destroy(hwnd1);
					}
				}
			}
			if(GTK_IS_TOOLBAR(hwnd) && vbox != NULL){
				gtk_box_pack_start(GTK_BOX (vbox),(HWND)hwnd,FALSE,FALSE,0);
				hwndToolbar = hwnd;
			}
			else if(GTK_IS_STATUSBAR(hwnd) && vbox != NULL)
				gtk_box_pack_end(GTK_BOX (vbox),(HWND)hwnd,FALSE,FALSE,0);
			else
				gtk_fixed_put((GtkFixed *)fixed,hwnd,ctrlInfo.x*xBaseUnit/4,ctrlInfo.y*yBaseUnit/8);
			hwnd1 = hwnd;
			memcpy(&ctrlInfoCopy,&ctrlInfo,sizeof(DLG_CONTROL_INFO));
		}
	}
	if(vbox != NULL){
		gtk_box_pack_start(GTK_BOX(vbox),(HWND)fixed,TRUE,TRUE,0);
		gtk_widget_set_size_request(hDlg,(dlgInfo.cx+2)*xBaseUnit / 4,dlgInfo.cy*yBaseUnit/8);
		gtk_widget_set_uposition(hDlg,dlgInfo.x*xBaseUnit/4,dlgInfo.y*yBaseUnit/8);
		ShowWindow(hDlg,SW_SHOW);
		/*if(hwndToolbar){
			pList = gtk_container_get_children((GtkContainer *)fixed);
			if(pList != NULL){
				btn = gtk_tool_button_new(NULL,"");
				ShowWindow((HWND)btn,SW_SHOW);
				gtk_toolbar_insert(GTK_TOOLBAR (hwndToolbar),btn,-1);
				ShowWindow((HWND)hwndToolbar,SW_SHOW);
				GetWindowRect(hwndToolbar,&rc);
				rc.bottom -= rc.top;
				gtk_container_remove((GtkContainer *)hwndToolbar,(HWND)btn);
				i1 = g_list_length(pList);
				for(i=0;i<i1;i++){
					hwnd = (HWND)g_list_nth_data(pList,i);
					gtk_widget_translate_coordinates(hwnd,(HWND)fixed,0,0,&x,&y);
					y -= rc.bottom;
					gtk_fixed_move((GtkFixed *)fixed,hwnd,x,y);
				}
			}
		}*/
//		XSelectInput(gdk_x11_drawable_get_xdisplay(hDlg->window),gdk_x11_drawable_get_xid(hDlg->window),0x13FFFD);
		return hDlg;
	}
	if(hWndParent != NULL){
		gtk_widget_set_size_request(fixed,(dlgInfo.cx+2)*xBaseUnit / 4,dlgInfo.cy*yBaseUnit/8);
		if(GTK_IS_WINDOW(hWndParent)){
			vbox = gtk_bin_get_child((GtkBin *)hWndParent);
			if(vbox != NULL){
				list = gtk_container_get_children((GtkContainer *)vbox);
				if(list != NULL){
					i1 = g_list_length(list);
					for(i=0;i<i1;i++){
						hwnd = (HWND)g_list_nth_data(list,i);
						if(hwnd != NULL && GTK_IS_FIXED(hwnd)){
							gtk_fixed_put((GtkFixed *)hwnd,fixed,dlgInfo.x*xBaseUnit/8,dlgInfo.y*yBaseUnit/8);
							break;
						}
					}
				}
			}
		}
	}
	return fixed;
}
//--------------------------------------------------------------------------------
BOOL EndDialog(HWND hDlg,INT nResult)
{
	GtkResponseType type;

	switch(nResult){
		case 0:
			type = GTK_RESPONSE_CANCEL;
		break;
		default:
			type = GTK_RESPONSE_OK;
		break;
	}
	gtk_dialog_response((GtkDialog *)hDlg,type);
	return TRUE;
}
//--------------------------------------------------------------------------------
int DialogBoxParam(HINSTANCE instance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
	HWND hDlg;
	gint res;

	hDlg = CreateDialogParam(instance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
	gtk_window_set_modal((GtkWindow *)hDlg,true);
	res = gtk_dialog_run(GTK_DIALOG (hDlg));
	gtk_widget_destroy(hDlg);
	if(res == GTK_RESPONSE_OK)
		return IDOK;
	else if(res == GTK_RESPONSE_CANCEL)
		return IDCANCEL;
	return IDOK;
}
//--------------------------------------------------------------------------------
int DialogBox(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc)
{
	return DialogBoxParam(hInstance,lpTemplateName,hWndParent,lpDialogFunc,0);
}
//--------------------------------------------------------------------------------
unsigned int strlenW(const WCHAR *str)
{
    const WCHAR *s = str;

    while(*s) s++;
    return s - str;
}
//--------------------------------------------------------------------------------
int WideCharToMultiByte(UINT CodePage,DWORD dwFlags,LPCWSTR lpWideCharStr,int cchWideChar,LPSTR lpMultiByteStr,int cbMultiByte,LPCSTR lpDefaultChar,LPBOOL lpUsedDefaultChar)
{
	GdkWChar *p;
	int i;
	gchar *p1;

	if(lpWideCharStr == NULL || lpMultiByteStr == NULL)
		return 0;
	if(cchWideChar == -1)
		cchWideChar = strlenW(lpWideCharStr);
	if(cchWideChar > cbMultiByte)
		cchWideChar = cbMultiByte;
	if(cchWideChar == 0)
		return 0;
	p = (GdkWChar *)LocalAlloc(LPTR,(cchWideChar+2)*sizeof(GdkWChar));
	for(i=0;i<cchWideChar;i++)
		p[i] = lpWideCharStr[i];
	p1 = gdk_wcstombs(p);
	LocalFree(p);
	if(p1 == NULL)
		return 0;
	lstrcpy(lpMultiByteStr,p1);
	g_free(p1);
	return lstrlen(lpMultiByteStr);
}
//---------------------------------------------------------------------------
BOOL DestroyWindow(HWND hwnd)
{
	if(hwnd == NULL)
		return FALSE;
	gtk_widget_destroy(hwnd);
	return TRUE;
}
//---------------------------------------------------------------------------
int GetSystemMetrics(int nIndex)
{
	switch(nIndex){
		default:
			return 3;
	}
}

//---------------------------------------------------------------------------
HDWP BeginDeferWindowPos(int nNumWindows)
{
	return (HDWP)TRUE;
}
//---------------------------------------------------------------------------
HDWP DeferWindowPos(HDWP hWinPosInfo,HWND hWnd,HWND hWndInsertAfter,int x,int y,int cx,int cy,UINT uFlags)
{
	return (HDWP)SetWindowPos(hWnd,hWndInsertAfter,x,y,cx,cy,uFlags);
}
//---------------------------------------------------------------------------
BOOL EndDeferWindowPos(HDWP hWinPosInfo)
{
	return TRUE;
}
//---------------------------------------------------------------------------
HWND CreateWindow(LPCSTR lpClassName,LPCSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam)
{
	return CreateWindowEx(0,lpClassName,lpWindowName,dwStyle,X,Y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);
}
//---------------------------------------------------------------------------
HWND CreateWindowEx(DWORD dwExStyle,LPCSTR lpClassName,LPCSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam)
{
	HWND hwnd;

	hwnd = NULL;
	if(lstrcmp(lpClassName,TOOLBARCLASSNAME) == 0)
		hwnd = CreateToolBar(dwStyle);
	else if(lstrcmp(lpClassName,"msctls_statusbar32") == 0)
		hwnd = CreateStatusBar(dwStyle);
	else if(lstrcmp(lpClassName,"SysTreeView32") == 0)
		hwnd = CreateTreeView(dwStyle);
	else if(lstrcmp(lpClassName,"EDIT") == 0)
		hwnd = CreateTreeView(dwStyle);
	if(hwnd == NULL)
		return NULL;
	SetWindowLong(hwnd,GWL_ID,(LONG)hMenu);
	SetWindowLong(hwnd,GWL_NOTIFYPARENT,(LONG)hWndParent);
	return hwnd;
}
//---------------------------------------------------------------------------
DWORD GetCurrentDirectory(DWORD nBufferLength,LPSTR lpBuffer)
{
	getcwd(lpBuffer,nBufferLength);
	return lstrlen(lpBuffer);
}
//---------------------------------------------------------------------------
BOOL SetCurrentDirectory(LPCSTR lpPathName)
{
	if(!chdir(lpPathName))
		return TRUE;
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL DosDateTimeToFileTime(WORD wFatDate,WORD wFatTime,LPFILETIME lpFileTime)
{
	struct tm t={0};

	if(lpFileTime == NULL)
		return FALSE;
	t.tm_sec = (wFatTime & 0x1f) * 2;
	t.tm_min = (wFatTime >> 5) & 0x3f;
	t.tm_hour= (wFatTime >> 11) & 0x1f;
	t.tm_mday = (wFatDate & 31);
	t.tm_mon = ((wFatDate >> 5) & 0xF)-1;
	t.tm_year = 80 + (wFatDate >> 9);
	t.tm_isdst = -1;
	*lpFileTime = mktime(&t);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL FileTimeToDosDateTime(CONST FILETIME *lpFileTime,LPWORD lpFatDate,LPWORD lpFatTime)
{
	struct tm *p;

	if(lpFileTime == NULL || lpFatDate  == NULL || lpFatTime == NULL)
		return FALSE;
	p = localtime((const time_t *)lpFileTime);
	if(p == NULL)
		return FALSE;
	*lpFatTime = p->tm_sec / 2;
	*lpFatTime |= p->tm_min << 5;
	*lpFatTime |= p->tm_hour << 11;
	*lpFatDate = p->tm_mday;
	*lpFatDate |= (p->tm_mon+1) << 5;
	*lpFatDate |= (p->tm_year - 80) << 9;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL FileTimeToLocalFileTime(FILETIME *lpFileTime,LPFILETIME lpLocalFileTime)
{
	struct tm *p;

	if(lpFileTime == NULL || lpLocalFileTime == NULL)
		return FALSE;
	/*p = localtime(lpFileTime);
	if(p == NULL)
		return FALSE;
	lpSystemTime->wSecond = p->tm_sec;
	lpSystemTime->wMinute = p->tm_min;
	lpSystemTime->wHour = p->tm_hour;
	lpSystemTime->wDay = p->tm_mday;
	lpSystemTime->wMonth = p->tm_mon + 1;
	lpSystemTime->wYear = 1900 + p->tm_year;
	lpSystemTime->wDayOfWeek = p->tm_wday;
	lpSystemTime->wMilliseconds = 0;*/
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL FileTimeToSystemTime(FILETIME *lpFileTime,LPSYSTEMTIME lpSystemTime)
{
	struct tm *p;

	if(lpFileTime == NULL || lpSystemTime == NULL)
		return FALSE;
	p = localtime((const time_t *)lpFileTime);
	if(p == NULL)
		return FALSE;
	lpSystemTime->wSecond = p->tm_sec;
	lpSystemTime->wMinute = p->tm_min;
	lpSystemTime->wHour = p->tm_hour;
	lpSystemTime->wDay = p->tm_mday;
	lpSystemTime->wMonth = p->tm_mon + 1;
	lpSystemTime->wYear = 1900 + p->tm_year;
	lpSystemTime->wDayOfWeek = p->tm_wday;
	lpSystemTime->wMilliseconds = 0;
	return TRUE;
}
//---------------------------------------------------------------------------
VOID GetLocalTime(LPSYSTEMTIME lpSystemTime)
{
	struct tm *p;
	time_t t;

	if(lpSystemTime == NULL)
		return;
	t = time(NULL);
	p = localtime(&t);
	ZeroMemory(lpSystemTime,sizeof(SYSTEMTIME));
	if(p == NULL)
		return;
	lpSystemTime->wSecond = p->tm_sec;
	lpSystemTime->wMinute = p->tm_min;
	lpSystemTime->wHour = p->tm_hour;
	lpSystemTime->wDay = p->tm_mday;
	lpSystemTime->wMonth = p->tm_mon + 1;
	lpSystemTime->wYear = 1900 + p->tm_year;
	lpSystemTime->wDayOfWeek = p->tm_wday;
	lpSystemTime->wMilliseconds = 0;
}
//---------------------------------------------------------------------------
BOOL SystemTimeToFileTime(CONST SYSTEMTIME *lpSystemTime,LPFILETIME lpFileTime)
{
	struct tm p;

	if(lpFileTime == NULL || lpSystemTime == NULL)
		return FALSE;
	p.tm_sec = lpSystemTime->wSecond;
	p.tm_min = lpSystemTime->wMinute;
	p.tm_hour = lpSystemTime->wHour;
	p.tm_mday = lpSystemTime->wDay;
	p.tm_mon = lpSystemTime->wMonth - 1;
	p.tm_year = lpSystemTime->wYear - 1900;
	p.tm_wday = lpSystemTime->wDayOfWeek;
	*lpFileTime = mktime(&p);
	return TRUE;
}
//---------------------------------------------------------------------------
int GetTimeFormat(LCID Locale,DWORD dwFlags,SYSTEMTIME *lpTime,LPCSTR lpFormat,LPSTR lpTimeStr,int cchTime)
{
	time_t t;
	char *p;
	struct tm t0={0};
	int i;

	t0.tm_sec = lpTime->wSecond;
	t0.tm_min = lpTime->wMinute;
	t0.tm_hour = lpTime->wHour;
	t0.tm_mday = lpTime->wDay;
	t0.tm_mon = lpTime->wMonth-1;
	t0.tm_year = lpTime->wYear - 1900;
	t0.tm_isdst = -1;
	t = mktime(&t0);
	p = ctime(&t);
	if(p == NULL)
		return 0;
	i = cchTime > 9 ? 9 : cchTime;
	lstrcpyn(lpTimeStr,&p[11],i);
	lpTimeStr[i-1] = 0;
	return i;
}
//---------------------------------------------------------------------------
int GetDateFormat(LCID Locale,DWORD dwFlags,SYSTEMTIME *lpDate,LPCSTR lpFormat,LPSTR lpDateStr,int cchDate)
{
	time_t t;
	char *p;
	struct tm t0={0};
	int i,i1;

	t0.tm_sec = lpDate->wSecond;
	t0.tm_min = lpDate->wMinute;
	t0.tm_hour = lpDate->wHour;
	t0.tm_mday = lpDate->wDay;
	t0.tm_mon = lpDate->wMonth-1;
	t0.tm_year = lpDate->wYear - 1900;
	t0.tm_isdst = -1;
	t = mktime(&t0);
	p = ctime(&t);
	if(p == NULL)
		return 0;
	i = cchDate > 11 ? 11 : cchDate;
	lstrcpyn(lpDateStr,p,i);
	cchDate -= i;
	if(cchDate < 2)
		return i;
	lpDateStr[10] = 32;
	i1 = i;
	i = cchDate > 5 ? 5 : cchDate;
	lstrcpyn(&lpDateStr[11],&p[20],i);
	i1 += i;
	lpDateStr[i1-1] = 0;
	return i1;
}
//---------------------------------------------------------------------------
HINSTANCE LoadLibrary(LPCTSTR lpLibFileName)
{
	HINSTANCE hLib;
//	char *error;

	if(lpLibFileName == NULL)
       return  NULL;
	hLib = (HINSTANCE)dlopen(lpLibFileName,RTLD_LAZY);
/*	if(hLib == NULL){
		error = dlerror();
	}*/
   return hLib;
}
//---------------------------------------------------------------------------
HINSTANCE GetModuleHandle(LPCTSTR lpLibFileName)
{
	return dlopen(lpLibFileName,RTLD_NOLOAD|RTLD_LAZY);
}
//---------------------------------------------------------------------------
BOOL FreeLibrary(HMODULE hLibModule)
{
   dlclose(hLibModule);
   return TRUE;
}
//---------------------------------------------------------------------------
FARPROC GetProcAddress(HMODULE hModule,LPCSTR lpProcName)
{
   if(hModule == NULL || lpProcName == NULL)
       return NULL;
   return (FARPROC)dlsym(hModule,lpProcName);
}
//---------------------------------------------------------------------------
BOOL GetCursorPos(LPPOINT lpPoint)
{
	GdkDisplay *display;

	if(lpPoint == NULL || (display = gdk_display_get_default()) == NULL)
		return FALSE;
	gdk_display_get_pointer(display,NULL,(gint *)&lpPoint->x,(gint *)&lpPoint->y,NULL);
	return TRUE;
}
//--------------------------------------------------------------------------------
HWND SetParent(HWND hWndChild,HWND hWndNewParent)
{
	HWND hwnd,vbox,h;
	GList *pList;
	int x,y,i;

	if(hWndChild == NULL)
		return NULL;
	hwnd = gtk_widget_get_parent(hWndChild);
	vbox = NULL;

	//gtk_widget_unparent(hWndChild);
	//gtk_widget_reparent(hWndChild,hWndNewParent);
	if(GTK_IS_CONTAINER(hWndNewParent)){
		pList = gtk_container_get_children((GtkContainer*)hWndNewParent);
		if(pList != NULL){
			x = g_list_length(pList);
			for(y=0;y<x;y++){
				h = (HWND)g_list_nth_data(pList,y);
				if(GTK_IS_VBOX(h)){
					vbox = h;
					break;
				}
			}
		}
		if(vbox != NULL){
			pList = gtk_container_get_children((GtkContainer*)vbox);
			vbox = NULL;
			if(pList != NULL){
				x = g_list_length(pList);
				y = 0;
				for(i=0;i<x;i++){
					h = (HWND)g_list_nth_data(pList,i);
					if(GTK_IS_FIXED(h)){
						vbox = h;
						break;
					}
				}
			}
		}
	}
	if(vbox == NULL)
		vbox = hWndNewParent;
	if(vbox == NULL)
		return NULL;
	gtk_widget_set_parent(hWndChild,vbox);
	return hwnd;
}
//--------------------------------------------------------------------------------
HWND GetParent(HWND hWnd)
{
	if(hWnd == NULL)
		return NULL;
	return gtk_widget_get_parent(hWnd);
}
//---------------------------------------------------------------------------
gboolean on_window_event(GtkWidget *widget,GdkEvent *event, gpointer user_data)
{
	CallWindowProc(event,widget);
	if(event->type == GDK_DELETE)
		return TRUE;
	return FALSE;
}
//---------------------------------------------------------------------------
LRESULT DefWindowProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	return 0;
}
//---------------------------------------------------------------------------
LRESULT CallWindowProc(WNDPROC lpPrevWndFunc,HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	if(lpPrevWndFunc == NULL || GetWindowLong(hWnd,GWL_WNDPROC) == (LONG)lpPrevWndFunc)
		return 0;
	return lpPrevWndFunc(hWnd,Msg,wParam,lParam);
}
//---------------------------------------------------------------------------
BOOL CallWindowProc(GdkEvent *e,GtkWidget *w)
{
	UINT uMsg;
	WPARAM wParam;
	LPARAM lParam;
	char *c;
	LPDWORD p;
	WNDPROC pWndProc;
	LRESULT res;
	GList *list;
	int i,length;

	p = (LPDWORD)g_object_get_data((GObject *)w,"UserData");
	if(p == NULL || p[GWL_WNDPROC] == 0)
		return FALSE;
	pWndProc = (WNDPROC)p[GWL_WNDPROC];
	if(pWndProc == NULL)
		return TRUE;
	switch(e->type){
		case GDK_DELETE:
			uMsg = WM_CLOSE;
			wParam = 0;
			lParam = 0;
		break;
		case GDK_VISIBILITY_NOTIFY:
			uMsg = WM_SHOWWINDOW;
			wParam = 0;
		break;
		case GDK_KEY_PRESS:
			uMsg = WM_KEYDOWN;
			wParam = KeyCodeToVK(&e->key);
			lParam = e->key.state;
			if((e->key.state & GDK_SHIFT_MASK))
				lParam |= MK_SHIFT;
			if((e->key.state & GDK_CONTROL_MASK))
				lParam |= MK_CONTROL;
		break;
		case GDK_KEY_RELEASE:
			uMsg = WM_KEYUP;
		break;
		case GDK_DESTROY:
			uMsg = WM_DESTROY;
			wParam = 0;
			lParam = 0;
		break;
		case GDK_EXPOSE:
			if(e->expose.count != 0)
				return 0;
			uMsg = WM_PAINT;
			wParam = 0;
			lParam = 0;
		break;
		case GDK_2BUTTON_PRESS:
			wParam = 0;
			uMsg = 0;
			if(e->button.button == 1){
				wParam |= MK_LBUTTON;
				uMsg = WM_LBUTTONDBLCLK;
			}
			else if(e->button.button == 2){
				wParam |= MK_MBUTTON;
				uMsg = WM_MBUTTONDBLCLK;
			}
			else if(e->button.button == 3){
				wParam |= MK_RBUTTON;
				uMsg = WM_RBUTTONDBLCLK;
			}
			if((e->button.state & GDK_SHIFT_MASK))
				wParam |= MK_SHIFT;
			if((e->button.state & GDK_CONTROL_MASK))
				wParam |= MK_CONTROL;
			lParam = (LPARAM)MAKEWPARAM((int)e->button.x,(int)e->button.y);
		break;
		case GDK_BUTTON_PRESS:
			wParam = 0;
			uMsg = 0;
			if(e->button.button == 1){
				wParam |= MK_LBUTTON;
				uMsg = WM_LBUTTONDOWN;
			}
			else if(e->button.button == 2){
				wParam |= MK_MBUTTON;
				uMsg = WM_MBUTTONDOWN;
			}
			else if(e->button.button == 3){
				wParam |= MK_RBUTTON;
				uMsg = WM_RBUTTONDOWN;
			}
			if((e->button.state & GDK_SHIFT_MASK))
				wParam |= MK_SHIFT;
			if((e->button.state & GDK_CONTROL_MASK))
				wParam |= MK_CONTROL;
			lParam = (LPARAM)MAKEWPARAM((int)e->button.x,(int)e->button.y);
		break;
		case GDK_BUTTON_RELEASE:
			wParam = 0;
			uMsg = 0;
			if(e->button.button == 1){
				wParam |= MK_LBUTTON;
				uMsg = WM_LBUTTONUP;
			}
			else if(e->button.button == 2){
				wParam |= MK_MBUTTON;
				uMsg = WM_MBUTTONUP;
			}
			else if(e->button.button == 3){
				wParam |= MK_RBUTTON;
				uMsg = WM_RBUTTONUP;
			}
			if((e->button.state & GDK_SHIFT_MASK))
				wParam |= MK_SHIFT;
			if((e->button.state & GDK_CONTROL_MASK))
				wParam |= MK_CONTROL;
			lParam = (LPARAM)MAKEWPARAM((int)e->button.x,(int)e->button.y);
		break;
		case GDK_MOTION_NOTIFY:
			wParam = 0;
			uMsg = WM_MOUSEMOVE;
			if(e->motion.state & GDK_BUTTON1_MASK)
				wParam |= MK_LBUTTON;
			else if(e->motion.state & GDK_BUTTON2_MASK)
				wParam |= MK_MBUTTON;
			else if(e->motion.state & GDK_BUTTON3_MASK)
				wParam |= MK_RBUTTON;
			if((e->motion.state & GDK_SHIFT_MASK))
				wParam |= MK_SHIFT;
			if((e->motion.state & GDK_CONTROL_MASK))
				wParam |= MK_CONTROL;
			lParam = (LPARAM)MAKEWPARAM((int)e->motion.x,(int)e->motion.y);
		break;
		case GDK_CONFIGURE:
			if(GTK_IS_DIALOG(w) && GetWindowLong(w,GWL_STATE) == 0){
				pWndProc(w,WM_INITDIALOG,NULL,(LPARAM)GetWindowLong(w,GWL_LPARAM));
				SetWindowLong(w,GWL_STATE,1);
			if(GTK_IS_CONTAINER(w)){
				//gtk_container_check_resize((GtkContainer *)w);
				gdk_flush();
			}

				return TRUE;
			}
			pWndProc(w,WM_SIZE,0,MAKELPARAM(e->configure.width,e->configure.height));
			uMsg = WM_MOVE;
			wParam = 0;
			lParam = (LPARAM)MAKEWPARAM(e->configure.x,e->configure.y);
		break;
        case GDK_SCROLL:
            uMsg = WM_MOUSEWHEEL;
            switch(e->scroll.direction){
                case GDK_SCROLL_DOWN:
                    wParam = -(WHEEL_DELTA << 16);
                break;
                case GDK_SCROLL_UP:
                    wParam = WHEEL_DELTA<<16;
                break;
                default:
                    return TRUE;
            }
            lParam = 0;
        break;
		default:
			return 0;
	}
	res = pWndProc(w,uMsg,wParam,lParam);
	list = (GList *)GetWindowLong(w,GWL_MESSAGES);
	if(list != NULL){
		length = g_list_length(list);
		for(i=0;i<length;i++){
			p = (LPDWORD)g_list_nth_data(list,i);
			pWndProc(w,p[0],p[1],p[2]);
			delete p;
		}
		g_list_free(list);
		SetWindowLong(w,GWL_MESSAGES,0);
	}
	return TRUE;
}
//---------------------------------------------------------------------------
void gtk_widget_subclass(GtkWidget *widget)
{
	if(!GTK_WIDGET_TOPLEVEL(widget)){
		//XSelectInput(gdk_x11_drawable_get_xdisplay(widget->window),gdk_x11_drawable_get_xid(widget->window),0x3FFFFF);
		g_signal_connect((gpointer)widget,"event",G_CALLBACK(on_window_event),NULL);
	}
}
//---------------------------------------------------------------------------
static HPROPSHEETPAGE CreatePropertySheetPage(LPCPROPSHEETPAGE page,DLG_INFO *pPage)
{
   DLG_INFO dlgInfo;
   WORD *info;
   void *hRsrc;
   HWND fixed,hwnd,hwnd1;
	gint i,xBaseUnit,yBaseUnit;
	DLG_CONTROL_INFO ctrlInfo,ctrlInfoCopy;
	char temp[300];

   	if(page == NULL)
       return 0;
   	if(!(hRsrc = FindResource(page->hInstance,page->pszTemplate,RT_DIALOG)))
       return 0;
   	if((info = (LPWORD)LockResource(hRsrc)) == NULL)
	    return 0;
   	info = (WORD *)DIALOG_Get32((const WORD *)info,&dlgInfo);
	fixed = gtk_fixed_new();
	temp[0] = 0;
	if(page->dwFlags & PSP_USETITLE)
		lstrcpy(temp,page->pszTitle);
	else
		WideCharToMultiByte(0,0,dlgInfo.caption,-1,temp,300,NULL,FALSE);
	gtk_widget_set_name(fixed,temp);
	gtk_widget_show (fixed);
	xBaseUnit = GetDialogBaseUnits();
	yBaseUnit = xBaseUnit >> 16;
	xBaseUnit &= 0xFFFF;
	hwnd1 = NULL;
	for(i=0;i<dlgInfo.nItems;i++){
		info = (LPWORD)DIALOG_GetControl32(info,&ctrlInfo,dlgInfo.dialogEx);
		hwnd = DIALOG_CreateControl(&ctrlInfo,xBaseUnit,yBaseUnit,NULL);
		if(hwnd != NULL){
            if(GTK_IS_SPIN_BUTTON(hwnd) && (ctrlInfo.style & UDS_AUTOBUDDY)){
				if(hwnd1 != NULL && GTK_IS_ENTRY(hwnd1)){
					if(fixed != NULL){
						memcpy(&ctrlInfo,&ctrlInfoCopy,sizeof(DLG_CONTROL_INFO));
						gtk_widget_set_size_request(hwnd,ctrlInfo.cx*xBaseUnit/4,ctrlInfo.cy*yBaseUnit/8);
						SetWindowLong(hwnd,GWL_ID2,GetWindowLong(hwnd1,GWL_ID));
						if(ctrlInfo.style & WS_DISABLED)
							gtk_widget_set_sensitive(hwnd,false);
						gtk_container_remove((GtkContainer *)fixed,hwnd1);
					}
				}
			}
			gtk_fixed_put((GtkFixed *)fixed,hwnd,ctrlInfo.x*xBaseUnit/4,ctrlInfo.y*yBaseUnit/8);
			hwnd1 = hwnd;
			memcpy(&ctrlInfoCopy,&ctrlInfo,sizeof(DLG_CONTROL_INFO));
		}
	}
	if(pPage !=  NULL)
		memcpy(pPage,&dlgInfo,sizeof(DLG_INFO));
	SetWindowLong(fixed,GWL_WNDPROC,(LONG)page->pfnDlgProc);
	SetWindowLong(fixed,GWL_LPARAM,(LONG)page->lParam);
   	SetWindowLong(fixed,GWL_ID,(LONG)page->pszTemplate);
   	return fixed;
}
//---------------------------------------------------------------------------
HPROPSHEETPAGE CreatePropertySheetPage(LPCPROPSHEETPAGE page)
{
	return CreatePropertySheetPage(page,NULL);
}
//---------------------------------------------------------------------------
BOOL DestroyPropertySheetPage(HPROPSHEETPAGE)
{
	return FALSE;
}
//---------------------------------------------------------------------------
static void OnChangePage(GtkNotebook *notebook,GtkNotebookPage *page,guint page_num,gpointer user_data)
{
	HWND hwnd,hwnd1;
	DLGPROC pProc,pProc1;
	PROPSHEETPAGE p;
   NMHDR hdr;
   gint i;

	hwnd = gtk_notebook_get_nth_page(notebook,page_num);
	if(hwnd == NULL)
		return;
	pProc = (DLGPROC)GetWindowLong(hwnd,GWL_WNDPROC);
	if(pProc == NULL)
		return;
	if(GetWindowLong(hwnd,GWL_STATE) == 0){
		ZeroMemory(&p,sizeof(p));
		p.lParam = GetWindowLong(hwnd,GWL_LPARAM);
		pProc(hwnd,WM_INITDIALOG,0,(LPARAM)&p);
		SetWindowLong(hwnd,GWL_STATE,1);
	}
   i = gtk_notebook_get_current_page(notebook);
   if(i != page_num){
       hwnd1 = gtk_notebook_get_nth_page(notebook,i);
       if(hwnd1 != NULL && (pProc1 = (DLGPROC)GetWindowLong(hwnd1,GWL_WNDPROC)) != NULL){
           hdr.code = PSN_KILLACTIVE;
           hdr.hwndFrom = hwnd1;
           hdr.idFrom = GetWindowLong(hwnd1,GWL_ID);
           pProc1(hwnd1,WM_NOTIFY,(WPARAM)hdr.idFrom,(LPARAM)&hdr);
       }
   }
   hdr.code = PSN_SETACTIVE;
   hdr.hwndFrom = hwnd;
   hdr.idFrom = GetWindowLong(hwnd,GWL_ID);
   pProc(hwnd,WM_NOTIFY,(WPARAM)hdr.idFrom,(LPARAM)&hdr);
}
//---------------------------------------------------------------------------
static LRESULT WndProcPropertySheet(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	HWND label;

	switch(uMsg){
		case PSM_ADDPAGE:
			gtk_widget_show((HWND)lParam);
  			gtk_container_add(GTK_CONTAINER(hwnd),(HWND)lParam);
			set_DialogNotifyParent((HWND)lParam);
			label = gtk_label_new (gtk_widget_get_name((HWND)lParam));
  			gtk_widget_show(label);
  			gtk_notebook_set_tab_label(GTK_NOTEBOOK (hwnd),gtk_notebook_get_nth_page(GTK_NOTEBOOK(hwnd),gtk_notebook_page_num(GTK_NOTEBOOK(hwnd),(HWND)lParam)),label);
		break;
	}
	return 0;
}
//---------------------------------------------------------------------------
int PropertySheet(LPCPROPSHEETHEADER hdr)
{
	HWND hDlg,hTabControl,page,label,vbox;
	UINT i,i1;
	DLG_INFO dlgInfo;
	char temp[300];
   DLGPROC pProc;
   NMHDR h;

	hDlg = gtk_dialog_new_with_buttons(hdr->pszCaption,NULL,(GtkDialogFlags)(GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT),
		"Ok",GTK_RESPONSE_ACCEPT,"Cancel",GTK_RESPONSE_REJECT,NULL);
   if(hDlg == NULL)
       return -1;
	if(hdr->hwndParent != NULL){
		//gtk_widget_set_parent(hDlg,hdr->hwndParent);
		gtk_window_set_transient_for(GTK_WINDOW(hDlg),GTK_WINDOW(hdr->hwndParent));
	}
	hTabControl = gtk_notebook_new();
	gtk_window_set_resizable (GTK_WINDOW (hDlg), FALSE);
	vbox = GTK_DIALOG (hDlg)->vbox;
	gtk_box_pack_start(GTK_BOX (vbox),(HWND)hTabControl,FALSE,FALSE,0);
	for(i=0;i<hdr->nPages;i++){
		page = CreatePropertySheetPage(&hdr->ppsp[i],&dlgInfo);
		if(page == NULL)
			continue;
		gtk_widget_show(page);
  		gtk_container_add(GTK_CONTAINER(hTabControl),page);
		if(hdr->ppsp[i].pszTitle != NULL)
  			label = gtk_label_new(hdr->ppsp[i].pszTitle);
		else{
			temp[0] = 0;
			WideCharToMultiByte(0,0,dlgInfo.caption,-1,temp,300,NULL,FALSE);
			label = gtk_label_new(gtk_widget_get_name(page));
		}
  		gtk_widget_show(label);
  		gtk_notebook_set_tab_label(GTK_NOTEBOOK(hTabControl),gtk_notebook_get_nth_page(GTK_NOTEBOOK(hTabControl),i),label);
		set_DialogNotifyParent(page);
	}
	if((hdr->dwFlags & PSH_USECALLBACK) != 0 && hdr->pfnCallback != NULL)
		hdr->pfnCallback((HWND)hTabControl,PSCB_INITIALIZED,0);
	SetWindowLong(hTabControl,GWL_WNDPROC,(LONG)WndProcPropertySheet);
	gtk_widget_show((HWND)hTabControl);
	gtk_widget_show(hDlg);
	g_signal_connect((gpointer)hTabControl,"switch-page",G_CALLBACK(OnChangePage),(gpointer)0);
	gtk_notebook_set_current_page((GtkNotebook *)hTabControl,0);
	OnChangePage((GtkNotebook *)hTabControl,NULL,0,0);
	if(gtk_dialog_run(GTK_DIALOG(hDlg)) == GTK_RESPONSE_ACCEPT){
   		i = gtk_notebook_get_current_page((GtkNotebook *)hTabControl);
   		page = gtk_notebook_get_nth_page((GtkNotebook *)hTabControl,i);
       	if(page != NULL && (pProc = (DLGPROC)GetWindowLong((HWND)page,GWL_WNDPROC)) != NULL){
        	h.code = PSN_KILLACTIVE;
           	h.hwndFrom = page;
           	h.idFrom = GetWindowLong(page,GWL_ID);
           	pProc(page,WM_NOTIFY,(WPARAM)h.idFrom,(LPARAM)&h);
		}
       i = gtk_notebook_get_n_pages(GTK_NOTEBOOK (hTabControl));
       h.code = PSN_APPLY;
       for(i1=0;i1<i;i1++){
           page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(hTabControl),i1);
           if(page == NULL || (pProc = (DLGPROC)GetWindowLong(page,GWL_WNDPROC)) == NULL)
               continue;
			if(!GetWindowLong(page,GWL_STATE))
				continue;
           h.hwndFrom = page;
           h.idFrom = GetWindowLong(page,GWL_ID);
           pProc(page,WM_NOTIFY,(WPARAM)h.idFrom,(LPARAM)&h);
       }
   }
	gtk_widget_destroy(hDlg);
	return 1;
}
//---------------------------------------------------------------------------
BOOL IsDialogMessage(HWND hDlg,LPMSG lpMsg)
{
	return FALSE;
}
//---------------------------------------------------------------------------
long GetDialogBaseUnits()
{
	gint xBaseUnit,yBaseUnit;
	PangoContext *context;
	PangoFontDescription *font_desc;
	PangoFontMetrics *font_metrics;

	context = gdk_pango_context_get();
	font_desc = pango_context_get_font_description(context);
	font_metrics = pango_context_get_metrics(context,font_desc,NULL);
	yBaseUnit = floor((pango_font_metrics_get_descent(font_metrics) + pango_font_metrics_get_ascent(font_metrics)) / PANGO_SCALE);
	xBaseUnit = floor(pango_font_metrics_get_approximate_digit_width(font_metrics) / PANGO_SCALE);
	g_object_unref(context);
	return (long)MAKELONG(xBaseUnit,yBaseUnit);
}
//---------------------------------------------------------------------------
HWND GetNextDlgTabItem(HWND hDlg,HWND hCtl,BOOL bPrevious)
{
	return NULL;
}
//---------------------------------------------------------------------------
LPITEMIDLIST SHBrowseForFolder(LPBROWSEINFO lpbi)
{
	HWND w;
	char *filename;
	LPITEMIDLIST res;

	res = NULL;
	w = gtk_file_chooser_dialog_new(lpbi->lpszTitle,(GtkWindow *)lpbi->hwndOwner,GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_widget_show(w);
	if(gtk_dialog_run((GtkDialog *)w) == GTK_RESPONSE_ACCEPT){
    	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (w));
		res = (LPITEMIDLIST)new	char[lstrlen(filename) + sizeof(ITEMIDLIST)];
		res->mkid.cb = lstrlen(filename);
		lstrcpy((char *)res->mkid.abID,filename);
		g_free (filename);
	}
	gtk_widget_destroy(w);
	return res;
}
//---------------------------------------------------------------------------
BOOL SHGetPathFromIDList(LPCITEMIDLIST pidl,LPSTR pszPath)
{
	if(pidl == NULL || pszPath == NULL)
		return FALSE;
	lstrcpy(pszPath,(char *)pidl->mkid.abID);
	delete pidl;
}
//---------------------------------------------------------------------------
int GetKeyNameText(LONG lParam,LPSTR lpString,int nSize)
{
	gchar *p;

	if(lpString == NULL)
		return 0;
	p = gdk_keyval_name(VKToKeyCodeTo(lParam>>16));
	if(p == NULL)
		return 0;
	lstrcpyn(lpString,p,nSize);
	return lstrlen(lpString);
}
//---------------------------------------------------------------------------
BOOL AdjustWindowRect(LPRECT lpRect,DWORD dwStyle,BOOL bMenu)
{
	HWND w,vbox,drawingarea,item,menu;
	RECT rc;

	if(dwStyle & WS_CHILD)
		return FALSE;
	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if(w == NULL)
		return FALSE;
	menu = NULL;
	gtk_window_set_decorated(GTK_WINDOW(w),(dwStyle & WS_BORDER ? TRUE : FALSE));
	vbox = gtk_vbox_new(FALSE,0);
	if(vbox == NULL){
		gtk_widget_destroy(w);
		return FALSE;
	}
	gtk_container_add(GTK_CONTAINER(w),vbox);
	gtk_widget_show(vbox);

	if(bMenu){
		menu = gtk_menu_bar_new();
		gtk_widget_show((HWND)menu);
		item = gtk_menu_item_new_with_label("iDeaS");
		gtk_widget_show((HWND)item);
		gtk_container_add((GtkContainer *)menu,item);
  		gtk_box_pack_start(GTK_BOX(vbox),(HWND)menu,FALSE,FALSE,0);
	}
 	drawingarea = gtk_drawing_area_new();
  	gtk_widget_set_size_request(drawingarea,lpRect->right,lpRect->bottom);
  	gtk_box_pack_start(GTK_BOX(vbox),drawingarea,TRUE,TRUE,0);
  	gtk_widget_show(drawingarea);
	gtk_window_set_keep_below(GTK_WINDOW(w),true);
	//gtk_widget_show_now(w);
	gtk_widget_map(w);
	gtk_widget_realize(w);
	SetWindowStyle(w,dwStyle);

	GetWindowRect(w,&rc);
    lpRect->left = -((rc.right - rc.left) - lpRect->right) >> 1;
    lpRect->right = (rc.right - rc.left) + lpRect->left;
    lpRect->top = -((rc.bottom - rc.top) - lpRect->bottom) >> 1;
    lpRect->bottom = (rc.bottom - rc.top) + lpRect->top;
	gtk_widget_destroy(w);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL IsWindow(HWND hWnd)
{
	if(hWnd == NULL || hWnd->window == 0)
		return FALSE;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL InvalidateRect(HWND hWnd,CONST RECT *lpRect,BOOL bErase)
{
	if(hWnd == NULL || hWnd->window == 0)
		return FALSE;
	gdk_window_invalidate_rect(hWnd->window,(GdkRectangle *)lpRect,TRUE);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL RedrawWindow(HWND hWnd,CONST RECT *lprcUpdate,HRGN hrgnUpdate,UINT flags)
{
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL UpdateWindow(HWND hWnd)
{
	gtk_widget_queue_draw(hWnd);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL BringWindowToTop(HWND hWnd)
{
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL SetForegroundWindow(HWND hWnd)
{
	if(hWnd == NULL)
		return FALSE;
	if(GTK_IS_WINDOW(hWnd))
		gtk_window_present((GtkWindow *)hWnd);
	return TRUE;
}
//---------------------------------------------------------------------------
HWND GetCapture()
{
	return NULL;
}
//---------------------------------------------------------------------------
HWND SetCapture(HWND hWnd)
{
	if(hWnd == NULL || hWnd->window == 0)
		return NULL;
	/*gdk_pointer_grab(hWnd->window,FALSE,(GdkEventMask)(GDK_POINTER_MOTION_MASK|
		GDK_BUTTON_MOTION_MASK|GDK_BUTTON1_MOTION_MASK|GDK_BUTTON2_MOTION_MASK|
		GDK_BUTTON3_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK)
		,NULL,NULL,GDK_CURRENT_TIME);*/
	return hWnd;
}
//---------------------------------------------------------------------------
BOOL ReleaseCapture()
{
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
	return TRUE;
}
//---------------------------------------------------------------------------
static void OnEnumChildBeginPaint(GtkWidget *widget,gpointer data)
{
   LPDWORD p;
	GdkRegion *rgn;
	GdkRectangle rect;

	p = (LPDWORD)data;
	if(p[0] == 0)
       return;
	if(GTK_IS_BOX(widget) || GTK_IS_FIXED(widget)){
		gtk_container_foreach((GtkContainer *)widget,OnEnumChildBeginPaint,data);
	}
	else{
		memcpy(&rect,&widget->allocation,sizeof(GdkRectangle));
		rgn = gdk_region_rectangle((GdkRectangle *)&rect);
		if(rgn == NULL)
			return;
		gdk_region_subtract((GdkRegion *)p[0],rgn);
		gdk_region_destroy(rgn);
	}
}
//---------------------------------------------------------------------------
HDC BeginPaint(HWND hWnd,LPPAINTSTRUCT lpPaint)
{
	GdkRegion *rgn;
	GdkRectangle rect;
	DWORD data[2];

	rgn = gdk_window_get_update_area(hWnd->window);
	if(rgn == NULL)
		rgn = gdk_drawable_get_visible_region(hWnd->window);
	if(rgn != NULL){
		if(GTK_WIDGET_TOPLEVEL(hWnd)){
			data[0] = (DWORD)rgn;
			gtk_container_foreach((GtkContainer *)hWnd,OnEnumChildBeginPaint,data);
		}
		gdk_region_get_clipbox(rgn,&rect);
	}
	lpPaint->rcPaint.left = 0;//rect.x;
	lpPaint->rcPaint.top = 0;//rect.y;
	lpPaint->rcPaint.right = rect.width;
	lpPaint->rcPaint.bottom = rect.height;
	gdk_window_freeze_updates(hWnd->window);
	gdk_window_begin_paint_region(hWnd->window,rgn);
	if((lpPaint->hdc = GetDC(hWnd)) != NULL)
		cairo_translate(lpPaint->hdc,rect.x,rect.y);
	gdk_region_destroy(rgn);
	return lpPaint->hdc;
}
//---------------------------------------------------------------------------
BOOL EndPaint(HWND hWnd,CONST PAINTSTRUCT *lpPaint)
{
	if(lpPaint != NULL && lpPaint->hdc != NULL)
		ReleaseDC(hWnd,lpPaint->hdc);
	gdk_window_end_paint(hWnd->window);
	gdk_window_thaw_updates(hWnd->window);
	return TRUE;
}
//---------------------------------------------------------------------------
int SetScrollPos(HWND hWnd,int nBar,int nPos,BOOL bRedraw)
{
	if(hWnd == NULL)
		return 0;
	switch(nBar){
		case SB_CTL:
			if(!GTK_IS_SCROLLBAR(hWnd))
				hWnd = gtk_bin_get_child((GtkBin *)hWnd);
		break;
	}
	gtk_range_set_value((GtkRange *)hWnd,nPos);
	return 0;
}
//---------------------------------------------------------------------------
int GetScrollPos(HWND hWnd,int nBar)
{
	if(hWnd == NULL)
		return 0;
	return (int)gtk_range_get_value((GtkRange *)hWnd);
}
//---------------------------------------------------------------------------
int SetScrollInfo(HWND hWnd,int nBar,LPCSCROLLINFO lpsi,BOOL bRedraw)
{
	int nMax;
	gdouble dMax;

	if(hWnd == NULL || lpsi == NULL)
		return 0;
	switch(nBar){
		case SB_CTL:
			if(!GTK_IS_SCROLLBAR(hWnd))
				hWnd = gtk_bin_get_child((GtkBin *)hWnd);
		break;
		default:
			hWnd = NULL;
		break;
	}
	if(hWnd == NULL)
		return 0;
	if(lpsi->fMask & SIF_PAGE)
		gtk_range_set_increments((GtkRange *)hWnd,1,lpsi->nPage);
	if(lpsi->fMask & SIF_RANGE){
		nMax = lpsi->nMax;
		if(lpsi->fMask & SIF_PAGE)
			nMax -= lpsi->nPage;
		else{
			dMax = 0;
			g_object_get((gpointer)gtk_range_get_adjustment((GtkRange *)hWnd),"page-increment",&dMax,NULL);
			nMax -= dMax;
		}
		gtk_range_set_range((GtkRange *)hWnd,lpsi->nMin,nMax);
	}
	if(lpsi->fMask & SIF_POS)
		gtk_range_set_value((GtkRange *)hWnd,lpsi->nPos);
	gtk_widget_queue_draw(hWnd);
	return 1;
}
//---------------------------------------------------------------------------
BOOL GetScrollInfo(HWND hWnd,int nBar,LPSCROLLINFO lpsi)
{
	gdouble d,d1,d2,d3;
	GtkAdjustment *control;

	if(hWnd == NULL || lpsi == NULL)
		return FALSE;
	if(!GTK_IS_SCROLLBAR(hWnd))
		hWnd = gtk_bin_get_child((GtkBin *)hWnd);
	if((control = gtk_range_get_adjustment((GtkRange *)hWnd)) == NULL)
		return FALSE;
	g_object_get((gpointer)control,"lower",&d,"upper",&d1,"value",&d2,"page-increment",&d3,NULL);
	if(lpsi->fMask & SIF_RANGE){
		lpsi->nMin = (int)d;
		lpsi->nMax = (int)d1;
	}
	if(lpsi->fMask & SIF_POS)
		lpsi->nPos = (int)d2;
	if(lpsi->fMask & SIF_PAGE)
		lpsi->nPage = (int)d3;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL OpenClipboard(HWND hWndNewOwner)
{
	if(hWndNewOwner == NULL)
		return FALSE;
	clipboard = gtk_widget_get_clipboard(hWndNewOwner,GDK_SELECTION_CLIPBOARD);
	return (BOOL)(clipboard != NULL ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
BOOL CloseClipboard()
{
	if(clipboard == NULL)
		return FALSE;
	clipboard = NULL;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL EmptyClipboard()
{
	if(clipboard == NULL)
		return FALSE;
	gtk_clipboard_clear(clipboard);
	return TRUE;
}
//---------------------------------------------------------------------------
HANDLE SetClipboardData(UINT uFormat,HANDLE hMem)
{
	if(clipboard == NULL || hMem == NULL)
		return NULL;
	switch(uFormat){
		case CF_TEXT:
			gtk_clipboard_set_text(clipboard,(gchar *)hMem,-1);
		break;
	}
	return NULL;
}
//---------------------------------------------------------------------------
BOOL DestroyCaret()
{
	LCaret *caret;

	caret = currentCaret;
	if(caret == NULL)
		return FALSE;
	SetWindowLong(caret->Window(),GWL_CARET,0);
	delete caret;
	currentCaret =  NULL;
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL HideCaret(HWND hWnd)
{
	LCaret *caret;

	if(hWnd == NULL)
		return FALSE;
	caret = (LCaret *)GetWindowLong(hWnd,GWL_CARET);
	currentCaret = caret;
	if(caret == NULL)
		return FALSE;
	caret->Show(hWnd,FALSE);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL ShowCaret(HWND hWnd)
{
	LCaret *caret;

	if(hWnd == NULL)
		return FALSE;
	caret = (LCaret *)GetWindowLong(hWnd,GWL_CARET);
	currentCaret = caret;
	if(caret == NULL)
		return FALSE;
	caret->Show(hWnd,TRUE);
	return TRUE;
}
//---------------------------------------------------------------------------
static void OnSearchActiveWindow(GtkWidget *widget,gpointer data)
{
	LPDWORD p;

	if(data == NULL)
		return;
	p = (LPDWORD)data;
	if(p[1] != 0)
		return;
	if(widget->window == (GdkWindow *)p[0])
		p[1] = (DWORD)widget;
}
//---------------------------------------------------------------------------
HWND GetActiveWindow()
{
	GtkWindow *window;
	GdkWindow *win;
	GList *p;
	int i,length;
	DWORD dw[2];

	win = gdk_screen_get_active_window(gdk_screen_get_default());
	if(win == NULL)
		return NULL;
	p = gtk_window_list_toplevels();
	if(p == NULL)
		return FALSE;
	length = g_list_length(p);
	for(i=0;i<length;i++){
		window = (GtkWindow *)g_list_nth_data(p,i);
		if(window == NULL)
			continue;
		dw[0] = (DWORD)win;
		dw[1] = 0;
		gtk_container_forall((GtkContainer *)window,OnSearchActiveWindow,(gpointer)dw);
		if(dw[1] != 0){
			if(GTK_IS_WINDOW((HWND)dw[1]))
				return gtk_window_get_focus((GtkWindow *)dw[1]);
			else if(GTK_IS_CONTAINER((HWND)dw[1])){
				dw[1] = (DWORD)((GtkContainer *)dw[1])->focus_child;
				if(GTK_IS_CONTAINER((HWND)dw[1]))
					return ((GtkContainer *)dw[1])->focus_child;
			}
			return (HWND)dw[1];
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
BOOL SetCaretPos(int X,int Y)
{
	LCaret *caret;

	caret = currentCaret;
	if(caret == NULL)
		return FALSE;
	caret->Pos(X,Y);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL CreateCaret(HWND hWnd,HBITMAP hBitmap,int nWidth,int nHeight)
{
	LCaret *caret;

	if(hWnd == NULL)
		return FALSE;
	caret = (LCaret *)GetWindowLong(hWnd,GWL_CARET);
	if(caret == NULL){
		caret = new LCaret();
		if(caret == NULL)
			return FALSE;
		if(!caret->Create(hWnd,hBitmap,nWidth,nHeight)){
			delete caret;
			return FALSE;
		}
		SetWindowLong(hWnd,GWL_CARET,(LONG)caret);
		currentCaret = caret;
	}
	return TRUE;
}
//---------------------------------------------------------------------------
int TranslateAccelerator(HWND hWnd,HACCEL hAccTable,LPMSG lpMsg)
{
	DWORD i,nCount;
    WORD mask;
	LPPE_ACCEL p;
	int key;

   	if(hAccTable == NULL || lpMsg->type != GDK_KEY_PRESS || lpMsg->any.window != hWnd->window)
   		return FALSE;
	nCount = LocalSize(hAccTable) / sizeof(PE_ACCEL);
	key = KeyCodeToVK(&lpMsg->key);
	mask = 0;
	if(lpMsg->key.state & GDK_SHIFT_MASK)
		mask |= FSHIFT;
	if(lpMsg->key.state & GDK_CONTROL_MASK)
		mask |= FCONTROL;
	if(lpMsg->key.state & GDK_MOD1_MASK)
		mask |= FALT;
   	for(i=0,p=(LPPE_ACCEL)hAccTable;i<nCount;i++,p++){
		if(p->key != key || (p->fVirt & mask) != (p->fVirt & 0x1C))
			continue;
		SendMessage(hWnd,WM_COMMAND,p->cmd,0);
    	return TRUE;
    }
    return FALSE;
}
//---------------------------------------------------------------------------
BOOL DestroyAcceleratorTable(HACCEL hAccel)
{
	if(LocalFree(hAccel) == NULL)
		return TRUE;
	return FALSE;
}
//---------------------------------------------------------------------------
HACCEL CreateAcceleratorTable(LPACCEL accel,int cAccelEntries)
{
	LPPE_ACCEL p2;
	int i;

	if(accel == NULL || cAccelEntries < 1)
		return NULL;
	p2 = (LPPE_ACCEL)LocalAlloc(LPTR,sizeof(PE_ACCEL) * cAccelEntries);
	if(p2 == NULL)
		return NULL;
	for(i=0;i<cAccelEntries;i++){
		p2[i].fVirt = accel[i].fVirt;
		p2[i].key = accel[i].key;
		p2[i].cmd = accel[i].cmd;
	}
	return p2;
}
//---------------------------------------------------------------------------
HACCEL LoadAccelerators(HINSTANCE hInstance,LPCSTR lpTableName)
{
	void *p,*p1;
	DWORD size;
	LPPE_ACCEL p2;

	p = FindResource(NULL,lpTableName,(char *)RT_ACCELERATOR);
	if(p == NULL)
		return NULL;
	p1 = LockResource(p);
	size = SizeofResource(p) / sizeof(PE_ACCEL);
	if(size == 0)
		return NULL;
	p2 = (LPPE_ACCEL)LocalAlloc(LPTR,sizeof(PE_ACCEL) * size);
	if(p2 == NULL)
		return NULL;
	memcpy(p2,p1,size * sizeof(PE_ACCEL));
	return (HACCEL)p2;
}
//---------------------------------------------------------------------------
int CopyAcceleratorTable(HACCEL hAccelSrc,LPACCEL lpAccelDst,int cAccelEntries)
{
	int nCount,i;

	if(hAccelSrc == NULL)
   		return NULL;
   	nCount = LocalSize(hAccelSrc) / sizeof(PE_ACCEL);
	if(lpAccelDst == NULL)
   		return nCount;
   	if(cAccelEntries > nCount)
	   	cAccelEntries = nCount;
	for(i=0;i<cAccelEntries;i++){
		lpAccelDst[i].fVirt = ((LPPE_ACCEL)hAccelSrc)[i].fVirt;
		lpAccelDst[i].key = ((LPPE_ACCEL)hAccelSrc)[i].key;
		lpAccelDst[i].cmd = ((LPPE_ACCEL)hAccelSrc)[i].cmd;
	}
   	return cAccelEntries;
}
//---------------------------------------------------------------------------
HFONT CreateFontIndirect(LOGFONT *lf)
{
	PangoFontDescription *font_desc;
	GdiObject *obj;

	obj = gdiobject_new_font();
	pango_font_description_set_family((PangoFontDescription *)obj->object,lf->lfFaceName);
	pango_font_description_set_size((PangoFontDescription *)obj->object,lf->lfHeight);
	return obj;
}
//---------------------------------------------------------------------------
HGDIOBJ SelectObject(HDC hDC,HGDIOBJ hObj)
{
	LPDWORD data;
	DWORD old_data;
	PangoContext *context;
	PangoFontDescription *font_desc;
	cairo_font_options_t *cairo_context;
	double dpi;
	GdiObject *object;

	if(hObj == NULL || hDC == NULL)
		return NULL;
	old_data = 0;
	data = (LPDWORD)cairo_get_user_data(hDC,&cr_data);
	context = gdk_pango_context_get();
	dpi = pango_cairo_context_get_resolution(context);

	if(GDI_IS_OBJECT(hObj)) {
		object = (GdiObject *)hObj;
		switch(((GdiObject *)hObj)->type){
			case GDI_FONT:
				font_desc = (PangoFontDescription *)object->object;
				cairo_select_font_face(hDC,pango_font_description_get_family(font_desc),
					CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
				cairo_set_font_size(hDC,pango_font_description_get_size(font_desc)*dpi/72/PANGO_SCALE);
			break;
		}
	}
	else if(GDK_IS_PIXBUF(hObj)){
		old_data = data[3];
		data[3] = (DWORD)hObj;
	}
	return (HGDIOBJ)old_data;
}
//---------------------------------------------------------------------------
COLORREF SetBkColor(HDC hDC,COLORREF color)
{
	LPDWORD data;

	if(hDC == NULL)
		return 0;
	data = (LPDWORD)cairo_get_user_data(hDC,&cr_data);
	if(data == NULL)
		return 0;
	data[1] = color;
	return 0;
}
//---------------------------------------------------------------------------
COLORREF SetTextColor(HDC hDC,COLORREF color)
{
	LPDWORD data;

	if(hDC == NULL)
		return 0;
	data = (LPDWORD)cairo_get_user_data(hDC,&cr_data);
	if(data == NULL)
		return 0;
	data[2] = color;
	return 0;
}
//---------------------------------------------------------------------------
int SetBkMode(HDC hDC,int mode)
{
	LPDWORD data;

	if(hDC == NULL)
		return 0;
	data = (LPDWORD)cairo_get_user_data(hDC,&cr_data);
	if(data == NULL)
		return 0;
	data[0] = mode;
	return 0;
}
//---------------------------------------------------------------------------
HBRUSH CreateSolidBrush(COLORREF color)
{
	double r,g,b;

	b = ((BYTE)color) / 255.0;
	g = ((BYTE)(color >> 8)) / 255.0;
	r = ((BYTE)(color >> 16)) / 255.0;
	return gdiobject_new_solid_brush(r,g,b);
}
//---------------------------------------------------------------------------
HBRUSH GetSysColorBrush(int nIndex)
{
	static GdiObject *system_resource[80]={0};
	GdiObject *obj;
	double r,g,b;
	DWORD color;

	if(system_resource[nIndex] == NULL){
		color = GetSysColor(nIndex);
		b = ((BYTE)color) / 255.0;
		g = ((BYTE)(color >> 8)) / 255.0;
		r = ((BYTE)(color >> 16)) / 255.0;
		system_resource[nIndex] = gdiobject_new_solid_brush(r,g,b);
	}
	return system_resource[nIndex];
}
//---------------------------------------------------------------------------
int GetDeviceCaps(HDC hDC,int nValue)
{
	switch(nValue){
		case BITSPIXEL:
			return 32;
	}
}
//---------------------------------------------------------------------------
DWORD GetSysColor(int nIndex)
{
	GtkStyle *style;
	DWORD color;

	style = gtk_widget_get_default_style();
	switch(nIndex){
		case COLOR_WINDOW:
			color = style->base[0].blue >> 8;
			color |= (style->base[0].green >> 8) << 8;
			color |= (style->base[0].red >> 8)<<16;
		break;
		case COLOR_WINDOWTEXT:
			color = style->text[0].blue >> 8;
			color |= (style->text[0].green >> 8) << 8;
			color |= (style->text[0].red >> 8)<<16;
		break;
		case COLOR_HIGHLIGHT:
			color = style->base[3].blue >> 8;
			color |= (style->base[3].green >> 8) << 8;
			color |= (style->base[3].red >> 8)<< 16;
		break;
		case COLOR_HIGHLIGHTTEXT:
			color = style->text[3].blue >> 8;
			color |= (style->text[3].green >> 8) << 8;
			color |= (style->text[3].red >> 8)<<16;
		break;
		case COLOR_MENU:
			color = style->mid[0].blue >> 8;
			color |= (style->mid[0].green >> 8) << 8;
			color |= (style->mid[0].red >> 8)<<16;
		break;
		default:
			color = 0xFFFFFFFF;
		break;
	}
	return color;
}
//---------------------------------------------------------------------------
UINT SetTextAlign(HDC hDC,UINT fMode)
{
	LPDWORD data;

	if(hDC == NULL)
		return 0;
	data = (LPDWORD)cairo_get_user_data(hDC,&cr_data);
	if(data == NULL)
		return 0;
	data[2] = fMode;
	return 0;
}
//---------------------------------------------------------------------------
int FillRect(HDC hDC,CONST RECT *lprc,HBRUSH hbr)
{
	double r,g,b;

	if(hDC == NULL)
		return 0;
	cairo_save(hDC);
	r = g = b = 1;
	if(hbr != NULL)
		cairo_pattern_get_rgba((cairo_pattern_t *)((GdiObject *)hbr)->object,&r,&g,&b,NULL);
	cairo_set_source_rgba(hDC,r,g,b,1);
	cairo_rectangle(hDC,lprc->left,lprc->top,lprc->right-lprc->left,lprc->bottom-lprc->top);
	cairo_fill(hDC);
	cairo_restore(hDC);
	return 1;
}
//---------------------------------------------------------------------------
BOOL DrawFocusRect(HDC hDC,CONST RECT * lprc)
{
	double dashes[4]={1,2,2,2};

	cairo_save(hDC);
	cairo_set_source_rgba(hDC,0,0,0,1);
	cairo_set_dash(hDC,dashes,4,0);
	cairo_rectangle(hDC,lprc->left,lprc->top+1,lprc->right - lprc->left,lprc->bottom - lprc->top - 2);
	cairo_stroke(hDC);
	cairo_restore(hDC);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL DrawEdge(HDC hdc,LPRECT qrc,UINT edge,UINT grfFlags)
{
	return TRUE;
}
//---------------------------------------------------------------------------
int DrawText(HDC hDC,LPCSTR lpString,int nCount,LPRECT lpRect,UINT uFormat)
{
	char *string;
	cairo_font_extents_t sz;
	LPDWORD data;
	COLORREF color;
	RECT rc;
	cairo_text_extents_t ex;
	cairo_operator_t op;
	XID xid;
	GdkWindow *window;

	if(hDC == NULL)
		return 0;
	if(nCount == -1)
		nCount = lstrlen(lpString);
	string = (char *)LocalAlloc(LPTR,nCount+2);
	if(string == NULL)
		return 0;
	data = (LPDWORD)cairo_get_user_data(hDC,&cr_data);
	lstrcpyn(string,lpString,nCount+1);
	cairo_font_extents(hDC,&sz);
	cairo_save(hDC);
	if(data != NULL && data[0] != TRANSPARENT){
		cairo_set_source_rgb(hDC,((data[1] >> 16) & 0xFF) / 255.0,((data[1] >> 8) & 0xff) / 255.0,(data[1]&0xFF)/255.0);
		cairo_rectangle(hDC,lpRect->left,lpRect->top+2,lpRect->right,lpRect->bottom);
		cairo_fill(hDC);
	}
	else{
		cairo_text_extents(hDC,string,&ex);
		xid = cairo_xlib_surface_get_drawable(cairo_get_target(hDC));
		if(xid != 0){
			window = (GdkWindow *)gdk_window_lookup(xid);
			if(window != 0 && GDK_IS_WINDOW(window))
				gdk_window_clear_area(window,lpRect->left,lpRect->top+3,ex.width+2,ex.height);
		}
	}
	color = data[2];
	cairo_set_source_rgba(hDC,(color & 0xFF) / 255.0,((color >> 8) & 0xFF) / 255.0,((color >> 16) & 0xFF) / 255.0,1);
	cairo_move_to(hDC,lpRect->left,lpRect->top+sz.ascent);
	cairo_show_text(hDC,string);
	cairo_restore(hDC);
	LocalFree(string);
	return 0;
}
//---------------------------------------------------------------------------
BOOL ExtTextOut(HDC hDC,int x,int y,UINT flags,LPRECT lpRect,LPCSTR lpString,UINT nCount,INT *lpTabs)
{
	RECT rc;

	rc.left = x;
	rc.top = y;
	rc.right = lpRect->right;
	rc.bottom = lpRect->bottom;
	if(!DrawText(hDC,lpString,nCount,&rc,0))
		return TRUE;
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL TextOut(HDC hDC,int x,int y,LPCSTR lpString,int nCount)
{
	RECT rc;

	rc.left = x;
	rc.top = y;
	rc.right = 3000;
	rc.bottom = 3000;
	if(!DrawText(hDC,lpString,nCount,&rc,0))
		return TRUE;
	return FALSE;
}
//---------------------------------------------------------------------------
BOOL GetTextExtentPoint32(HDC hDC,LPCSTR lpszText,int cchTextMax,LPSIZE psz)
{
	cairo_text_extents_t extents;
	gchar *string;

	if(hDC == NULL || psz == NULL || lpszText == NULL)
		return FALSE;
	string = (gchar *)LocalAlloc(LPTR,cchTextMax+1);
	if(string == NULL)
		return FALSE;
	strncpy(string,lpszText,cchTextMax);
	cairo_text_extents(hDC,string,&extents);
	LocalFree(string);
	psz->cx = extents.width+1;
	psz->cy = extents.height+1;
	return TRUE;
}
//---------------------------------------------------------------------------
static void DC_destroy_func(void *data)
{
	if(data != NULL)
		LocalFree(data);
}
//---------------------------------------------------------------------------
HDC GetDC(HWND hwnd)
{
	HDC hdc;
	void *data;
	GdkDrawable *drawable;

	if(hwnd == NULL)
		return NULL;
	if(!GTK_IS_WIDGET(hwnd))
		drawable = (GdkDrawable *)hwnd;
	else
		drawable = hwnd->window;
	if(drawable == NULL)
		return NULL;
	hdc = gdk_cairo_create(drawable);
	if(hdc == NULL)
		return NULL;
	data = LocalAlloc(LPTR,10*sizeof(DWORD));
	if(data == NULL){
		cairo_destroy(hdc);
		return NULL;
	}
	cairo_set_user_data(hdc,&cr_data,data,DC_destroy_func);
	return hdc;
}
//---------------------------------------------------------------------------
BOOL DeleteDC(HDC hDC)
{
	if(!ReleaseDC(NULL,hDC))
		return FALSE;
	return TRUE;
}
//---------------------------------------------------------------------------
int ReleaseDC(HWND hWnd,HDC hDC)
{
	if(hDC == NULL)
		return 0;
	cairo_destroy(hDC);
	return 1;
}
//---------------------------------------------------------------------------
HDC CreateCompatibleDC(HDC hDC)
{
	HDC hdc;
	GdkDrawable *drawable;
	void *data;

	if(hDC == NULL)
	{
		drawable = gdk_screen_get_root_window(gdk_screen_get_default());
		if(drawable == NULL)
			return NULL;
		hdc = gdk_cairo_create(drawable);
	}
	else
	{

	}
	data = LocalAlloc(LPTR,10*sizeof(DWORD));
	if(data == NULL){
		cairo_destroy(hdc);
		return NULL;
	}
	cairo_set_user_data(hdc,&cr_data,data,DC_destroy_func);
	return hdc;
}
//---------------------------------------------------------------------------
static void OnDestroyDIBSection(gpointer data)
{
	if(data != NULL)
		LocalFree(data);
}
//---------------------------------------------------------------------------
HBITMAP CreateDIBSection(HDC hdc,BITMAPINFO *pbmi,UINT iUsage,VOID **ppvBits,HANDLE hSection,DWORD dwOffset)
{
	LPDWORD data;
	HBITMAP bit;
	void *buffer;

	if(pbmi == NULL || hdc == NULL)
		return NULL;
	bit = CreateBitmap(pbmi->bmiHeader.biWidth,pbmi->bmiHeader.biHeight,
		pbmi->bmiHeader.biPlanes,pbmi->bmiHeader.biBitCount,NULL);
	if(bit == NULL)
		return NULL;
	buffer = LocalAlloc(LPTR,pbmi->bmiHeader.biWidth*abs(pbmi->bmiHeader.biHeight)*pbmi->bmiHeader.biBitCount >> 3);
	if(buffer == NULL){
		DeleteObject(bit);
		return NULL;
	}
	g_object_set_data_full((GObject *)bit,"DIBSectionData",buffer,OnDestroyDIBSection);
	data = (LPDWORD)cairo_get_user_data(hdc,&cr_data);
	if(data != NULL)
		data[3] = (DWORD)bit;
	if(ppvBits != NULL)
		*ppvBits = buffer;
	return bit;
}
//---------------------------------------------------------------------------
BOOL BitBlt(HDC hdcDest,int nXDest,int nYDest,int nWidth,int nHeight,HDC hdcSrc,int nXSrc,int nYSrc,DWORD dwRop)
{
    HBITMAP bitDest,bitSrc;
	LPDWORD data;
	cairo_surface_t *img;

	if(hdcDest == NULL || hdcSrc == NULL)
		return FALSE;
	data = (LPDWORD)cairo_get_user_data(hdcDest,&cr_data);
	if(data == NULL)
		return FALSE;
	bitDest = (HBITMAP)data[3];
	if(bitDest = NULL)
		return FALSE;
	data = (LPDWORD)cairo_get_user_data(hdcSrc,&cr_data);
	if(data == NULL)
		return FALSE;
	bitSrc = (HBITMAP)data[3];
	if(bitSrc == NULL)
		return FALSE;
//	gdk_pixbuf_copy_area(bitSrc,nXSrc,nYSrc,nWidth,nHeight,bitDest,nXDest,nYDest);
	img = cairo_image_surface_create_for_data(gdk_pixbuf_get_pixels(bitSrc),gdk_pixbuf_get_has_alpha(bitSrc) ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
		gdk_pixbuf_get_width(bitSrc),gdk_pixbuf_get_height(bitSrc),gdk_pixbuf_get_rowstride(bitSrc));

	cairo_set_source_surface(hdcDest,img,0,0);
	cairo_rectangle(hdcDest,nXDest,nYDest,nWidth,nHeight);
	cairo_clip(hdcDest);
	cairo_paint(hdcDest);
	cairo_surface_destroy(img);
	return TRUE;
}
//---------------------------------------------------------------------------
int StretchDIBits(HDC hdc,int XDest,int YDest,int nDestWidth,int nDestHeight,int XSrc,int YSrc,int nSrcWidth,int nSrcHeight,const VOID *lpBits,const BITMAPINFO *lpBitsInfo,UINT iUsage,DWORD dwRop)
{
	HBITMAP bit;
	LPDWORD data;
	guchar *buffer,*dst_buffer,*p;
	int width,height,x,y,r,g,b,alpha,bytes_line;

	if(hdc == NULL || lpBits == NULL ||	lpBitsInfo == NULL)
		return 0;
	data = (LPDWORD)cairo_get_user_data(hdc,&cr_data);
	if(data == NULL)
		return 0;
	bit = (HBITMAP)data[3];
	if(bit == NULL)
		return 0;
	buffer = (guchar *)lpBits;//g_object_get_data((GObject *)bit,"DIBSectionData");
	if(buffer == NULL)
		return 0;
	width = gdk_pixbuf_get_width(bit);//
	height = gdk_pixbuf_get_height(bit);//
	dst_buffer = gdk_pixbuf_get_pixels(bit);//
	alpha = (int)gdk_pixbuf_get_has_alpha(bit);
	bytes_line = gdk_pixbuf_get_rowstride(bit);
	switch(lpBitsInfo->bmiHeader.biBitCount){
		case 16:
			for(y = 0;y<height;y++){
				p = dst_buffer;
				for(x=0;x<width;x++){
					r = (int)((unsigned short *)buffer)[y*width+x];
					b = r & 31;
					g = (r >> 5) & 31;
					r >>= 10;
					*p++ = r << 3;
					*p++ = g << 3;
					*p++ = b << 3;
					if(alpha)
						*p++ = 255;
				}
				dst_buffer += bytes_line;
			}
		break;
		default:
			y = x = 0;
		break;
	}
	return y;
}
//---------------------------------------------------------------------------
HBITMAP CreateBitmap(int width,int height,UINT nPlanes,UINT nBits,CONST VOID *lpBits)
{
	BOOL bAlpha;
	HBITMAP bit;

	bAlpha = nBits == 32 ? TRUE : FALSE;
	bit = gdk_pixbuf_new(GDK_COLORSPACE_RGB,bAlpha,8,width,abs(height));
	if(bit == NULL)
		return NULL;
	if(lpBits == NULL)
		return bit;
	return bit;
}
//---------------------------------------------------------------------------
HFONT GetWindowFont(HWND hWnd)
{
	GtkStyle *style;
	PangoContext *context;
	PangoFontDescription *font_desc;
	GdiObject *obj;
	int i;

	if(hWnd == NULL)
		return NULL;
	font_desc = NULL;
	if((style = gtk_widget_get_style(hWnd)) != NULL)
		font_desc = style->font_desc;
	if(font_desc == NULL){
		context = gtk_widget_get_pango_context(hWnd);
		if(context == NULL)
			return NULL;
		font_desc = pango_context_get_font_description(context);
	}
	obj = gdiobject_new_font();
	pango_font_description_set_family((PangoFontDescription *)obj->object,pango_font_description_get_family(font_desc));
	i = pango_font_description_get_size(font_desc);
	pango_font_description_set_size((PangoFontDescription *)obj->object,pango_font_description_get_size(font_desc));
	return obj;
}
//---------------------------------------------------------------------------
UINT SetTimer(HWND hWnd,UINT nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc)
{
	LTimer *timer;

	timer = FindTimer(hWnd,nIDEvent);
	if(timer != NULL)
		return timer->Event();
	timer = new LTimer();
	if(timer == NULL)
		return 0;
	return timer->Start(hWnd,nIDEvent,uElapse,lpTimerFunc);
}
//---------------------------------------------------------------------------
BOOL KillTimer(HWND hWnd,UINT uIDEvent)
{
	LTimer *timer;

	timer = FindTimer(hWnd,uIDEvent);
	if(timer == NULL)
		return FALSE;
	timer->Stop();
	delete timer;
	return TRUE;
}
//---------------------------------------------------------------------------
UINT DragQueryFile(HDROP hDrop,UINT iFile,LPTSTR lpszFile,UINT cch)
{
	int i,len,count;
	char *p,*p1;

	if(hDrop == NULL || hDrop->format != 8)
		return 0;
	if(lpszFile == NULL)
		return hDrop->length;
	for(p = (char *)hDrop->data,count = i=0;i < hDrop->length-1;i++){
		for(len=0;i<hDrop->length-1;i++){
			if(*p++ == '\n')
				break;
		}
		count++;
	}
	if(iFile == 0xFFFFFFFF)
		return (UINT)count;
	if(iFile >= count)
		return 0;
	for(p = (char *)hDrop->data,count = i =0;i<hDrop->length-1;i++){
		if(count == iFile){
			for(p1 = p,len = 0;i<hDrop->length-1;i++){
				len++;
				if(*p++ == '\n')
					break;
			}
			if((p = strstr(p1,"file://")) != NULL){
				p1 = p+7;
				len -= 7;
			}
			if(cch > len)
				cch = len;
			lstrcpyn(lpszFile,p1,cch+1);
			return cch;
		}
		for(len=0;i<hDrop->length-1;i++){
			if(*p++ == '\n')
				break;
		}
		count++;
	}
	return 0;
}
//---------------------------------------------------------------------------
VOID DragFinish(HDROP hDrop)
{
}
//---------------------------------------------------------------------------
UINT ShellExecute(HWND hwnd,LPCSTR lpOperation,LPCSTR lpFile,LPCSTR lpParameters,LPCSTR lpDirectory,INT nShowCmd)
{
	if(lpFile == NULL || lpOperation == NULL)
		return 0;
	if(lstrcmpi(lpOperation,"open") == 0){
		g_app_info_launch_default_for_uri(lpFile,NULL,NULL);
	}
	return 0;
}
//---------------------------------------------------------------------------
void GetSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
    DWORD dw,ebx,dw1;

    if(lpSystemInfo == NULL)
        return;
    __asm__ __volatile__(
        "pushl %%ebx\n"
        "pushl %%ecx\n"
        "pushl %%edx\n"
        "movl $1,%%eax\n"
        "cpuid\n"
        "movl %%ebx,%0\n"
        "popl %%edx\n"
        "popl %%ecx\n"
        "popl %%ebx\n"
        : "=m" (dw) : "m" (ebx)
    );
    dw1 = (dw & 0xFF0000) >> 16;
    if(dw1 == 0)
        dw1 = 1;
    lpSystemInfo->dwNumberOfProcessors = dw1;
}
//---------------------------------------------------------------------------
BOOL change_signal(HWND hWnd,int index,const gchar *detailed_signal,GCallback c_handler,gpointer gobject,BOOL bAfter)
{
	gulong signal;

	if(hWnd == NULL)
		return FALSE;
	if(bAfter)
		signal = g_signal_connect_after((gpointer)hWnd,detailed_signal,c_handler,gobject);
	else
		signal = g_signal_connect((gpointer)hWnd,detailed_signal,c_handler,gobject);
	SetWindowLong(hWnd,GWL_SIGNAL+index,(LONG)signal);
	return TRUE;
}
//---------------------------------------------------------------------------
int KeyCodeToVK(GdkEventKey *event)
{
    switch(event->keyval){
		case GDK_F1:
			return VK_F1;
		case GDK_F2:
			return VK_F2;
		case GDK_F3:
			return VK_F3;
		case GDK_F4:
			return VK_F4;
		case GDK_F5:
			return VK_F5;
		case GDK_F6:
			return VK_F6;
		case GDK_F7:
			return VK_F7;
		case GDK_F8:
			return VK_F8;
		case GDK_F9:
			return VK_F9;
		case GDK_F10:
			return VK_F10;
		case GDK_Left:
			return VK_LEFT;
		case GDK_Right:
			return VK_RIGHT;
		case GDK_Up:
			return VK_UP;
		case GDK_Down:
			return VK_DOWN;
		case GDK_Tab:
			return VK_TAB;
		case GDK_Return:
			return VK_RETURN;
		case GDK_Home:
			return VK_HOME;
		case GDK_Shift_L:
		case GDK_Shift_R:
			return VK_SHIFT;
		case GDK_Delete:
			return VK_DELETE;
        default:
            return gdk_keyval_to_upper(event->keyval);
    }
}
//---------------------------------------------------------------------------
int VKToKeyCodeTo(int value)
{
    switch(value){
		case VK_F1:
			return GDK_F1;
		case VK_F2:
			return GDK_F2;
		case VK_F3:
			return GDK_F3;
		case VK_F4:
			return GDK_F4;
		case VK_F5:
			return GDK_F5;
		case VK_F6:
			return GDK_F6;
		case VK_F7:
			return GDK_F7;
		case VK_F8:
			return GDK_F8;
		case VK_F9:
			return GDK_F9;
		case VK_F10:
			return GDK_F10;
		case VK_DELETE:
			return GDK_Delete;
		case VK_SHIFT:
			return GDK_Shift_L;
		case VK_HOME:
			return GDK_Home;
		case VK_LEFT:
			return GDK_Left;
		case VK_RIGHT:
			return GDK_Right;
		case VK_UP:
			return GDK_Up;
		case VK_DOWN:
			return GDK_Down;
		case VK_TAB:
			return GDK_Tab;
		case VK_RETURN:
			return GDK_Return;
        default:
            return value;
    }
}
#endif



