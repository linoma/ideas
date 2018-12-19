#include "polyviewer.h"
#include "resource.h"
#include "lds.h"
#include "3d.h"
#include "util.h"

#ifdef _DEBPRO
//---------------------------------------------------------------------------
LPolygonViewer::LPolygonViewer() : LDlg()
{
	hRC = NULL;
   hdcRender = NULL;
   hbRender = NULL;
   hdcZoom = NULL;
   hbZoom = NULL;
   hdcWindow = NULL;
   iScale = xScroll = yScroll = 0;
   dwOptions = NTR_DONT_USE_LISTS|NTR_TEX_COMBINE_FLAG|NTR_DEPTH_TEST_OPTION|NTR_LIGHT0_OPTION|NTR_LIGHT1_OPTION|NTR_LIGHT2_OPTION|NTR_LIGHT3_OPTION|NTR_TEXTURE_OPTION;
	for(int i = 0;i<3;i++){
       fScale[i] = 1.0f;
       fTrans[i] = 0.0f;
       nTrans[i] = 0;
       nScale[i] = 120;
       fRotate[i] = 0;
       nRotate[i] = 0;
   }
   nPos[0] = 128;
   nPos[1] = 96;
   nPos[2] = 50;
   imageLists[0] = imageLists[1] = NULL;
   nMouseMode = 1;
}
//---------------------------------------------------------------------------
LPolygonViewer::~LPolygonViewer()
{
	Destroy();
}
//---------------------------------------------------------------------------
BOOL LPolygonViewer::Destroy()
{
   PlugInList *list;
   PlugIn *p;
	u32 values[10];

   if(pPlugInContainer != NULL && (list = pPlugInContainer->get_PlugInList(PIL_VIDEO)) != NULL && (p = list->get_ActivePlugIn(PIT_IS3D)) != NULL){
       values[0] = -1;
       values[1] = (u32)-12;
       values[2] = (u32)0;
       p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)values);
       values[0] = -1;
       values[1] = (u32)-13;
       values[2] = (u32)0;
       p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)values);
   }
   if(hRC != NULL){
       //       wglMakeCurrent(NULL,NULL);
       wglDeleteContext(hRC);
       hRC = NULL;
   }
   if(hdcRender != NULL){
       DeleteDC(hdcRender);
       hdcRender = NULL;
   }
   if(hbRender != NULL){
       ::DeleteObject(hbRender);
       hbRender = NULL;
   }
   if(hdcZoom != NULL){
       DeleteDC(hdcZoom);
       hdcZoom = NULL;
   }
   if(hbZoom != NULL){
       DeleteObject(hbZoom);
       hbZoom = NULL;
   }
   if(hdcWindow != NULL)
       ReleaseDC(m_hWnd,hdcWindow);
	for(int i = 0;i<2;i++){
		if(imageLists[i] != 0)
   		ImageList_Destroy(imageLists[i]);
       imageLists[i] = NULL;
   }
   return LDlg::Destroy();
}
//---------------------------------------------------------------------------
BOOL LPolygonViewer::Show(HWND parent)
{
   if(m_hWnd == NULL){
       if(!LDlg::Create(hInst,MAKEINTRESOURCE(IDD_DIALOG21),parent))
           return FALSE;
   }
   else{
       ::BringWindowToTop(m_hWnd);
       ::InvalidateRect(m_hWnd,NULL,TRUE);
       ::UpdateWindow(m_hWnd);
   }
   return TRUE;
}
//---------------------------------------------------------------------------
void LPolygonViewer::DrawPolygon()
{
	int i;
   SCROLLINFO si;

	if(hRC == NULL || hdcRender == NULL || hbRender == NULL)
   	return;
	SelectObject(hdcRender,hbRender);
	::StretchDIBits(hdcRender,0,0,256,192,0,0,256,192,pBuffer,&bmi,DIB_RGB_COLORS,SRCCOPY);
   if(hbZoom != NULL && hdcZoom != NULL){
   	SelectObject(hdcZoom,hbZoom);
       if((i = SendDlgItemMessage(m_hWnd,IDC_TRACK1,TBM_GETPOS,0,0)) > 1){
   		memset(&si,0,sizeof(SCROLLINFO));
   		si.cbSize = sizeof(SCROLLINFO);
   		si.fMask = SIF_ALL;
   		si.nMax = (192*i)-1;
   		si.nPage = 192;
       	if(yScroll < si.nMax)
   			si.nPos = yScroll;
       	else
       		si.nPos = yScroll = 0;
   		ShowScrollBar(GetDlgItem(m_hWnd,IDC_VSBTEX),SB_CTL,TRUE);
   		SetScrollInfo(GetDlgItem(m_hWnd,IDC_VSBTEX),SB_CTL,&si,TRUE);

   		memset(&si,0,sizeof(SCROLLINFO));
   		si.cbSize = sizeof(SCROLLINFO);
   		si.fMask = SIF_ALL;
   		si.nMax = (256*i)-1;
   		si.nPage = 256;
       	if(xScroll  < si.nMax)
   			si.nPos = xScroll;
       	else
       		si.nPos = xScroll = 0;
   		ShowScrollBar(GetDlgItem(m_hWnd,IDC_HSBTEX),SB_CTL,TRUE);
   		SetScrollInfo(GetDlgItem(m_hWnd,IDC_HSBTEX),SB_CTL,&si,TRUE);
       }
       else{
			ShowScrollBar(GetDlgItem(m_hWnd,IDC_HSBTEX),SB_CTL,FALSE);
           ShowScrollBar(GetDlgItem(m_hWnd,IDC_VSBTEX),SB_CTL,FALSE);
           xScroll = yScroll = 0;
       }
		StretchBlt(hdcZoom,0,0,256*i,192*i,hdcRender,0,0,256,192,SRCCOPY);
       BitBlt(hdcWindow,0,0,256,192,hdcZoom,xScroll,yScroll,SRCCOPY);
   }
   else
   	BitBlt(hdcWindow,0,0,256,192,hdcRender,xScroll,yScroll,SRCCOPY);
}
//---------------------------------------------------------------------------
void LPolygonViewer::UpdatePolygon()
{
	PlugIn *p;
	u32 i[9];
   float f[4];
   char s[60];
	const char de[2][8]={{"Disable"},{"Enable"}};
   const char hr[2][8]={{"Hide"},{"Render"}};

	if(hRC == NULL || hdcRender == NULL || hbRender == NULL)
   	return;
   if((p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D)) == NULL)
   	return;
	glEnd();
   glEndList();
	wglMakeCurrent(hdcWindow,hRC);
   glViewport(0,0,256,192);
	if(dwOptions & NTR_TEXTURE_OPTION)
   	glEnable(GL_TEXTURE_2D);
   else
   	glDisable(GL_TEXTURE_2D);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_DITHER);
	glDisable(GL_LIGHTING);
   glDisable(GL_BLEND);
   glDepthFunc(GL_ALWAYS);
   glClearColor(0,0,0,0);
  	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   glDepthRange(0,1);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
   f[0] = f[1] = f[2] = 0.2;
   f[3] = 1.0;
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,f);
	f[0] = f[1] = f[2] = 0.8;
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,f);
	f[0] = f[1] = f[2] = 0;
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,f);
	f[0] = f[1] = f[2] = 0;
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,f);
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glScalef(fScale[0],fScale[1],fScale[2]);
   glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
   glTranslatef(fTrans[0],fTrans[1],fTrans[2]);
   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();

   i[0] = (u32)(wIndex - 1);
   i[1] = (u32)dwOptions;
   i[2] = (u32)(dwOptions >> 32);
   i[3] = (u32)hdcWindow;
   i[4] = (u32)hRC;
   i[5] = (u32)pBuffer;
   i[6] = 256;
   i[7] = 192;
   i[8] = 0;
   if(wIndex != 0)
   	p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)i);
	xScroll = yScroll = 0;
   ShowScrollBar(GetDlgItem(m_hWnd,IDC_VSBTEX),SB_CTL,FALSE);
   ShowScrollBar(GetDlgItem(m_hWnd,IDC_HSBTEX),SB_CTL,FALSE);
   if(i[0] == 0 || wIndex == 0){
   	SetDlgItemText(m_hWnd,IDC_POLYGON_LIGHT0,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_LIGHT1,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_LIGHT2,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_LIGHT3,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_MODE,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_BACK,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_FRONT,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_DEPTHVALUE,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_FAR,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_1DOT,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_DEPTHTEST,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_FOG,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_ALPHA,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_ID,"");
   	SetDlgItemText(m_hWnd,IDC_POLYGON_DATA,"");
       ShowWindow(GetDlgItem(m_hWnd,IDC_TRACK1),SW_HIDE);
       memset(pBuffer,0,256*192*sizeof(u32));
		TreeView_DeleteAllItems(GetDlgItem(m_hWnd,IDC_TREE1));
       DrawPolygon();
       return;
   }
   lstrcpy(s,hr[(i[0] & 1)]);
  	SetDlgItemText(m_hWnd,IDC_POLYGON_LIGHT0,s);
   lstrcpy(s,hr[(i[0] & 2) >> 1]);
  	SetDlgItemText(m_hWnd,IDC_POLYGON_LIGHT1,s);
   lstrcpy(s,hr[(i[0] & 4) >> 2]);
  	SetDlgItemText(m_hWnd,IDC_POLYGON_LIGHT2,s);
   lstrcpy(s,hr[(i[0] & 8) >> 3]);
  	SetDlgItemText(m_hWnd,IDC_POLYGON_LIGHT3,s);

  	switch(i[0] & 0x30){
      	case 0:
          	lstrcpy(s,"Modulation");
       break;
       case 0x10:
       	lstrcpy(s,"Decal");
       break;
       case 0x20:
       	lstrcpy(s,"Toon/Highlight");
       break;
       case 0x30:
          	lstrcpy(s,"Shadow");
       break;
   }
   SetDlgItemText(m_hWnd,IDC_POLYGON_MODE,s);
	lstrcpy(s,hr[((i[0] >> 15) & 1)]);
   SetDlgItemText(m_hWnd,IDC_POLYGON_BACK,s);
	lstrcpy(s,hr[((i[0] >> 15) & 1)]);
   SetDlgItemText(m_hWnd,IDC_POLYGON_FRONT,s);
   switch(i[0] & (1<<11)){
      	case 0:
          	lstrcpy(s,"Keep old");
       break;
       default:
          	lstrcpy(s,"Set New Depth");
       break;
   }
   SetDlgItemText(m_hWnd,IDC_POLYGON_DEPTHVALUE,s);
	lstrcpy(s,hr[((i[0] >> 12) & 1)]);
   SetDlgItemText(m_hWnd,IDC_POLYGON_FAR,s);
   lstrcpy(s,hr[((i[0] >> 13) & 1)]);
   SetDlgItemText(m_hWnd,IDC_POLYGON_1DOT,s);
   switch(i[0] & 0x4000){
      	case 0:
         	lstrcpy(s,"Less");
       break;
       default:
          	lstrcpy(s,"Equal");
       break;
   }
   SetDlgItemText(m_hWnd,IDC_POLYGON_DEPTHTEST,s);
   lstrcpy(s,de[((i[0] >> 15) & 1)]);
   SetDlgItemText(m_hWnd,IDC_POLYGON_FOG,s);
	wsprintf(s,"0x%02X",((i[0] >> 16) & 0x1F));
   SetDlgItemText(m_hWnd,IDC_POLYGON_ALPHA,s);
	wsprintf(s,"0x%02X",((i[0] >> 24) & 0x3F));
   SetDlgItemText(m_hWnd,IDC_POLYGON_ID,s);
   wsprintf(s,"0x%08X",i[0]);
   SetDlgItemText(m_hWnd,IDC_POLYGON_DATA,s);

   i[8] = (u32)GetDlgItem(m_hWnd,IDC_TREE1);
	i[0] = (u32)(wIndex - 1);
