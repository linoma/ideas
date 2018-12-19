#include "ideastypes.h"
#include "dstype.h"
#include "lwnd.h"
#include "dsmem.h"
#include "lcanvas.h"

//---------------------------------------------------------------------------
#ifndef gpuH
#define gpuH
#define BGR(a)(u16)(((a & 0x1F) << 10) | (a & 0x3e0) | ((a >> 10) & 0x1F))

#define MAKEZPIXEL(prtSrc,indexSrc,ptrTar,indexTar) (u32)((u8)prtSrc | ((u8)indexSrc << 8) | ((u8)ptrTar << 16) | ((u8)indexTar << 24))
#define MAKEZPIXEL16(ptrSrc,indexSrc) (u16)(prtSrc | (indexSrc << 8))
#define NULLZPIXEL (u32)0xFFFFFFFF
#define NULLPIZEL NULLZPIXEL
#define GETZPRTSRC(value) (u8)value
#define GETZPRTTAR(value) (u8)(value >> 16)
#define GETZIDXSRC(value) (u8)(value >> 8)
#define GETZIDXTAR(value) (u8)(value >> 24)
//---------------------------------------------------------------------------
#define DISPCNT	*((u16 *)reg)
#define DISPCNT1	*((u16 *)reg + 1)
#define VCOUNT		*((u16 *)io_mem + 0x3)
#define VCOUNT7	*((u16 *)io_mem7 + 0x3)
#define DISPSTAT   *((u16 *)io_mem + 0x2)
#define DISPSTAT7  *((u16 *)io_mem7 + 0x2)
#define BG0CNT		*((u16 *)reg + 0x4)
#define BG1CNT		*((u16 *)reg + 0x5)
#define BG2CNT		*((u16 *)reg + 0x6)
#define BG3CNT		*((u16 *)reg + 0x7)

#define WININ      *((u16 *)reg + 0x24)
#define WINOUT   	*((u16 *)reg + 0x25)

#define MOSAIC     *((u16 *)(reg + 0x4c))
#define BLENDCNT 	*((u16 *)(reg + 0x50))
#define BLENDV     *((u16 *)(reg + 0x52))
#define BLENDY     *((u16 *)(reg + 0x54))
#define BRIGHTCNT  *((u16 *)(reg + 0x6C))
//---------------------------------------------------------------------------
struct _layer;
struct _sprite;
class LGPU;

typedef u32 I_FASTCALL (*DRAWPIXEL)(struct _layer *);
typedef void I_FASTCALL (*INITDRAWLINE)(struct _layer *);
typedef void I_FASTCALL (*POSTDRAWPIXEL)(struct _layer *);
typedef void I_FASTCALL (*POSTDRAWLINE)(struct _layer *);
typedef void (*RENDERMODE)();

