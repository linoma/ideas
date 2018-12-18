#include "lds.h"
#include "gfx/modetile.h"
#include "gfx/mode4.h"
#include "gfx/mode6.h"
#include "videoplug.h"
#include "io.h"

#define GPU_MIN_CORES  4

LDownLcd downLcd;
LUpLcd upLcd;
//---------------------------------------------------------------------------
LGPU::LGPU() : LCanvas(),LWnd()
{
	int i;

   SourceBuffer = NULL;
   nRotate = ID_WINDOW_ROTATE0;
	pMainMemory = NULL;
   for(i=0;i<2;i++)
   	win[i].Visible = 1;
   swapBuffer[0] = &LGPU::rgbNormalLine;
   swapBuffer[1] = &LGPU::rgbAlphaLayerLine;
   swapBuffer[2] = &LGPU::rgbFadeLineUp;
	swapBuffer[3] = &LGPU::rgbFadeLineDown;
   swapBuffer[4] = &LGPU::rgbNormalLineNoOAM;
   swapBuffer[5] = &LGPU::rgbAlphaLayerLineNoOAM;
   swapBuffer[6] = &LGPU::rgbFadeLineUpNoOAM;
	swapBuffer[7] = &LGPU::rgbFadeLineDownNoOAM;
   fScale = 1;
}
//---------------------------------------------------------------------------
LGPU::~LGPU()
{
	LCanvas::Destroy();
   LWnd::Destroy();
   if(pMainMemory != NULL)
   	delete []pMainMemory;
   if(SourceBuffer != NULL){
       LocalFree(SourceBuffer);
       SourceBuffer = NULL;
   }
}
//---------------------------------------------------------------------------
BOOL LGPU::Init()
{
   SourceBuffer = (u8 *)LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT,5120+600+600+0x100+0x100+(256*192*sizeof(u16)) +
       128*sizeof(SPRITE) + 32 * sizeof(SROTMATRIX) + 132*4 + 5 * sizeof(LAYER) + 16 * sizeof(LPLAYER));
   if(SourceBuffer == NULL)
       return FALSE;
	TargetBuffer = &SourceBuffer[1024];
   SpriteBuffer = &SourceBuffer[2048];
   ZBuffer = &SourceBuffer[3072];
   WinObjSprite = &SourceBuffer[4096];
   tabColor = &SourceBuffer[5120];
   tabColor1 = tabColor + 600;
   tpal_bg = (u16 *)(tabColor1 + 600);
   tpal_obj = (tpal_bg + 0x100);
   sprite_buffer = (SPRITE *)(tpal_obj + 0x100);
   rotMatrix = (SROTMATRIX *)(sprite_buffer+128);
   sprite_priority[0] = (u8 *)(rotMatrix+32);
   sprite_priority[1] = sprite_priority[0] + 132;
   sprite_priority[2] = sprite_priority[1] + 132;
   sprite_priority[3] = sprite_priority[2] + 132;
   layers = (LAYER *)(sprite_priority[3] + 132);
   Source = (LPLAYER *)(layers + 5);
   Target = (LPLAYER *)(Source + 8);
   pRotateBuffer = (u16 *)(Target + 8);
   for(int i=0;i<5;i++)
   	layers[i].Visible = 1;
	if(!CreateWnd())
   	return FALSE;
   if(!LCanvas::Init(m_hWnd))
       return FALSE;
   if(type == LCD_MAIN){
   	if((pMainMemory = new u16[256*192]) == NULL)
       	return FALSE;
   }
	return (BOOL)TRUE;
}
//---------------------------------------------------------------------------
int LGPU::BitBlt(HDC hdc)
{
   int i,spos,dpos,desp,r,g,b;
   void *p;
   u8 *p1;
	u16 *p2;

   if(hdc == NULL)
       hdc = hDC;
   if(ds.get_EmuMode() == 0){
       switch(BRIGHTCNT >> 14){
           case 1:
               p2 = (u16 *)tempBuffer;
               spos = BRIGHTCNT & 0x1F;
               if(spos > 16)
                   spos = 16;
               p1 = tabColor1 + (spos << 5);
               for(i=0;i<256*192;i++){
                   dpos = *p2;
                   r = dpos & 0x1F;
                   g = (dpos >> 5) & 0x1F;
                   b = (dpos >> 10) & 0x1F;
                   if((r += p1[r]) > 31) r = 31;
                   if((g += p1[g]) > 31) g = 31;
                   if((b += p1[b]) > 31) b = 31;
                   *p2++ = (u16)(r | (g << 5) | (b << 10));
               }
           break;
           case 2:
               p2 = (u16 *)tempBuffer;
               spos = BRIGHTCNT & 0x1F;
               if(spos > 16)
                   spos = 16;
               p1 = tabColor + (spos << 5);
               for(i=0;i<256*192;i++){
                   dpos = *p2;
                   r = (dpos & 0x1F);
                   g = (dpos >> 5) & 0x1F;
                   b = (dpos >> 10) & 0x1F;
                   r -= p1[r];
                   g -= p1[g];
                   b -= p1[b];
                   *p2++ = (u16)(r | (g << 5) | (b << 10));
               }
           break;
       }
   }
   switch(nRotate){
       case ID_WINDOW_ROTATE0:
           return LCanvas::BitBlt(hdc);
       case ID_WINDOW_ROTATE90:
           p = type == LCD_SUB ? downLcd.get_Buffer() : upLcd.get_Buffer();
           desp=0;
           for(i=0;i<256*192;i+=192){
               spos = 256*191 + desp;
               dpos = i;
               while(spos > 0){
                   pRotateBuffer[dpos++] = ((u16 *)p)[spos];
                   spos -= 256;
               }
               desp++;
           }
           return LCanvas::BitBlt(hdc,(char *)pRotateBuffer);
       case ID_WINDOW_ROTATE180:
           p = type == LCD_SUB ? downLcd.get_Buffer() : upLcd.get_Buffer();
           return LCanvas::BitBlt(hdc,(char *)p);
       case ID_WINDOW_ROTATE270:
           desp = 255;
           for(i=0;i<256*192;i += 192){
               dpos = i;
               spos = desp;
               while(spos < 256*192){
                   pRotateBuffer[dpos++] = ((u16 *)screen)[spos];
                   spos+=256;
               }
               desp--;
           }
           return LCanvas::BitBlt(hdc,(char *)pRotateBuffer);
   }
   return 0;
}
//---------------------------------------------------------------------------
void LGPU::set_Scale(float value)
{
   RECT rc;

   if(value == fScale)
       return;
   if(value == 0)
       value = 1;
   else if(value < 0)
       value *= -1;
   fScale = value;
   get_Rect(rc);
   set_Rect(rc);
}
//---------------------------------------------------------------------------
void LGPU::set_Rect(RECT &rc)
{
   RECT rc1;
   SIZE sz;

   sz.cx = rc.right - (rc.left << 1);
   sz.cy = rc.bottom - (rc.top << 1);
   rc.right *= fScale;
   rc.bottom *= fScale;
   ::SetRect(&rc1,0,0,rc.right,rc.bottom);
   AdjustWindowRect(&rc1);
   rc1.right -= rc1.left;
   rc1.bottom -= rc1.top;
   SetWindowPos(NULL,0,0,rc1.right,rc1.bottom,SWP_NOMOVE|SWP_NOZORDER);
   rc1.right = sz.cx*fScale;
   rc1.bottom = sz.cy*fScale;
   rc1.left = rc.left * fScale;
   rc1.top = rc.top * fScale;
   set_WorkArea(rc1,sz);
//   LCanvas::SetRect(right,bottom);
}
//---------------------------------------------------------------------------
void LGPU::get_Rect(RECT &rc)
{
   switch(nRotate){
       case ID_WINDOW_ROTATE0:
       case ID_WINDOW_ROTATE180:
           rc.right = 256;
           rc.bottom = 192;
           rc.top = rc.left = 0;
       break;
       case ID_WINDOW_ROTATE270:
       case ID_WINDOW_ROTATE90:
           rc.right = 256;
           rc.bottom = 256;
           rc.left = 32;
           rc.top = 0;
       break;
   }
}
//---------------------------------------------------------------------------
BOOL LGPU::Rotate(WORD wID)
{
   RECT rc;

   nRotate = wID;
   get_Rect(rc);
   set_Rect(rc);
   return TRUE;
}
//---------------------------------------------------------------------------
void LGPU::Reset()
{
	int i;
	u8 i1;

   ResetFrame();
   for(i=0;i<2;i++){
   	i1 = win[i].Visible;
       ZeroMemory(&win[i],sizeof(WINGBA));
   	win[i].Visible = i1;
   }
   for(i=0;i<5;i++){
   	i1 = layers[i].Visible;
   	ZeroMemory(&layers[i],sizeof(LAYER));
   	layers[i].parent = this;
       layers[i].bDrawEnable = 1;
       layers[i].Visible = i1;
       layers[i].Index = (u8)i;
       layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
      	layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
   }
   for(i=0;i<7;i++)
       Source[i] = Target[i] = NULL;
   for(i=0;i<17;i++){
       for(i1=0;i1<32;i1++)
           tabColor[i*32+i1] = (u8)(i1 * i >> 4);
   }
   for(i=0;i<17;i++){
       for(i1=0;i1<32;i1++)
           tabColor1[i*32+i1] = (u8)((31 - i1) * i >> 4);
   }
   FillMemory(SpriteBuffer,sizeof(SpriteBuffer),0xFF);
   ZeroMemory(tpal_bg,sizeof(tpal_bg));
   ZeroMemory(tpal_obj,sizeof(tpal_obj));
   xMosaic = yMosaic = 0;
   SpriteEnable = 0;
   DrawMode = 0xFF;
   iBlend = 0;
   texpal_bg = NULL;
   eva = evy = evb = 0;
   peva = pevb = pevy = tabColor;
   pevy1 = tabColor1;
   GetPixelSprite = &LGPU::GetPXSprite;
   reset_Sprite();
	set_PowerMode();
   LCanvas::Reset();
   if(hDC != NULL)
       BitBlt();
}
//---------------------------------------------------------------------------
void LGPU::Command(u16 address,u32 data,u8 accessMode)
{
	u8 i;
   LPLAYER layer;
   LPWINGBA w;
   LPWINCONTROL winc;
   s32 s;

   if(/*type == LCD_MAIN || */(type != LCD_SUB && address > 0x7FF) || (type == LCD_SUB && address < 0x400))
   	return;
   address &= 0x3FF;
	switch(address){
       case 0:
       case 1:
       case 2:
       case 3:
           if((i = (u8)(DISPCNT & 7)) != DrawMode){
               DrawMode = i;
               switch(i){
                   case 0:
                       for(i=0;i<4;i++){
                           layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                           layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                           layers[i].postDraw      = NULL;//PostdrawPixelMode0;
                           layers[i].postDrawLine = NULL;
                       }
                   break;
                   case 1:
                   	if(ds.get_EmuMode() == 0){
                           for(i=0;i<3;i++){
                               layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                               layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                               layers[i].postDraw      = NULL;//PostdrawPixelMode0;
                               layers[i].postDrawLine  = NULL;
                           }
                           for(;i<4;i++){
                               layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode1;
                               layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode1;
                               layers[i].postDraw      = (POSTDRAWPIXEL)PostDrawMode1;
                               layers[i].postDrawLine  = PostDrawLineMode1;
                           }
                       }
                       else{
                           for(i=0;i<2;i++){
                               layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                               layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                               layers[i].postDraw      = NULL;//PostdrawPixelMode0;
                           }
                           for(;i<3;i++){
                               layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode1;
                               layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode1;
                               layers[i].postDraw      = (POSTDRAWPIXEL)PostDrawMode1;
                           }
                           layers[3].drawPixel         = NULL;
                           layers[3].initDrawLine      = NULL;
                           layers[3].Enable = 0;
                       }
                   break;
                   case 2:
                   	if(ds.get_EmuMode() == 0){
                           for(i=0;i<2;i++){
                               layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                               layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                               layers[i].postDraw      = NULL;
                           }
                           for(;i<4;i++){
                               layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode1;
                               layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode1;
                               layers[i].postDraw      = (POSTDRAWPIXEL)PostDrawMode1;
                               layers[i].postDrawLine  = (POSTDRAWLINE)PostDrawLineMode1;
                           }
                       }
                       else{
                           for(i=0;i<2;i++){
                               layers[i].drawPixel     = NULL;
                               layers[i].initDrawLine  = NULL;
                               layers[i].postDraw      = NULL;
                           }
                           for(;i<4;i++){
                               layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode1;
                               layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode1;
                               layers[i].postDraw      = (POSTDRAWPIXEL)PostDrawMode1;
                           }
                       }
                   break;
                   case 3:
                       for(i=0;i<3;i++){
                           layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                           layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                           layers[i].postDraw      = NULL;//PostdrawPixelMode0;
                       }
                       for(;i<4;i++){
                           layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode4Rot;
                           layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode4Rot;
                           layers[i].postDraw      = (POSTDRAWPIXEL)PostDrawMode1;
                           layers[i].postDrawLine  = (POSTDRAWLINE)PostDrawLineMode1;
                       }
                   break;
                   case 4:
                       if(ds.get_EmuMode() == 0){
                           for(i=0;i<2;i++){
                               layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                               layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                               layers[i].postDraw      = NULL;//PostdrawPixelMode0;
                           }
                           layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode1;
                           layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode1;
                           layers[i].postDraw    = (POSTDRAWPIXEL)PostDrawMode1;
                           layers[i++].postDrawLine  = (POSTDRAWLINE)PostDrawLineMode1;
                           for(;i<4;i++){
                               layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode4Rot;
                               layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode4Rot;
                               layers[i].postDraw      = (POSTDRAWPIXEL)PostDrawMode1;
                               layers[i].postDrawLine  = (POSTDRAWLINE)PostDrawLineMode1;
                           }
                       }
                       else{
                           for(i=0;i<4;i++){
                               layers[i].Enable = 0;
                               layers[i].drawPixel = (DRAWPIXEL)NULL;
                               layers[i].initDrawLine = NULL;
                               layers[i].postDraw = NULL;
                           }
                           layers[2].drawPixel = (DRAWPIXEL)drawPixelMode4;
                           layers[2].initDrawLine = (INITDRAWLINE)InitLayerMode4;
                           layers[2].postDraw = (POSTDRAWPIXEL)PostDrawMode1;
                           layers[2].postDrawLine  = (POSTDRAWLINE)PostDrawLineMode1;
                       }
                   break;
                   case 5:
                       for(i=0;i<2;i++){
                           layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                           layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                           layers[i].postDraw      = NULL;//PostdrawPixelMode0;
                           layers[i].postDrawLine  = NULL;
                       }
                       for(;i<4;i++){
                           layers[i].drawPixel     = (DRAWPIXEL)drawPixelMode4Rot;
                           layers[i].initDrawLine  = (INITDRAWLINE)InitLayerMode4Rot;
                           layers[i].postDraw      = (POSTDRAWPIXEL)PostDrawMode1;
                           layers[i].postDrawLine  = (POSTDRAWLINE)PostDrawLineMode1;
                       }
                   break;
                   case 6:
                       for(i=0;i<4;i++){
                           layers[i].Enable = 0;
                           layers[i].drawPixel = (DRAWPIXEL)NULL;
                           layers[i].initDrawLine = NULL;
                           layers[i].postDraw = NULL;
                       }
                       layers[2].drawPixel = (DRAWPIXEL)drawPixelMode6;
                       layers[2].initDrawLine = (INITDRAWLINE)InitLayerMode6Rot;
                       layers[2].postDraw = (POSTDRAWPIXEL)PostDrawMode6Rot;
                       layers[2].postDrawLine  = (POSTDRAWLINE)PostDrawLineMode1;
                   break;
                   default:
                       for(i=0;i<4;i++){
                           layers[i].Enable = 0;
                           layers[i].drawPixel = (DRAWPIXEL)NULL;
                           layers[i].initDrawLine = NULL;
                           layers[i].postDraw = NULL;
                       }
                   break;
               }
           }
           SpriteEnable = (u8)((DISPCNT & 0x1000) >> 12);
			if(ds.get_EmuMode() != EMUM_GBA){
               if((layers[0].Enable = (u8)((DISPCNT & 0x100) >> 8)) != 0){
                   if((DISPCNT & 8)){
                       layers[0].drawPixel     = (DRAWPIXEL)drawPixelMode3D;
                       layers[0].initDrawLine  = (INITDRAWLINE)InitLayerMode3D;
                       layers[0].postDraw      = NULL;
                       layers[0].postDrawLine = NULL;
                   }
                   else{
                       layers[0].drawPixel     = (DRAWPIXEL)drawPixelMode0;
                       layers[0].initDrawLine  = (INITDRAWLINE)InitLayerMode0;
                       layers[0].postDraw      = NULL;//PostdrawPixelMode0;
                       layers[0].postDrawLine = NULL;
                   }
               }
           }
           else if((layers[0].Enable = (u8)((DISPCNT & 0x100) >> 8)) != 0)
           	FillBaseLayer(&layers[0]);
           if((layers[1].Enable = (u8)((DISPCNT & 0x200) >> 9)) != 0)
               FillBaseLayer(&layers[1]);
           if((layers[2].Enable = (u8)((DISPCNT & 0x400) >> 10)) != 0)
               FillBaseLayer(&layers[2]);
           if((layers[3].Enable = (u8)((DISPCNT & 0x800) >> 11)) != 0)
               FillBaseLayer(&layers[3]);
           win[0].Enable = (u8)((DISPCNT & 0x2000) >> 13);
           win[1].Enable = (u8)((DISPCNT & 0x4000) >> 14);
           winObj.Enable = (u8)(((DISPCNT & 0x8000) >> 15) & SpriteEnable);
           WinEnable = (u8)(win[0].Enable | (win[1].Enable << 1));
           FillListBackground();
           GetBackgroundRect(0);
           GetBackgroundRect(1);
           GetBackgroundRect(2);
           GetBackgroundRect(3);
           if(ds.get_EmuMode() != EMUM_GBA){
               if(bDoubleSize != ((DISPCNT & 0x10) >> 4)){
                   bDoubleSize = (u8)((DISPCNT & 0x10) >> 4);
                   RemapAllSprite();
               }
           }
           else{
                if(bDoubleSize != ((DISPCNT & 0x40) >> 6)){
                    bDoubleSize = (u8)((DISPCNT & 0x40) >> 6);
                    RemapAllSprite();
                }
			}
       break;
       case 4:
       case 5:
           DISPSTAT &= ~7;
           DISPSTAT |= (u16)(ds.get_CurrentStatus() & 7);
           DISPSTAT7 &= ~7;
           DISPSTAT7 |= (u16)(ds.get_CurrentStatus() & 7);
           if(accessMode == AMM_WORD && type == LCD_MAIN){
               if(pPlugInContainer != NULL)
                   pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_CHANGEVCOUNT,PIS_NOTIFYMASK,(LPARAM)io_mem[6]);
           }
       break;
       case 6:
       case 7:
       	if(pPlugInContainer != NULL)
   	    	pPlugInContainer->NotifyState(PIL_VIDEO,PNMV_CHANGEVCOUNT,PIS_NOTIFYMASK,(LPARAM)io_mem[6]);
       break;
       case 8:
       case 9:
           FillBaseLayer(&layers[0]);
           GetBackgroundRect(0);
           if(accessMode == AMM_WORD){
               FillBaseLayer(&layers[1]);
               GetBackgroundRect(1);
           }
       break;
       case 10:
       case 11:
           FillBaseLayer(&layers[1]);
           GetBackgroundRect(1);
           if(accessMode == AMM_WORD){
               FillBaseLayer(&layers[2]);
               GetBackgroundRect(2);
           }
       break;
       case 12:
       case 13:
           FillBaseLayer(&layers[2]);
           GetBackgroundRect(2);
           if(accessMode == AMM_WORD){
               FillBaseLayer(&layers[3]);
               GetBackgroundRect(3);
           }
       break;
       case 14:
       case 15:
           FillBaseLayer(&layers[3]);
           GetBackgroundRect(3);
       break;
       case 16:
       case 17:
       case 18:
       case 19:
           layer = &layers[0];
           layer->bg[0] = ((u16 *)reg)[8] & 0x3FF;
           layer->bg[1] = ((u16 *)reg)[9] & 0x3FF;
       break;
       case 20:
       case 21:
       case 22:
       case 23:
           layer = &layers[1];
           layer->bg[0] = ((u16 *)reg)[10] & 0x3FF;
           layer->bg[1] = ((u16 *)reg)[11] & 0x3FF;
       break;                                             //803cd16
       case 24:
       case 25:
       case 26:
       case 27:
           layer = &layers[2];
           layer->bg[0] = ((u16 *)reg)[12] & 0x3FF;
           layer->bg[1] = ((u16 *)reg)[13] & 0x3FF;
       break;
       case 28:
       case 29:
       case 30:
       case 31:
           layer = &layers[3];
           layer->bg[0] = ((u16 *)reg)[14] & 0x3FF;
           layer->bg[1] = ((u16 *)reg)[15] & 0x3FF;
       break;
       case 32:
       case 33:
       case 34:
       case 35:
           layer = &layers[2];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer);
           layer->rotMatrix[0] = (s16)((u16 *)reg)[0x10];
           layer->rotMatrix[1] = (s16)((u16 *)reg)[0x11];
       break;
       case 36:
       case 37:
       case 38:
       case 39:
           layer = &layers[2];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer);
           layer->rotMatrix[2] = (s16)((u16 *)reg)[0x12];
           layer->rotMatrix[3] = (s16)((u16 *)reg)[0x13];
       break;
       case 40:
       case 41:
       case 42:
       case 43:
           layer = &layers[2];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer);
           if(((s = ((u32 *)reg)[0xA] & 0x0FFFFFFF) & 0x08000000) != 0)
               s = -(0x10000000 - s);
           layer->CurrentX = layer->bgrot[0] = s;
       break;
       case 44:
       case 45:
       case 46:
       case 47:
           layer = &layers[2];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer);
           if(((s = ((u32 *)reg)[0xB] & 0x0FFFFFFF) & 0x08000000) != 0)
               s = -(0x10000000 - s);
           layer->CurrentY = layer->bgrot[1] = s;
       break;
       case 48:
       case 49:
       case 50:
       case 51:
           layer = &layers[3];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer);
           layer->rotMatrix[0] = (s16)((u16 *)reg)[0x18];
           layer->rotMatrix[1] = (s16)((u16 *)reg)[0x19];
       break;
       case 52:
       case 53:
       case 54:
       case 55:
           layer = &layers[3];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer);
           layer->rotMatrix[2] = (s16)((u16 *)reg)[0x1A];
           layer->rotMatrix[3] = (s16)((u16 *)reg)[0x1B];
       break;
       case 56:
       case 57:
       case 58:
       case 59:
           layer = &layers[3];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer);
           if(((s = ((u32 *)reg)[0xE] & 0x0FFFFFFF) & 0x08000000) != 0)
               s = -(0x10000000 - s);
           layer->CurrentX = layer->bgrot[0] = s;
       break;
       case 60:
       case 61:
       case 62:
       case 63:
           layer = &layers[3];
           if(layer->Priority == 0xFF)
               FillBaseLayer(layer);
           if(((s = ((u32 *)reg)[0xF] & 0x0FFFFFFF) & 0x08000000) != 0)
               s = -(0x10000000 - s);
           layer->CurrentY = layer->bgrot[1] = s;
       break;
       case 64:
       case 65:
       case 66:
       case 67:
           w = &win[0];
           w->left = (u8)(((u16 *)reg)[0x20] >> 8);
           w->right = (u8)((u16 *)reg)[0x20];
           if(w->left > w->right)
           	w->right = 255;
           w = &win[1];
           w->left = (u8)(((u16 *)reg)[0x21] >> 8);
           w->right = (u8)((u16 *)reg)[0x21];
           if(w->left > w->right)
           	w->right = 255;
       break;
       case 68:
       case 69:
       case 70:
       case 71:
           w = &win[0];
           w->top = (u8)(((u16 *)reg)[0x22] >> 8);
           w->bottom = (u8)((u16 *)reg)[0x22];
           if(w->top > w->bottom)
           	w->bottom = 160;
           w = &win[1];
           w->top = (u8)(((u16 *)reg)[0x23] >> 8);
           w->bottom = (u8)((u16 *)reg)[0x23];
           if(w->top > w->bottom)
           	w->bottom = 160;
       break;
       case 72:
       case 73:
       case 74:
       case 75:
           winc = &win[0].Control;
           for(i=0;i<4;i++)
               winc->EnableBg[i] = (WININ & (1 << i)) ? &layers[i] : NULL;
           winc->EnableObj = (u8)((WININ & 0x10) >> 4);
           winc->EnableBlend = (u8)((WININ & 0x20) >> 5);
           winc = &win[1].Control;
           for(i=0;i<4;i++)
               winc->EnableBg[i] = (WININ & (1 << (i + 8))) ? &layers[i] : NULL;
           winc->EnableObj = (u8)((WININ & 0x1000)>> 12);
           winc->EnableBlend = (u8)((WININ & 0x2000) >> 13);
           winc = &winOut;
           for(i=0;i<4;i++)
               winc->EnableBg[i] = (WINOUT & (1 << i)) ? &layers[i] : NULL;
           winc->EnableObj = (u8)((WINOUT & 0x10) >> 4);
           winc->EnableBlend = (u8)((WINOUT & 0x20) >> 5);
           winc = (LPWINCONTROL)&winObj;
           for(i=0;i<4;i++)
               winc->EnableBg[i] = (WINOUT & (1 << (i + 8))) ? &layers[i] : NULL;
           winc->EnableObj = (u8)((WINOUT & 0x1000) >> 12);
           winc->EnableBlend = (u8)((WINOUT & 0x2000) >> 13);
       break;
       case 76:
       case 77:
           i = (u8)(MOSAIC & 0x0f);
           layers[0].xMosaic = i;
           layers[1].xMosaic = i;
           layers[2].xMosaic = i;
           layers[3].xMosaic = i;
           i = (u8)((MOSAIC >> 4) & 0x0f);
           layers[0].yMosaic = i;
           layers[1].yMosaic = i;
           layers[2].yMosaic = i;
           layers[3].yMosaic = i;
           sprxMosaic = (u8)((MOSAIC >> 8) & 0xF);
           spryMosaic = (u8)((MOSAIC >> 12) & 0xF);
       break;
       case 80:
       case 81:
           iBlend = (u8)(((BLENDCNT >> 6) & 3));
           switch(iBlend){
               case 0:
                   GetPixelSprite = &LGPU::GetPXSprite;
               break;
               case 1:
                   GetPixelSprite = &LGPU::GetPXSpriteAlpha;
               break;
               default:
                   GetPixelSprite = &LGPU::GetPXSpriteBrightness;
               break;
           }
           FillListBackground();                                //8004994  8001a40
           Source[5] = (LPLAYER)((BLENDCNT & 0x10) >> 4);
           Target[5] = (LPLAYER)((BLENDCNT & 0x1000) >> 12);
           Source[6] = (LPLAYER)((BLENDCNT & 0x20) >> 5);//backdrop
           Target[6] = (LPLAYER)((BLENDCNT & 0x2000) >> 13);//backdrop
           Source[7] = (LPLAYER)(BLENDCNT & 0x2F);
           Target[7] = (LPLAYER)(BLENDCNT & 0x2F00);
           if(accessMode == AMM_WORD)
               GetAlphaIndex();
       break;
       case 82:                                                 //80022cc  //803b44
       case 83:
           GetAlphaIndex();
           if(accessMode == AMM_WORD)
               GetBrightnessIndex();
       break;
       case 84:
       case 85:
           GetBrightnessIndex();
       break;
       case 0x64:
       case 0x65:
       case 0x66:
       case 0x67:
       	if(type == LCD_MAIN && (data & 0x80000000))
           	ds.OnDisplayCapture(data);
       break;
       case 0x68:
       	if(pMainMemory != NULL){
               while(accessMode > 3){
                   //((u16 *)tempBuffer)[dwMainMemoryIndex] = (u16)data;
                   pMainMemory[dwMainMemoryIndex++] = (u16)data;
                   accessMode >>= 1;
                   data >>= 16;
               }
           }
       break;
       default:
       	data = 0;
       break;
   }
}
//---------------------------------------------------------------------------
void LGPU::SwapMainMemory()
{
	if(!is_MainMemoryUsed())
   	return;
   memcpy(tempBuffer,pMainMemory,256*192*sizeof(u16));
}
//---------------------------------------------------------------------------
void LGPU::ResetFrame()
{
	dwMainMemoryIndex = 0;
   toutBuffer = (u16 *)get_OutBuffer();
   layers[2].CurrentX = layers[2].bgrot[0];
   layers[2].CurrentY = layers[2].bgrot[1];
   layers[3].CurrentX = layers[3].bgrot[0];
   layers[3].CurrentY = layers[3].bgrot[1];
   if(layers[2].parent != NULL)
       FillBaseLayer_Internal(&layers[2]);
   if(layers[3].parent != NULL)
       FillBaseLayer_Internal(&layers[3]);
}
//---------------------------------------------------------------------------
void LGPU::RenderLine()
{
	int x;
	u16 *v;
	u32 col;

//	if(type == LCD_SUB)
//   	return;
	if(!ds.get_EmuMode()){
       if(type == LCD_MAIN)
           col = (u16)(((u16 *)reg)[1] & 3);
       else
           col = (u16)(((u16 *)reg)[1] & 1);
   }
   else{
   	if(type == LCD_SUB)
       	col = 0;
       else{
           if(DrawMode < 3)
               col = 1;
           else
               col = (u32)DrawMode;
       }
   }
/*   switch(type){
       case LCD_MAIN:
           if(!(POWCNT1 & 2))
               col = 0;
       break;
       case LCD_SUB:
           if(!(POWCNT1 & 0x200))
               col = 0;
       break;
   }*/
   switch(col){
   	case 0:
           for(x=0;x<256;x++)
               *toutBuffer++ = 0;
       break;
       case 1:
       	ubDrawSprite = 1;
       	pCurrentZBuffer = (u32 *)ZBuffer;
       	pCurSB = (u32 *)SourceBuffer;
       	pCurOB = (u32 *)SpriteBuffer;
           if(WinEnable)
           	drawLineModeTileWindow();
           else{
  				drawLineModeTile();
           	draw_Sprite(0,256);
           }
       break;
       case 2:
           x = ((DISPCNT1 >> 2) & 3);
           if(io_mem[0x240 + x] != 0x80){
           	for(x=0;x<4;x++){
               	if(io_mem[0x240+x] == 0x80)
                   	break;
               }
           }
           ubDrawSprite = 1;
       	pCurrentZBuffer = (u32 *)ZBuffer;
       	pCurSB = (u32 *)SourceBuffer;
       	pCurOB = (u32 *)SpriteBuffer;
           x = (x << 16) + (render_line << 8);
           col = VRAMmap[512 + ((x>>14)&0x3ff)];
           if(col != 41){
           	x = (col << 14) + (x & 0x3FFF);
           	v = &((u16 *)video_mem)[x];
           	x = (col << 14) + (x & 0x3FFF);
           	v = &((u16 *)video_mem)[x];
               x = 0;
               if(layers[0].Visible && layers[0].Enable && (DISPCNT & 8)){
                   if(ds.get_ActiveVideoPlugIn() != NULL){
                       for(;x<256;x++){
                           col = ds.get_ActiveVideoPlugIn()->Run(x,render_line);
                           pCurSB[x] = col;
                           ((u32 *)TargetBuffer)[x] = (u32)-1;
                           *pCurrentZBuffer++ = (u32)-1;
                       }
                   }
               }
               if(!x && layers[0].Visible){
                   for(;x<256;x++){
                       pCurSB[x] = BGR(*v);
                       v++;
                       ((u32 *)TargetBuffer)[x] = (u32)-1;
                       *pCurrentZBuffer++ = (u32)-1;
                   }
               }
               draw_Sprite(0,256);
           }
           else {
               if(WinEnable)
                   drawLineModeTileWindow();
               else{
                   drawLineModeTile();
                   draw_Sprite(0,256);
               }
           }
       break;
       case 3:
       	x = 0;
       break;
       case 4:
       	ubDrawSprite = 1;
       	pCurrentZBuffer = (u32 *)ZBuffer;
       	pCurSB = (u32 *)SourceBuffer;
       	pCurOB = (u32 *)SpriteBuffer;
       	if(WinEnable)
           	drawLineModeTileWindow();
           else{
               drawLineModeTile();
               draw_Sprite(0,256);
           }
       break;
   }
}
//---------------------------------------------------------------------------
void LGPU::set_VRAM(u8 bank,u8 data)
{
	int i;

   if(ds.get_EmuMode() == 0){
       i = VRAMmap[256 + (type == LCD_MAIN ? 0 : 128)];
       vram_obj = video_mem + (i << 14);
       //   i = get_MemoryBank(i);
       i = mapVRAM[i];
       if(type == LCD_MAIN){
           if(i == 0 || i == 1 || i == 4 || i == 5 || i == 6){
               if((io_mem[0x240] & 0x87) == 0x82)
                   layers[4].bDrawEnable = 1;
               else if((io_mem[0x241] & 0x87) == 0x82)
                   layers[4].bDrawEnable = 1;
               else if((io_mem[0x244] & 0x87) == 0x82)
                   layers[4].bDrawEnable = 1;
               else if((io_mem[0x245] & 0x87) == 0x82)
                   layers[4].bDrawEnable = 1;
               else if((io_mem[0x246] & 0x87) == 0x82)
                   layers[4].bDrawEnable = 1;
               else
                   layers[4].bDrawEnable = 0;
           }
       }
       else{
           if(i == 3 || i == 9){
               if((io_mem[0x243] & 0x87) == 0x84)
                   layers[4].bDrawEnable = 1;
               else if((io_mem[0x249] & 0x87) == 0x82)
                   layers[4].bDrawEnable = 1;
               else
                   layers[4].bDrawEnable = 0;
           }
       }
       for(i=0;i<4;i++){
           FillBaseLayer_Internal(&layers[i]);
//           layers[i].bDrawEnable = get_LayerBank(get_LayerAddress(&layers[i],FALSE));
//           layers[i].bDrawEnable |= (u8)(get_LayerBank(get_LayerAddress(&layers[i],TRUE)) << 1);
       }
       switch(type){
           case LCD_MAIN:
               switch(bank){
                   case 4://E
                       if((data & 0x80)){
                           switch((data & 7)){
                               case 4:
                                   texpal_bg = (u16 *)(video_mem + 0x80000);// + (((data >> 3) & 3) << 13));
                                   texpal_slot = 0xF;
                               break;
                               case 5:
                                   texpal_obj = (u16 *)(video_mem + 0x80000);
                               break;
                           }
                       }
                   break;
                   case 5://F
                       if((data & 0x80)){
                           switch(data & 7){
                               case 4:
                                   texpal_bg = (u16 *)(video_mem + 0x90000);// + (((data >> 3) & 1) << 13));
                                   if(!((data >> 3) & 1))
                                       texpal_slot |= 3;
                                   else
                                       texpal_slot |= 12;
                               break;
                               case 5:
                                   texpal_obj = (u16 *)(video_mem + 0x90000);
                               break;
                               default:
                                   if((io_mem[0x244] & 0x84) != 0x84)
                                       texpal_slot &= ~0x3;
                               break;
                           }
                       }
                       else
                           texpal_slot &= ~0x3;
                   break;
                   case 6://G
                       if((data & 0x80)){
                           switch(data & 7){
                               case 4:
                                   texpal_bg = (u16 *)(video_mem + 0x94000);
                                   texpal_slot |= 0xC;
                               break;
                               case 5:
                                   texpal_obj = (u16 *)(video_mem + 0x94000);
                               break;
                           }
                       }
                       else
                           texpal_slot &= ~0xC;
                   break;
               }
           break;
           case LCD_SUB:
               switch(bank){
                   case 8://H
                       if((data & 0x80)){
                           switch(data & 7){
                               case 2:
                                   texpal_bg = (u16 *)(video_mem + 0x98000);// + (((data >> 3) & 1) << 13));
                                   texpal_slot = 0xF;
                               break;
                           }
                       }
                   break;
                   case 9://I
                       if((data & 0x80)){
                           switch(data & 7){
                               case 3:
                                   texpal_obj = (u16 *)(video_mem + 0xA0000);
                               break;
                           }
                       }
                   break;
               }
           break;
       }
   }
}
//---------------------------------------------------------------------------
BOOL LGPU::Save(LStream *pFile)
{
   int i;

   pFile->Write(layers,sizeof(layers));
   for(i=0;i<128;i++){
       pFile->Write(&sprite_buffer[i].pFunc,12);//7168
       pFile->Write(&sprite_buffer[i].a0,44);//7168
   }
   for(i=0;i<4;i++)
       pFile->Write(sprite_priority[i],132);
   pFile->Write(rotMatrix,sizeof(rotMatrix));
   pFile->Write(tpal_bg,sizeof(tpal_bg));
   pFile->Write(tpal_obj,sizeof(tpal_obj));
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL LGPU::Load(LStream *pFile,int ver)
{
   int i;

   Reset();
   if(ver < 37){
       for(i=0;i<5;i++)
           pFile->Read(&layers[i],116);
   }
   else
       pFile->Read(layers,sizeof(layers));
   for(i=0;i<128;i++){
		pFile->Read(&sprite_buffer[i].pFunc,12);//7168
		pFile->Read(&sprite_buffer[i].a0,44);//7168
   }
   if(ver < 47){
       char c[20];

       pFile->Read(c,16);//528
   }
   else{
       for(i=0;i<4;i++)
           pFile->Read(sprite_priority[i],132);//528
   }
   pFile->Read(rotMatrix,sizeof(rotMatrix));//320
	pFile->Read(tpal_bg,sizeof(tpal_bg));
   pFile->Read(tpal_obj,sizeof(tpal_obj));

   for(i=0;i<5;i++)
   	layers[i].parent = this;

   for(i=0x240;i<0x24a;i++)
       set_VRAM((u8)(i-0x240),(u8)io_mem[i]);
   DrawMode = -1;
   for(i=0;i<100;i+=4)
       Command((u16)((type == LCD_SUB ? 0x400 : 0)+i),0,AMM_WORD);
   reset_Sprite();
   for(i=0;i<1024;i+=2)
       WriteSprite((u16)((type == LCD_SUB ? 0x400 : 0)+i),AMM_HWORD);
   for(i=0;i<0x400;i+=2)
       WritePalette((u16)((type == LCD_SUB ? 0x400 : 0)+i),AMM_HWORD);
//   ds.get_ActiveVideoPlugIn()->Reset();
//  	write_hword(0x4000304,read_hword(0x4000304));
   return TRUE;
}
//---------------------------------------------------------------------------
LRESULT LGPU::OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   PAINTSTRUCT ps;

   switch(uMsg){
       case WM_DROPFILES:
           return OnDropFiles((HDROP)wParam);
       case WM_PAINT:
           BeginPaint(&ps);
           if(reg != NULL)
           	BitBlt(ps.hdc);
           EndPaint(&ps);
           return 0;
       case WM_LBUTTONDOWN:
      		OnLMouseDown(wParam,lParam);
       break;
       case WM_LBUTTONUP:
      		OnLMouseUp(wParam,lParam);
       break;
       case WM_MOUSEMOVE:
          	OnMouseMove(wParam,lParam);
       break;
		case WM_ACTIVATE:
       	if(pPlugInContainer != NULL)
				pPlugInContainer->NotifyState(-1,PNM_ONACTIVATEWINDOW,PIS_NOTIFYMASK,wParam);
       default:
           return LWnd::OnWindowProc(uMsg,wParam,lParam);
   }
   return 0;
}
//---------------------------------------------------------------------------
void LGPU::OnLMouseDown(WPARAM wParam,LPARAM lParam)
{
   if(!(type == LCD_MAIN && (nRotate == ID_WINDOW_ROTATE0 || nRotate == ID_WINDOW_ROTATE270)) &&
       !(type == LCD_SUB && (nRotate == ID_WINDOW_ROTATE90 || nRotate == ID_WINDOW_ROTATE180)))
       return;
  	Touch_down();
	OnMouseMove(wParam,lParam);
}
//---------------------------------------------------------------------------
void LGPU::OnLMouseUp(WPARAM wParam,LPARAM lParam)
{
   if(!(type == LCD_MAIN && (nRotate == ID_WINDOW_ROTATE0 || nRotate == ID_WINDOW_ROTATE270)) &&
       !(type == LCD_SUB && (nRotate == ID_WINDOW_ROTATE90 || nRotate == ID_WINDOW_ROTATE180)))
       return;
   Touch_up();
   ds.ReleaseCapture();
}
//---------------------------------------------------------------------------
void LGPU::OnMouseMove(WPARAM wParam,LPARAM lParam)
{
	POINT pt={(s16)LOWORD(lParam),(s16)HIWORD(lParam)};
   LONG tmp;

   if(!(type == LCD_MAIN && (nRotate == ID_WINDOW_ROTATE0 || nRotate == ID_WINDOW_ROTATE270)) &&
       !(type == LCD_SUB && (nRotate == ID_WINDOW_ROTATE90 || nRotate == ID_WINDOW_ROTATE180)) || !(wParam & MK_LBUTTON))
       return;
   pt.x /= fScale;
   pt.y /= fScale;
   switch(nRotate){
       case ID_WINDOW_ROTATE90:
           tmp = pt.y;
           pt.y = 192 - (pt.x - 32);
           pt.x = tmp - 32;
		break;
       case ID_WINDOW_ROTATE270:
           tmp = pt.y;
           pt.y = pt.x - 32;
           pt.x = 256 - tmp;
       break;
   }
	if(pt.x < 0)
   	pt.x = 0;
   else if(pt.x > 255)
   	pt.x = 255;
   if(pt.y < 0)
   	pt.y = 0;
   else if(pt.y > 191)
   	pt.y = 191;
   Touch_set(pt.x,pt.y,TRUE);
	ds.SetCapture(m_hWnd);
}
//---------------------------------------------------------------------------
LRESULT LGPU::OnDropFiles(HDROP h)
{
   LString s1;

   if(DragQueryFile(h,(UINT)-1,NULL,NULL) != (UINT)-1){
       s1.Capacity(MAX_PATH+1);
       DragQueryFile(h,0,s1.c_str(),MAX_PATH);
       if(ds.OpenRom(s1.c_str())){
           SetForegroundWindow(ds.Handle());
           ::PostMessage(ds.Handle(),WM_COMMAND,MAKEWPARAM(ID_FILE_START,0),0);
       }
   }
   DragFinish(h);
   return 0;
}
//---------------------------------------------------------------------------
int LGPU::UpdateLayers(int mask)
{
	int i,res;

   res = 0;
   for(i=0;i<5;i++)
   	res |= ((layers[i].Visible & 1) << i);
   for(i=0;i<2;i++)
       res |= ((win[i].Visible & 1) << (i+5));
   res |= ((type == LCD_MAIN ? IID_DOWNLCD : IID_UPLCD) << 16);
   for(i=0;i<5;i++)
   	layers[i].Visible = (u8)((mask >> i) & 1);
   for(i=0;i<2;i++)
   	win[i].Visible = (u8)((mask >> (i+5)) & 1);
	FillListBackground();
   return res;
}
//---------------------------------------------------------------------------
static void uplcdIOFunc(u32 address,u32 data,u8 am)
{
   upLcd.Command((u16)address,data,am);
}
//---------------------------------------------------------------------------
static void downlcdIOFunc(u32 address,u32 data,u8 am)
{
   downLcd.Command((u16)address,data,am);
}
//---------------------------------------------------------------------------
static u32 r_MOSAIC(u32 address,u8 am)
{
	return 0;
}
//---------------------------------------------------------------------------
Render2DPlugIn::Render2DPlugIn() : VideoPlug()
{
   GUID g = {0xB885368B,0x565C,0x4A92,0x8D,0xC6,0xD2,0x7F,0x3C,0x13,0xBF,0x7C};

   bQuit = FALSE;
   nDraw_Req = -1;
   memcpy(&guid,&g,sizeof(GUID));
   name = "Default 2D PlugIn";
   dwFlags = dwType = PIT_VIDEO;
   hEvents[0] = hEvents[1] = NULL;
   hThread = NULL;
   nCores = 0;
   if(ds.get_NumberOfCores((unsigned long *)&nCores) != S_OK)
       nCores = 1;
   nCores = 2;
}
//---------------------------------------------------------------------------
Render2DPlugIn::~Render2DPlugIn()
{
}
//---------------------------------------------------------------------------
u32 Render2DPlugIn::Run(LPRUNPARAM p)
{
   p->index = index;
#ifdef __WIN32__
   if(nCores < GPU_MIN_CORES){
#endif
       if(p->graphics.render){
           upLcd.set_RenderLine();
           upLcd.RenderLine();
           downLcd.set_RenderLine();
           downLcd.RenderLine();
       }
       else{
           upLcd.SkipLine();
           downLcd.SkipLine();
       }
       return 1;
#ifdef __WIN32__
   }
   if(nDraw_Req > 0)
       WaitForSingleObject(hEvents[0],INFINITE);
   if(p->graphics.render){
       if(ds.get_StopCycles()){
           nDraw_Req = 0;
           upLcd.set_RenderLine();
           upLcd.RenderLine();
           downLcd.set_RenderLine();
           downLcd.RenderLine();
       }
       else{
           nDraw_Req = p->graphics.render;
           downLcd.set_RenderLine();
           SetEvent(hEvents[1]);
           upLcd.set_RenderLine();
           upLcd.RenderLine();
       }
   }
   else{
       nDraw_Req = 0;
       upLcd.SkipLine();
       downLcd.SkipLine();
   }
   return 1;
#endif
}
//---------------------------------------------------------------------------
BOOL Render2DPlugIn::EnableMultiCores(BOOL bFlag)
{
#ifdef __WIN32__
   int i;
   DWORD dw;
   LString s;
   IPlugInManager *p;
   IPlugInInterface *p1;

   if(bFlag){
       if(nCores < GPU_MIN_CORES)
           return FALSE;
       if(hThread != NULL)
           return TRUE;
       if(ds.QueryInterface(IID_IPLUGINMANAGER,(LPVOID *)&p) != S_OK)
           return FALSE;
       if(p->get_PlugInInterface(&guid,(LPVOID *)&p1) != S_OK)
           return FALSE;
       {
           char *io_mem;

           if(p1->get_Object(OID_IO_MEMORY9,(LPVOID *)&io_mem) != S_OK)
               return FALSE;
           video_status = (unsigned short *)&io_mem[4];
       }
       for(i=0;i<2;i++){
           s = "iDeaS_Event_GPU_";
           s += i;
           hEvents[i] = CreateEvent(NULL,FALSE,FALSE,s.c_str());
           if(hEvents[i] == NULL){
               nCores = 1;
               break;
           }
       }
       if(nCores < GPU_MIN_CORES)
           return FALSE;
       bQuit = FALSE;
       hThread = CreateThread(NULL,0,ThreadFunc_01,(LPVOID)this,0,&dw);
       if(hThread == NULL)
           nCores = 1;
   }
   else{
       int i;

       bQuit = TRUE;
       if(hEvents[1] != NULL)
           SetEvent(hEvents[1]);
       if(hThread != NULL){
           WaitForSingleObject(hThread,INFINITE);
           CloseHandle(hThread);
           hThread = NULL;
       }
       for(i=0;i<2;i++){
           if(hEvents[i] != NULL){
               CloseHandle(hEvents[i]);
               hEvents[i] = NULL;
           }
       }
   }
   return TRUE;
#else
    return FALSE;
#endif
}
//---------------------------------------------------------------------------
BOOL Render2DPlugIn::Enable(BOOL bFlag)
{
	int i;
   IPlugInManager *p;
   IPlugInInterface *p1;

	if(!VideoPlug::Enable(bFlag))
   	return FALSE;
   if(!bFlag)
   	return TRUE;
   if(ds.QueryInterface(IID_IPLUGINMANAGER,(LPVOID *)&p) != S_OK)
   	return FALSE;
   if(p->get_PlugInInterface(&guid,(LPVOID *)&p1) != S_OK)
   	return FALSE;
   for(i=0;i<0x70;i++){
   	p1->WriteTable(0x04000000+i,(LPVOID)downlcdIOFunc,NULL);
       p1->WriteTable(0x04001000+i,(LPVOID)uplcdIOFunc,NULL);
   }
   for(i=0x4C;i<0x4E;i++){
        p1->WriteTable(0x04000000+i,(LPVOID)downlcdIOFunc,(LPVOID)r_MOSAIC);
        p1->WriteTable(0x04001000+i,(LPVOID)uplcdIOFunc,(LPVOID)r_MOSAIC);
	}
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL Render2DPlugIn::Reset()
{
	upLcd.Reset();
   downLcd.Reset();
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL Render2DPlugIn::Destroy()
{
   EnableMultiCores(FALSE);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL Render2DPlugIn::SaveState(LStream *p)
{
   upLcd.Save(p);
   downLcd.Save(p);
	return TRUE;
}
//---------------------------------------------------------------------------
BOOL Render2DPlugIn::LoadState(LStream *p,int ver)
{
	if(ver >= 38){
       upLcd.Load(p,ver);
   	downLcd.Load(p,ver);
	}
	return TRUE;
}
//---------------------------------------------------------------------------
DWORD Render2DPlugIn::SetInfo(LPSETPLUGININFO p)
{
	DWORD dwState;
   DWORD dwStateMask;
   LPARAM lParam;

   if(p == NULL)
   	return FALSE;
   lParam = p->lParam;
   dwStateMask = p->dwStateMask;
   dwState = p->dwState;
   if(dwStateMask & PIS_NOTIFYMASK){
       switch(dwState & PIS_NOTIFYMASK){
       	case PNM_INITTABLE:
           	Enable(TRUE);
           break;
           case PNM_STARTFRAME:
#ifdef __WIN32s
               if(nCores >= GPU_MIN_CORES){
                   if(nDraw_Req > 0){
                       WaitForSingleObject(hEvents[0],INFINITE);
                       ResetEvent(hEvents[1]);
                       nDraw_Req = 0;
                   }
               }
#endif
               upLcd.ResetFrame();
               downLcd.ResetFrame();
           break;
           case PNMV_CHANGEVRAMCNT:
       		upLcd.set_VRAM((u8)LOWORD(lParam),(u8)HIWORD(lParam));
       		downLcd.set_VRAM((u8)LOWORD(lParam),(u8)HIWORD(lParam));
           break;
           case PNMV_CHANGEVCOUNT:
           	downLcd.toutBuffer = ((u16 *)downLcd.get_OutBuffer() + lParam * 256);
           	upLcd.toutBuffer = ((u16 *)upLcd.get_OutBuffer() + lParam * 256);
           break;
           case PNMV_SHOWLAYERS:
           	lParam = *((LPARAM *)p->lParam);
           	if(((lParam >> 16) & IID_UPLCD))
           		*((LPARAM *)p->lParam) = upLcd.UpdateLayers(lParam);
           	if(((lParam >> 16) & IID_DOWNLCD))
           		*((LPARAM *)p->lParam) = downLcd.UpdateLayers(lParam);
           break;
           case PNM_POWCNT1:
           	lParam = lParam;
           break;
           case PNMV_OAMCHANGED:
          		upLcd.WriteSprite((u16)(lParam >> 16),(u8)lParam);
           	downLcd.WriteSprite((u16)(lParam >> 16),(u8)lParam);
           break;
           case PNMV_PALETTECHANGED:
				upLcd.WritePalette((u16)(lParam >> 16),(u8)lParam);
				downLcd.WritePalette((u16)(lParam >> 16),(u8)lParam);
           break;
           case PNM_CHANGECONFIG:
               if(lParam == 1){
                   p->lpNDS->get_NumberOfCores((LPDWORD)&nCores);
                   EnableMultiCores(nCores > 1);
               }
           break;
       }
   }
	return TRUE;
}
//---------------------------------------------------------------------------
DWORD WINAPI Render2DPlugIn::ThreadFunc_01(LPVOID arg)
{
   return ((Render2DPlugIn *)arg)->OnThread_01();
}
//---------------------------------------------------------------------------
DWORD Render2DPlugIn::OnThread_01()
{
#ifdef __WIN32__
   SetThreadIdealProcessor(GetCurrentThread(),((1 << nCores) -1));
   while(!bQuit){
       WaitForSingleObject(hEvents[1],INFINITE);
       if(bQuit)
           break;
       downLcd.RenderLine();
       SetEvent(hEvents[0]);
   }
#endif
   return 0;
}