//	LockWindowUpdate((HWND)i[8]);
	p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)i);
// 	LockWindowUpdate(NULL);
   if(hdcZoom == NULL)
   	hdcZoom = CreateCompatibleDC(NULL);
   if(hbZoom == NULL && hdcZoom != NULL){
   	for(iScale = 10;iScale > 1;iScale--){
			hbZoom = CreateCompatibleBitmap(hdcWindow,256 * iScale,192 * iScale);
           if(hbZoom != NULL)
           	break;
   	}
   }
	if(hbZoom == NULL || iScale < 2){
       ShowWindow(GetDlgItem(m_hWnd,IDC_TRACK1),SW_HIDE);
       iScale = 0;
       return;
   }
   SelectObject(hdcZoom,hbZoom);
   SendDlgItemMessage(m_hWnd,IDC_TRACK1,TBM_SETRANGE,FALSE,MAKELPARAM(1,iScale));
   SendDlgItemMessage(m_hWnd,IDC_TRACK1,TBM_SETPOS,TRUE,0);
   ShowWindow(GetDlgItem(m_hWnd,IDC_TRACK1),SW_SHOW);
	DrawPolygon();
}
//---------------------------------------------------------------------------
void LPolygonViewer::UpdateScaleTransMultiplier()
{
	int value;
	char s[50];

   value = SendDlgItemMessage(m_hWnd,IDC_TRACK2,TBM_GETPOS,0,0);
	SendDlgItemMessage(m_hWnd,IDC_UPDOWN2,UDM_SETPOS,0,MAKELONG(value,0));
   wsprintf(s,"%d",value - 100);
   SetDlgItemText(m_hWnd,IDC_EDIT2,s);
   DrawPolygon();
}
//---------------------------------------------------------------------------
void LPolygonViewer::OnPolygonViewerMouseMove(WORD wKeys,int x,int y)
{
   int index,idelta,*values;
   float scale,delta,*fvalues;
	BOOL update;

	switch(nMouseMode){
       case 2:
          	values = nScale;
           fvalues = fScale;
       break;
       case 3:
       	values = nRotate;
           fvalues = fRotate;
       break;
       case 4:
       	values = nTrans;
           fvalues = fTrans;
       break;
       default:
       	values = NULL;
       break;
   }
   if(values == NULL)
   	return;

	index = SendDlgItemMessage(m_hWnd,IDC_UPDOWN2,UDM_GETPOS,0,0);
   if((index >> 16))
   	return;

   update = FALSE;
   scale = (float)(short)index;
   if(scale == 0)
       scale = 1;
   else if(scale < 0)
       scale = 1.0f / scale * -1;

   idelta = (x - values[0]);
   if(abs(idelta) > 4){
       delta = idelta * scale / 4;
       fvalues[0] += delta;
       update = TRUE;
   }
   idelta = values[1] - y;
   if(abs(idelta) > 4){
       delta = idelta * scale / 4;
       fvalues[1] += delta;
       update = TRUE;
   }
   if(update){
       values[0] = x;
       values[1] = y;
       UpdateTransformation();
   }
}
//---------------------------------------------------------------------------
void LPolygonViewer::OnPolygonViewerMouseWheel(WORD wKeys,short wWheel,int x,int y)
{
   float scale,*fvalues;
   int index,*values;

	switch(nMouseMode){
       case 2:
          	values = nScale;
           fvalues = fScale;
       break;
       case 3:
       	values = nRotate;
           fvalues = fRotate;
       break;
       case 4:
       	values = nTrans;
           fvalues = fTrans;
       break;
       default:
       	values = NULL;
       break;
   }
   if(values == NULL)
   	return;
	index = SendDlgItemMessage(m_hWnd,IDC_UPDOWN2,UDM_GETPOS,0,0);
   if((index >> 16))
   	return;
   scale = (float)(short)index;
   if(scale == 0)
       scale = 1;
   else if(scale < 0)
       scale = 1.0f / scale * -1;
   values[2] += wWheel;
   if(values[2] < 0)
       fvalues[2] = 1.0f / ((float)abs(values[2]) / (float)WHEEL_DELTA);
   else
       fvalues[2] = values[2] / (float)WHEEL_DELTA;
   fvalues[2] *= scale;
   UpdateTransformation();
}
//---------------------------------------------------------------------------
void LPolygonViewer::UpdateTransformation(int mode)
{
   char s[100];
   
   sprintf(s,"X: %f Y: %f Z: %f",fScale[0],fScale[1],fScale[2]);
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,SB_SETTEXT,1,(LPARAM)s);
   sprintf(s,"X: %f Y: %f Z: %f",fTrans[0],fTrans[1],fTrans[2]);
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,SB_SETTEXT,0,(LPARAM)s);
   sprintf(s,"X: %f Y: %f Z: %f",fRotate[0],fRotate[1],fRotate[2]);
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,SB_SETTEXT,2,(LPARAM)s);
   UpdatePolygon();
}
//---------------------------------------------------------------------------
LRESULT LPolygonViewer::OnWindowProcPolygonViewer(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LRESULT res;

   res = CallWindowProc(lpPrevPolyViewerProc,hWnd,uMsg,wParam,lParam);
	switch(uMsg){
        case WM_PAINT:
            DrawPolygon();
        break;
        case WM_LBUTTONDOWN:
        	SetFocus(hWnd);
			nPos[0] = (int)LOWORD(lParam);
			nPos[1] = (int)HIWORD(lParam);
           SetCapture(hWnd);
        break;
        case WM_LBUTTONUP:
        	ReleaseCapture();
        break;
        case WM_MOUSEMOVE:
            if(wParam & MK_LBUTTON)
				OnPolygonViewerMouseMove(LOWORD(wParam),(int)(signed short)LOWORD(lParam),(int)(short)HIWORD(lParam));
        break;
        case WM_MOUSEWHEEL:
        	OnPolygonViewerMouseWheel(LOWORD(wParam),(short)HIWORD(wParam),(int)(signed short)LOWORD(lParam),(int)(short)HIWORD(lParam));
        break;
	}
   return res;
}
//---------------------------------------------------------------------------
LRESULT LPolygonViewer::WindowProcPolygonViewer(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LPolygonViewer *owner;

   owner = (LPolygonViewer *)GetWindowLong(hWnd,GWL_USERDATA);
   if(owner == 0)
   	return 0;
   return owner->OnWindowProcPolygonViewer(hWnd,uMsg,wParam,lParam);
}
//---------------------------------------------------------------------------
void LPolygonViewer::OnInitDialog()
{
	TBBUTTON tbb[16];
	PIXELFORMATDESCRIPTOR pfd={0};
	int pixelformat,st_parts[]={250,500,750};
	RECT rc,rc1;
   HWND hwnd;
	POINT pt;
	u32 i;
   HBITMAP bit;

	::SetRect(&rc,0,0,256,192);
   hwnd = GetDlgItem(m_hWnd,IDC_POLYGON);
   ::AdjustWindowRectEx(&rc,GetWindowLong(hwnd,GWL_STYLE),FALSE,GetWindowLong(hwnd,GWL_EXSTYLE));
	rc.right += abs(rc.left);
   rc.bottom += abs(rc.top);
	::SetWindowPos(hwnd,0,0,0,rc.right,rc.bottom,SWP_NOMOVE|SWP_NOZORDER|SWP_NOSENDCHANGING);
   ::GetWindowRect(hwnd,&rc);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc,2);
   SetWindowLong(hwnd,GWL_USERDATA,(LONG)this);
   lpPrevPolyViewerProc = (WNDPROC)SetWindowLong(hwnd,GWL_WNDPROC,(LONG)WindowProcPolygonViewer);

	hwnd = GetDlgItem(m_hWnd,IDC_HSBTEX);
	::GetWindowRect(hwnd,&rc1);
   ::MapWindowPoints(NULL,m_hWnd,(LPPOINT)&rc1,2);
	::SetWindowPos(hwnd,0,rc.left,rc.bottom+1,rc.right-rc.left,rc1.bottom-rc1.top,SWP_NOZORDER|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
	hwnd = GetDlgItem(m_hWnd,IDC_TRACK1);
	pt.y = rc.bottom + 2 + (rc1.bottom - rc1.top);
	::GetWindowRect(hwnd,&rc1);
   rc1.left = rc.left + ((rc.right - rc.left - (rc1.right - rc1.left)) >> 1);
	::SetWindowPos(hwnd,0,rc1.left,pt.y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
   ::SendMessage(hwnd,TBM_SETRANGE,0,MAKELPARAM(0,10));
   ::SendMessage(hwnd,TBM_SETPOS,TRUE,0);
	hwnd = GetDlgItem(m_hWnd,IDC_VSBTEX);
	::GetWindowRect(hwnd,&rc1);
	::SetWindowPos(hwnd,0,rc.right+1,rc.top,rc1.right - rc1.left,rc.bottom - rc.top,SWP_NOZORDER|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);

   ::SendMessage(GetDlgItem(m_hWnd,IDC_TRACK2),TBM_SETRANGE,0,MAKELPARAM(0,200));
   ::SendMessage(GetDlgItem(m_hWnd,IDC_TRACK2),TBM_SETPOS,TRUE,100);

   SendDlgItemMessage(m_hWnd,IDC_SPIN1,UDM_SETRANGE,0,MAKELONG(0,UD_MAXVAL));
   SendDlgItemMessage(m_hWnd,IDC_UPDOWN2,UDM_SETRANGE,0,MAKELONG(-100,100));
	SendDlgItemMessage(m_hWnd,IDC_UPDOWN3,UDM_SETRANGE,0,MAKELONG(-1,UD_MAXVAL));
	SendDlgItemMessage(m_hWnd,IDC_UPDOWN3,UDM_SETPOS,TRUE,-1);
	SendDlgItemMessage(m_hWnd,IDC_UPDOWN4,UDM_SETRANGE,0,MAKELONG(-1,UD_MAXVAL));
	SendDlgItemMessage(m_hWnd,IDC_UPDOWN4,UDM_SETPOS,TRUE,-1);

	UpdateScaleTransMultiplier();

   for(i=8;i<32;i++){
   	if(dwOptions & (1 << i))
       	CheckMenuItem(GetMenu(),ID_POLYGON_VIEW_ZSORT + i - 8,MF_BYCOMMAND|MF_CHECKED);
   }
	::SetFocus(GetDlgItem(m_hWnd,IDC_EDIT1));

	::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);

	imageLists[0] = ImageList_Create(16,16,ILC_COLOR16|ILC_MASK,4,4);
   if(imageLists[0] != NULL){
       bit = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_TOOLBAR_POLYVIEWER));
       if(bit != NULL){
           ImageList_AddMasked(imageLists[0],bit,RGB(255,0,255));
           ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_SETIMAGELIST,0,(LPARAM)imageLists[0]);
           ::DeleteObject(bit);
       }
   }
   imageLists[1] = ImageList_LoadImage(hInst,MAKEINTRESOURCE(IDB_TOOLBAR_POLYVIEWER_DISABLED),16,4,RGB(255,0,255),IMAGE_BITMAP,LR_DEFAULTCOLOR);
   if(imageLists[1] != NULL)
       ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_SETDISABLEDIMAGELIST,0,(LPARAM)imageLists[1]);

   i = 0;

	tbb[i].iBitmap = 3;
   tbb[i].idCommand = 1;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_CHECKGROUP;
   tbb[i].dwData = 0;
	tbb[i++].iString = -1;

	tbb[i].iBitmap = 0;
   tbb[i].idCommand = 2;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_CHECKGROUP;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

	tbb[i].iBitmap = 1;
   tbb[i].idCommand = 3;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_CHECKGROUP;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

	tbb[i].iBitmap = 2;
   tbb[i].idCommand = 4;
   tbb[i].fsState = TBSTATE_ENABLED;
   tbb[i].fsStyle = TBSTYLE_CHECKGROUP;
   tbb[i].dwData = 0;
   tbb[i++].iString = -1;

   ::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_ADDBUTTONS,(WPARAM)i,(LPARAM)&tbb);

   memset(&bmi,0,sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biWidth       = 256;
	bmi.bmiHeader.biHeight      = 192;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
   bmi.bmiHeader.biSizeImage 	= 0;
   hbRender = ::CreateDIBSection(NULL,&bmi,DIB_RGB_COLORS,(VOID**)&pBuffer,NULL,0);
   if(hbRender == NULL)
   	return;
   hdcRender = CreateCompatibleDC(NULL);
   if(hdcRender == NULL)
   	return;
	SelectObject(hdcRender,hbRender);
   hdcWindow = GetDC(GetDlgItem(m_hWnd,IDC_POLYGON));
   pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
   pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL |PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
   pfd.dwLayerMask = PFD_MAIN_PLANE;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.cColorBits = 24;
   pfd.cAlphaBits = 8;
   pfd.cDepthBits = 16;
   pfd.cStencilBits = 8;
   if((pixelformat = ChoosePixelFormat(hdcWindow,&pfd)) != 0){
       if(::SetPixelFormat(hdcWindow,pixelformat,&pfd))
           hRC = wglCreateContext(hdcWindow);
   }
   SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,SB_SETPARTS,sizeof(st_parts) / sizeof(int),(LPARAM)st_parts);
   SendMessage(WM_COMMAND,MAKEWPARAM(ID_POLYGON_VIEW_CLEAR_1,0),0);
   SendMessage(WM_COMMAND,MAKEWPARAM(ID_POLYGON_VIEW_LIGHTING,0),0);
	::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,TB_CHECKBUTTON,1,MAKELONG(TRUE,0));
}
//---------------------------------------------------------------------------
LRESULT LPolygonViewer::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	PlugIn *p;
	TV_ITEM item;
   HTREEITEM hItem;
	char szFile[MAX_PATH+1];
	LRESULT res;
	WORD wID;
	int i;
	u32 values[6];

   res = FALSE;
	switch(uMsg){
   	case WM_SIZE:
			::SendDlgItemMessage(m_hWnd,IDC_DEBUG_SB1,WM_SIZE,wParam,lParam);
   		::SendDlgItemMessage(m_hWnd,IDC_DEBUG_TOOLBAR,WM_SIZE,wParam,lParam);
       	res = TRUE;
       break;
   	case WM_INITDIALOG:
			OnInitDialog();
       break;
       case WM_COMMAND:
       	switch(wID = LOWORD(wParam)){
               case 2:
               case 3:
               case 4:
					SetFocus(GetDlgItem(m_hWnd,IDC_POLYGON));
           	case 1:
               	nMouseMode = wID;
               break;
           	case IDC_CHECK1:
               	if(HIWORD(wParam) == BN_CLICKED){
						i = ::SendMessage((HWND)lParam,BM_GETSTATE,0,0);
                       i = (i & BST_CHECKED) ? TRUE : FALSE;
                   	EnableWindow(GetDlgItem(m_hWnd,IDC_EDIT3),i);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_EDIT4),i);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_UPDOWN3),i);
                       EnableWindow(GetDlgItem(m_hWnd,IDC_UPDOWN4),i);
                   }
               break;
           	case ID_POLYGON_VIEW_SCALE:
					for(i=0;i<3;i++)
                   	fScale[i] = 1.0f;
                   UpdateTransformation(2);
               break;
               case ID_POLYGON_VIEW_TRANSLATE:
               	for(i=0;i<3;i++)
                   	fTrans[i] = 0.0f;
   				nPos[0] = 128;
   				nPos[1] = 96;
   				nPos[2] = 50;
                   UpdateTransformation(4);
               break;
               case ID_POLYGON_VIEW_ROTATE:
               	for(i=0;i<3;i++)
                   	fRotate[i] = 0.0f;
                   UpdateTransformation(3);
               break;
           	case ID_POLYGON_VIEW_CLOSE:
                 	Destroy();
               break;
               case ID_POLYGON_VIEW_REFRESH:
               	UpdatePolygon();
               break;
               case ID_POLYGON_VIEW_TEXTURE:
               	dwOptions ^= NTR_TEXTURE_OPTION;
					CheckMenuItem(GetMenu(),wID,MF_BYCOMMAND|((dwOptions & NTR_TEXTURE_OPTION) ? MF_CHECKED : MF_UNCHECKED));
                   UpdatePolygon();
               break;
               case ID_POLYGON_VIEW_LIGHT0:
               case ID_POLYGON_VIEW_LIGHT1:
               case ID_POLYGON_VIEW_LIGHT2:
               case ID_POLYGON_VIEW_LIGHT3:
					i = (1 << (8 + (wID - ID_POLYGON_VIEW_ZSORT)));
                   dwOptions ^= i;
					CheckMenuItem(GetMenu(),wID,MF_BYCOMMAND|((dwOptions & i) ? MF_CHECKED : MF_UNCHECKED));
                   UpdatePolygon();
               break;
               case ID_POLYGON_VIEW_ZSORT:
               	dwOptions ^= NTR_DEPTH_TEST_OPTION;
					CheckMenuItem(GetMenu(),wID,MF_BYCOMMAND|((dwOptions & NTR_DEPTH_TEST_OPTION) ? MF_CHECKED : MF_UNCHECKED));
                   UpdatePolygon();
               break;
               case ID_POLYGON_VIEW_CLEAR_1:
				case ID_POLYGON_VIEW_CLEAR_2:
               case ID_POLYGON_VIEW_CLEAR_4:
               case ID_POLYGON_VIEW_CLEAR_6:
					p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D);
                   if(p != NULL){
                       CheckMenuRadioItem(GetMenu(),ID_POLYGON_VIEW_CLEAR_1,ID_POLYGON_VIEW_CLEAR_6,wID,MF_BYCOMMAND);
                       values[1] = wID - ID_POLYGON_VIEW_CLEAR_1;
                       if(values[1] < 1)
                       	values[1] = 1;
                       else
                       	values[1] *= 2;
						values[0] = (u32)-1;
						p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)values);
                   }
               break;
               case ID_POLYGON_VIEW_LIGHTING:
               	dwOptions ^= NTR_DONT_USE_LIGHT;
					CheckMenuItem(GetMenu(),wID,MF_BYCOMMAND|((dwOptions & NTR_DONT_USE_LIGHT) ? MF_CHECKED : MF_UNCHECKED));
                   if((p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D)) == NULL)
                   	return res;
                   values[0] = -1;
                   values[1] = (u32)-13;
                   values[2] = (u32)(dwOptions & NTR_DONT_USE_LIGHT ? 1 : 0);
                   p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)values);
               break;
               case ID_POLYGON_VIEW_RENDER_ONLY:
               	dwOptions ^= NTR_RENDER_ONLY_POLYGONS;
					CheckMenuItem(GetMenu(),wID,MF_BYCOMMAND|((dwOptions & NTR_RENDER_ONLY_POLYGONS) ? MF_CHECKED : MF_UNCHECKED));
                   if((p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D)) == NULL)
                   	return res;
					if(!(dwOptions & NTR_RENDER_ONLY_POLYGONS)){
                       values[0] = -1;
                       values[1] = (u32)-12;
                       values[2] = (u32)0;
                       p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)values);
                       return res;
                   }
                   if(wIndex != 0){
           			hItem = TreeView_GetSelection(GetDlgItem(m_hWnd,IDC_TREE1));
           			if(hItem != NULL){
                            ZeroMemory(&item,sizeof(TV_ITEM));
                            item.mask = TVIF_PARAM|TVIF_CHILDREN;
                            item.hItem = hItem;
                            if(TreeView_GetItem(GetDlgItem(m_hWnd,IDC_TREE1),&item)){
                                if(item.cChildren == 1){
                                    hItem = TreeView_GetChild(GetDlgItem(m_hWnd,IDC_TREE1),hItem);
                                    if(hItem == NULL)
                                        return res;
                                    ZeroMemory(&item,sizeof(TV_ITEM));
                                    item.mask = TVIF_PARAM;
                                    item.hItem = hItem;
                                    if(!TreeView_GetItem(GetDlgItem(m_hWnd,IDC_TREE1),&item))
                                        return res;
                                }
                                if(item.lParam & 0x80000000){
                                    values[0] = -1;
                                    values[1] = (u32)-10;
                                    values[2] = (u32)(item.lParam & 0x7FFFFFFF);
                                    p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)values);
                                    values[0] = -1;
                                    values[1] = (u32)-11;
                                    values[2] = (u32)(item.lParam & 0x7FFFFFFF);
                                    p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)values);
                                    values[0] = -1;
                                    values[1] = (u32)-12;
                                    values[2] = (u32)1;
                                    p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)values);
                                }
                            }
						}
                   }
               break;
               case ID_POLYGON_VIEW_WIREFRAME:
               	dwOptions ^= NTR_RENDER_WIREFRAME;
					CheckMenuItem(GetMenu(),wID,MF_BYCOMMAND|((dwOptions & NTR_RENDER_WIREFRAME) ? MF_CHECKED : MF_UNCHECKED));
                   UpdatePolygon();
				break;
               case ID_POLYGON_VIEW_SAVE:
   				*((int *)szFile) = 0;
   				if(!ShowSaveDialog(NULL,szFile,"Bitmap Files (*.bmp)\0*.bmp\0TGA Files (*.tga)\0*.tga\0\0\0\0",m_hWnd,NULL))
       				return 0;
               break;
               case ID_POLYGON_VIEW_CLEAR_ALL:
					p = pPlugInContainer->get_PlugInList(PIL_VIDEO)->get_ActivePlugIn(PIT_IS3D);
                   if(p != NULL){
						values[0] = (u32)-1;
                       values[1] = (u32)-1;
						p->NotifyState(PNMV_DRAWPOLYGON,PIS_NOTIFYMASK,(LPARAM)values);
					}
               break;
				default:
               break;
           }
       break;
       case WM_CLOSE:
       	Destroy();
       break;
       case WM_HSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_TRACK1:
                   DrawPolygon();
               break;
               case IDC_TRACK2:
               	UpdateScaleTransMultiplier();
               break;
               case IDC_HSBSPR:
                   switch(LOWORD(wParam)){
                       case SB_LINEUP:
                           i = xScroll - 1;
                       break;
                       case SB_LINEDOWN:
                           i = xScroll + 1;
                       break;
                       case SB_THUMBPOSITION:
                       case SB_THUMBTRACK:
                           i = HIWORD(wParam);
                       break;
                       default:
                           i = xScroll;
                       break;
                   }
                   if(i != xScroll){
                       xScroll = i;
                       SetScrollPos((HWND)lParam,SB_CTL,xScroll,TRUE);
                       DrawPolygon();
                   }
               break;
           }
       break;
       case WM_VSCROLL:
           wID = (WORD)GetDlgCtrlID((HWND)lParam);
           switch(wID){
               case IDC_SPIN1:
                   switch(LOWORD(wParam)){
                       case SB_THUMBPOSITION:
                       	wIndex = HIWORD(wParam);
                           UpdatePolygon();
                       break;
                   }
               break;
               case IDC_VSBTEX:
                   switch(LOWORD(wParam)){
                       case SB_LINEUP:
                           i = yScroll - 1;
                       break;
                       case SB_LINEDOWN:
                           i = yScroll + 1;
                       break;
                       case SB_THUMBPOSITION:
                       case SB_THUMBTRACK:
                           i = HIWORD(wParam);
                       break;
                       default:
                           i = yScroll;
                       break;
                   }
                   if(i != yScroll){
                       yScroll = i;
                       SetScrollPos((HWND)lParam,SB_CTL,yScroll,TRUE);
                       DrawPolygon();
                   }
               break;
           }
		break;
       case WM_NOTIFY:
       	if(wParam == IDC_TREE1)
           	OnNotifyTreeView1((LPNM_TREEVIEW)lParam);
       break;
   }
   return res;
}
//---------------------------------------------------------------------------
void LPolygonViewer::OnNotifyTreeView1(LPNM_TREEVIEW hdr)
{
	TV_ITEM item;
   HTREEITEM hItem;

	switch(hdr->hdr.code){
   	case NM_DBLCLK:
           hItem = TreeView_GetSelection(hdr->hdr.hwndFrom);
           if(hItem == NULL)
           	return;
           ZeroMemory(&item,sizeof(TV_ITEM));
           item.mask = TVIF_PARAM;
           item.hItem = hItem;
       	if(!TreeView_GetItem(hdr->hdr.hwndFrom,&item))
           	return;

       break;
	}
}
#endif