typedef u32 (LGPU::*GETSPRITEPIXEL)(struct _sprite *,u16,u16);
typedef void (LGPU::*SWAPBUFFER)(u16,u16);
//---------------------------------------------------------------------------
typedef struct _layer
{
   DRAWPIXEL drawPixel;
   INITDRAWLINE initDrawLine;
   POSTDRAWPIXEL postDraw;
   POSTDRAWLINE postDrawLine;
   u8 Enable,Priority,Visible;
   u8 Index;
   u8 Tipo;
   u8 bExtPal;
   s16 rotMatrix[4];
   s32 bg[2];
   s32 bgrot[2];
   u8 bMosaic;
   u8 bPalette;
   u8 *ScreenBaseBlock,*CharBaseBlock,*scrbb;
   u16 Width,Height;
   u8 log2;
   u16 tileY,offsetToAdd1;
   u8 vFlip,bWrap;
   u8 yMosaic,xMosaic;
   u16 Control;
   s32 CurrentX,CurrentY,CurrentScrollX,CurrentScrollY;
   LGPU *parent;
   u32 CharBase,ScreenBase;
   u16 *pal;
   u8 bDrawEnable;
   s32 maxCharBase,maxScreenBase;
} LAYER,*PLAYER,FAR *LPLAYER;
//---------------------------------------------------------------------------
typedef struct {
   LPLAYER EnableBg[4];
   u8 EnableObj;
   u8 EnableBlend;
} WINCONTROL,*PWINCONTROL,*LPWINCONTROL;
//---------------------------------------------------------------------------
typedef struct {
   LPLAYER EnableBg[4];
   u8 EnableObj;
   u8 EnableBlend;
   u8 Enable;
} WINCONTROLEX,*PWINCONTROLEX,*LPWINCONTROLEX;
//---------------------------------------------------------------------------
typedef struct _tagPOINTWINDOW{
   u16 width;
   LPWINCONTROL winc;
} POINTWINDOW,*LPPOINTWINDOW;
//---------------------------------------------------------------------------
typedef struct{
   u8 Enable,Visible;
   u8 left,top;
   u8 right,bottom;
   WINCONTROL Control;
} WINGBA,*PWINGBA,*LPWINGBA;
//---------------------------------------------------------------------------
typedef struct
{
   u8 Enable,bMosaic,bRot,xMosaic;
   u16 x;
   u32 iLayer;
   s16 rotMatrix[4];
   s32 bg[2];
   u8 *pBuffer;
} ROTMATRIX,*PROTMATRIX,*LPROTMATRIX;
//---------------------------------------------------------------------------
typedef void (*OAMFUNC)(u16);
typedef void (LGPU::* OAMDRAWPIXEL)(struct _sprite *,u16 xStart,u16 xEnd);
//---------------------------------------------------------------------------
typedef struct _rotMatrix
{
   s16 PA,PB,PC,PD;
   u8 bWrite;
} SROTMATRIX,*PSROTMATRIX;
//---------------------------------------------------------------------------
typedef struct _sprite
{
   OAMDRAWPIXEL pFunc;
   u16 a0,a1,a2;
   u8 Enable;
   u8 Priority,Index;
   u8 hFlip,vFlip;
   u8 bDouble,bMosaic;
   s16 xPos,yPos;
   u8 SizeX,SizeY;
   u8 bPalette;
   u16 iPalette;
   u8 bRot;
   u8 VisibleMode;
   u8 value;
   PSROTMATRIX rotMatrix;
   u32 tileBaseNumber;
   u32 tileNumberYIncr;
   LGPU *parent;
} SPRITE,*PSPRITE,*LPSPRITE;
//---------------------------------------------------------------------------
enum LCDTYPE {LCD_MAIN=0,LCD_SUB};
//---------------------------------------------------------------------------
class LGPU : public LCanvas,public LWnd,public LBase
{
public:
	LGPU();
   virtual ~LGPU();
   virtual BOOL Init();
   void RenderLine();
   virtual void Reset();
   void Command(u16 address,u32 data,u8 accessMode);
   virtual void set_PowerMode() PURE;
	void WriteSprite(u16 adress,u8 mode);
   void WritePalette(u16 adress,u8 mode);
   void set_VRAM(u8 bank,u8 data);
   virtual BOOL Load(LStream *pFile,int ver);
   virtual BOOL Save(LStream *pFile);
   void ResetFrame();
   inline void SkipLine(){toutBuffer += 256;};
   virtual LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   LRESULT OnDropFiles(HDROP h);
   void set_Rect(RECT &rc);
   void get_Rect(RECT &rc);
   void set_Scale(float value);
   inline float get_Scale(){return fScale;};
//protected:
	virtual BOOL CreateWnd() PURE;
   void OnLMouseDown(WPARAM wParam,LPARAM lParam);
   void OnLMouseUp(WPARAM wParam,LPARAM lParam);
   void OnMouseMove(WPARAM wParam,LPARAM lParam);
   BOOL Rotate(WORD wID);
   int BitBlt(HDC hdc = NULL);
	void drawLineModeTile();
   void drawLineModeTileWindow();
   void GetBackgroundRect(u8 num);
   void FillListBackground();
   void FillBaseLayer(LPLAYER layer);
   void FillBaseLayer_Internal(LPLAYER layer);
   void rgbAlphaLayerLine(u16 xStart,u16 xEnd);
   void rgbNormalLine(u16 xStart,u16 xEnd);
   void rgbFadeLineDown(u16 xStart,u16 xEnd);
   void rgbFadeLineUp(u16 xStart,u16 xEnd);
   void rgbAlphaLayerLineNoOAM(u16 xStart,u16 xEnd);
   void rgbNormalLineNoOAM(u16 xStart,u16 xEnd);
   void rgbFadeLineDownNoOAM(u16 xStart,u16 xEnd);
   void rgbFadeLineUpNoOAM(u16 xStart,u16 xEnd);

