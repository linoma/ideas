#include "lopensavedlg.h"
#include "lapp.h"

extern HINSTANCE hInst;
//--------------------------------------------------------------------------------
LOpenSaveDlg::LOpenSaveDlg()
{
}
//--------------------------------------------------------------------------------
LOpenSaveDlg::~LOpenSaveDlg()
{
}
//--------------------------------------------------------------------------------
BOOL LOpenSaveDlg::ShowSave(char *caption,char *lpstrFilter,HWND hWndParent,char *fileName,LPFNCSAVEDLG pFnc)
{
#ifndef __WIN32__
	HWND w;
	BOOL res;
	char *filename,*p,*p1;
	GtkFileFilter *filter;
	LString s;

	w = gtk_file_chooser_dialog_new(caption,(GtkWindow *)hWndParent,GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_current_folder((GtkFileChooser *)w,fileName);
	*((long *)fileName) = 0;
	if(lpstrFilter != NULL && lpstrFilter[0] != 0){		
		p = lpstrFilter; 
		while(*p != 0){
			filter = gtk_file_filter_new();
			gtk_file_filter_set_name(filter,p);
			p += lstrlen(p) + 1;
			p1 = p;
			while(*p1 != 0){
				s = "";
				while(*p1 != 0){
					if(*p1 == ';'){
						gtk_file_filter_add_pattern (filter, s.c_str());					
						s = "";
						p1++;
						break;
					}				
					s += *p1++;
				}
				if(!s.IsEmpty())
					gtk_file_filter_add_pattern (filter, s.c_str());					
			}
			p += lstrlen(p) + 1;
			gtk_file_chooser_add_filter((GtkFileChooser *)w,filter);
		}
				
	}
	gtk_widget_show (w);	
	res = FALSE;
	if(gtk_dialog_run((GtkDialog *)w) == GTK_RESPONSE_ACCEPT){
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (w));
		lstrcpy(fileName,filename);
		g_free (filename);				
		res = TRUE;
	}	
	gtk_widget_destroy(w);
	return res;	
#else
   OPENFILENAME ofn={0};

   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hWndParent;
   ofn.lpstrFile = fileName;
   ofn.nMaxFile = MAX_PATH;
   if(lpstrFilter != NULL){
       ofn.lpstrFilter = lpstrFilter;
       ofn.nFilterIndex = 1;
   }
   ofn.Flags = OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
   ofn.lpstrTitle = caption;
   if(pFnc != NULL){
       if(!pFnc(&ofn))
           return FALSE;
   }
   return ::GetSaveFileName(&ofn);
#endif
}
//--------------------------------------------------------------------------------
BOOL LOpenSaveDlg::ShowOpen(char *caption,char *lpstrFilter,HWND hWndParent,char *fileName,int nMaxFile,DWORD dwFlags)
{
#ifndef __WIN32__
	HWND w;
	BOOL res;
	char *filename,*p,*p1;
	GtkFileFilter *filter;
	LString s;

	w = gtk_file_chooser_dialog_new(caption,(GtkWindow *)hWndParent,GTK_FILE_CHOOSER_ACTION_OPEN,GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_current_folder((GtkFileChooser *)w,fileName);
	*((long *)fileName) = 0;
	if(lpstrFilter != NULL && lpstrFilter[0] != 0){		
		p = lpstrFilter; 
		while(*p != 0){
			filter = gtk_file_filter_new();
			gtk_file_filter_set_name(filter,p);
			p += lstrlen(p) + 1;
			p1 = p;
			while(*p1 != 0){
				s = "";
				while(*p1 != 0){
					if(*p1 == ';'){
						gtk_file_filter_add_pattern (filter, s.c_str());					
						s = "";
						p1++;
						break;
					}				
					s += *p1++;
				}
				if(!s.IsEmpty())
					gtk_file_filter_add_pattern (filter, s.c_str());					
			}
			p += lstrlen(p) + 1;
			gtk_file_chooser_add_filter((GtkFileChooser *)w,filter);
		}
				
	}
	gtk_widget_show (w);	
	res = FALSE;
	if(gtk_dialog_run((GtkDialog *)w) == GTK_RESPONSE_ACCEPT){
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (w));
		lstrcpy(fileName,filename);
		g_free (filename);				
		res = TRUE;
	}	
	gtk_widget_destroy(w);
	return res;
#else
   OPENFILENAME ofn={0};
	char c[500],*p,*p1,*p2;
   int i;

   lstrcpy(c,fileName);
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hWndParent;
   ofn.lpstrFile = fileName;
   ofn.nMaxFile = nMaxFile;
   ofn.lpstrInitialDir = c;
   *((long *)fileName) = 0;
   if(lpstrFilter != NULL){
       ofn.lpstrFilter = lpstrFilter;
       ofn.nFilterIndex = 1;
   }
   ofn.Flags = OFN_FILEMUSTEXIST|OFN_NOREADONLYRETURN|OFN_HIDEREADONLY|OFN_EXPLORER|dwFlags;
   if(caption != NULL)
   	ofn.lpstrTitle = caption;
   if(!GetOpenFileName(&ofn))
       return FALSE;
   if((i = lstrlen(fileName)) < ofn.nFileOffset){
       if((p = (char *)LocalAlloc(LPTR,nMaxFile)) == NULL)
           return FALSE;
       lstrcpy(p,fileName);
       if(p[i] != '\\')
           lstrcat(p,"\\");
       lstrcat(p,(p2 = &fileName[i+1]));
       p1 = p + lstrlen(p);
       *p1++ = 0;
       lstrcpy(p1,fileName);
       while(*p2++ != 0);
       if(p1[i] != '\\')
           lstrcat(p1,"\\");
       lstrcat(p1,p2);
       CopyMemory(fileName,p,nMaxFile);
       LocalFree(p);
   }
   return TRUE;
#endif
}