	void GetAlphaIndex();
   void GetBrightnessIndex();
   inline LPSPRITE get_Sprite(int index){return &sprite_buffer[index];};
//---------------------------------------------------------------------------
   void reset_Sprite();
   void draw_Sprite(u16 xStart,u16 xEnd);
	u32 GetPXSprite(LPSPRITE pSprite,u16 xPos,u16 iColor);
	u32 GetPXSpriteBrightness(LPSPRITE pSprite,u16 xPos,u16 iColor);
	u32 GetPXSpriteAlpha(LPSPRITE pSprite,u16 xPos,u16 iColor);
	void drawPixelSprite(LPSPRITE pSprite,u16 xStart,u16 xEnd);
   void drawPixelSpriteBMP(LPSPRITE pSprite,u16 xStart,u16 xEnd);
   void drawPixelSpritePalette(LPSPRITE pSprite,u16 xStart,u16 xEnd);
	void drawPixelSpriteRot(LPSPRITE pSprite,u16 xStart,u16 xEnd);
	void drawPixelSpriteRotBMP(LPSPRITE pSprite,u16 xStart,u16 xEnd);
	void drawPixelSpriteRotPalette(LPSPRITE pSprite,u16 xStart,u16 xEnd);
	void SetrotMatrix(u16 *p,LPSPRITE pSprite);
	void CalcAttr0(LPSPRITE pSprite,u16 *p);
	void RemapAllSprite();
	void CalcSpriteYIncr(LPSPRITE pSprite);
	void CalcAttr1(LPSPRITE pSprite,u16 *p);
	void SpriteSort(u8 *tab,int left,int right);
	void CalcAttr2(LPSPRITE pSprite,u16 *p,u8 Index);
	void CalcRotMatrix(u32 adress);
   inline LCDTYPE get_Type(){return type;};
   inline u16 *get_MainMemoryAddress(){return pMainMemory;};
   inline BOOL is_MainMemoryUsed(){return (BOOL)dwMainMemoryIndex != 0;};
   void SwapMainMemory();
   inline u8 get_MemoryBank(u8 adr){
	    if(adr < 8)
   	    return 0;
       if(adr < 16)
   	    return 1;
       if(adr < 24)
   	    return 2;
       if(adr < 32)
   	    return 3;
       if(adr < 36)
   	    return 4;
       if(adr < 37)
   	    return 5;
       if(adr < 38)
   	    return 6;
       if(adr < 40)
   	    return 8;
       return 9;
   }
   int GetPointsWindow(LPPOINTWINDOW pt);
   int UpdateLayers(int mask);
   u32 get_LayerAddress(LPLAYER layer,BOOL scr = TRUE);
   u8 get_LayerBank(u32 adr,u8 *ret = NULL,u32 *size = NULL);
   inline void set_RenderLine(){render_line = VCOUNT;};     
//---------------------------------------------------------------------------
   u8 *pal_bg,*pal_obj,*obj,*reg,*vram_obj;
   u16 *tpal_bg,*tpal_obj,*toutBuffer,*texpal_bg,*texpal_obj,*pMainMemory,render_line;
	s8 texpal_slot;
   u8 *tabColor,*tabColor1;
   u8 *SourceBuffer,*TargetBuffer,*SpriteBuffer,*WinObjSprite,*ZBuffer;
   u32 *pCurOB,*pCurSB,dwMainMemoryIndex;
   LCDTYPE type;

  	LPLAYER *Source,*Target;
   LAYER *layers;
   WINGBA win[2];
   WINCONTROL winOut;
   WINCONTROLEX winObj;
   SWAPBUFFER swapBuffer[8];
   GETSPRITEPIXEL GetPixelSprite;

   u8 xMosaic,yMosaic,sprxMosaic,spryMosaic,ubDrawSprite;
   u8 eva,evb,evy,iBlend;
   u8 CountSource,CountTarget;
   u8 DrawMode,WinEnable,SpriteEnable;
   u32 *pCurrentZBuffer;
   u16 *pFilterBuffer,*pRotateBuffer,nRotate;
   u8 *peva,*pevb,*pevy,*pevy1;

	SPRITE *sprite_buffer;
	u8 *sprite_priority[4];
   SROTMATRIX *rotMatrix;
   u8 bDoubleSize;
   float fScale;
};
//---------------------------------------------------------------------------
class LUpLcd : public LGPU,public IWnd
{
public:
	LUpLcd();
   ~LUpLcd();
   LRESULT OnWindowProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
   void set_PowerMode();
   void Reset();
   void UpdateToolBarState(BOOL bLoad = FALSE,BOOL bStart = FALSE,BOOL bHinge = FALSE);
   BOOL AdjustWindowRect(LPRECT lpRect);
   void set_StatusLed(int nValue);
   void OnChangeStatusLed(int OnOff);
   //IWnd Methods
   HDC DC(){return LCanvas::DC();};
   HWND Handle(){return LGPU::Handle();};
   BOOL Map();
   void Release();
   BOOL Create();
   BOOL Load(LStream *pFile,int ver);
protected:
	void OnMenuSelect(WORD wID);
   BOOL CreateWnd();
   HWND m_hWndTB;
   HIMAGELIST imageListDebug[2],imLed;
   BOOL bLockWindows;
   int nStateLed;
   UINT uiTimerID;
};
//---------------------------------------------------------------------------
class LDownLcd : public LGPU,public IWnd
{
public:
	LDownLcd();
   ~LDownLcd();
   void Reset();
   void set_PowerMode();
	BOOL OnMenuSelect(WORD wID);
   //IWnd Methods
   HDC DC(){return LCanvas::DC();};
   HWND Handle(){return LGPU::Handle();};
   BOOL Map();
   BOOL Reposition();
   void Release();
   BOOL Create();
   BOOL Load(LStream *pFile,int ver);
protected:
   BOOL CreateWnd();
};
//---------------------------------------------------------------------------
extern LDownLcd downLcd;
extern LUpLcd upLcd;

#endif
