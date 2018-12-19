#include "modeTile.h"
#include "lds.h"

static u32 crx[2];
//---------------------------------------------------------------------------
void I_FASTCALL PostDrawLineMode1(LPLAYER layer)
{
   layer->CurrentX += layer->rotMatrix[1];
   layer->CurrentY += layer->rotMatrix[3];
}
//---------------------------------------------------------------------------
void I_FASTCALL PostDrawMode1(LPLAYER layer)
{
   layer->CurrentScrollX += layer->rotMatrix[0];
   layer->CurrentScrollY += layer->rotMatrix[2];
}
//---------------------------------------------------------------------------
void I_FASTCALL InitLayerMode1(LPLAYER layer)
{
/*   if(!layer->bPalette && (layer->parent->reg[3] & 0x40)){
   	layer->pal = layer->parent->texpal_bg;
		switch(layer->Index){
           case 0:
               if(layer->bWrap && layer->parent->texpal_slot)
               	layer->pal += 0x2000;
           break;
           case 1:
               if(!layer->bWrap || !layer->parent->texpal_slot)
           	    layer->pal += 0x1000;
               else
                   layer->pal += 0x3000;
           break;
       	case 2:
               if(layer->parent->texpal_slot)
      			    layer->pal += 0x2000;
           break;
           case 3:
               if(layer->parent->texpal_slot)
           	    layer->pal += 0x3000;
           break;
       }
   }
   else*/
		layer->pal = layer->parent->tpal_bg;
   layer->CurrentScrollX = layer->CurrentX;
   layer->CurrentScrollY = layer->CurrentY;
}
//---------------------------------------------------------------------------
u32 I_FASTCALL drawPixelMode1(LPLAYER layer)
{
   int xTile,yTile,y2,y3;
   u8 width;

	if(layer->pal == NULL || layer->bDrawEnable < 3)
   	return (u32)-1;
   width = (u8)(layer->Width - 1);
   xTile = (y2 = layer->CurrentScrollX >> 8) >> 3;
   yTile = (y3 = layer->CurrentScrollY >> 8) >> 3;
   if(layer->bWrap || (xTile >= 0 && xTile < layer->Width && yTile >= 0 && yTile < layer->Height)){
       if((width = layer->CharBaseBlock[((y3 & 7) << 3) + (y2 & 7) + (layer->ScreenBaseBlock[((yTile & width) << layer->log2) | (xTile & width)] << 6)]) == 0)
       	return (u32)-1;
       return layer->pal[width];
   }
   else
       return (u32)-1;
}
//---------------------------------------------------------------------------
void I_FASTCALL InitLayerMode0(LPLAYER layer)
{
   u32 y1;

   if(layer->bMosaic && layer->yMosaic)
       y1 = (u32)((layer->parent->render_line - (layer->parent->render_line % layer->yMosaic)) + layer->bg[1]);
   else
       y1 = (u32)(layer->parent->render_line + layer->bg[1]);
  	layer->tileY = (u16)((y1 >> 3) & layer->Height);
   if(layer->Width != 31 && layer->tileY > 31){
       layer->offsetToAdd1 = (u16)0x800;
       layer->tileY &= 0x1F;
   }
   else
   	layer->offsetToAdd1 = 0;
//   if(layer->ScreenBaseBlock == NULL)
//       layer->ScreenBaseBlock = NULL;
   layer->scrbb = layer->ScreenBaseBlock + (layer->tileY << 6);
   layer->vFlip = (u8)((y1 & 0x7) << 3);
   if(!layer->bPalette && (layer->parent->reg[3] & 0x40) && ds.get_EmuMode() == 0){
       layer->bExtPal = 1;
   	layer->pal = layer->parent->texpal_bg;
       if(layer->pal == NULL)
           layer->pal = layer->parent->tpal_bg;
		switch(layer->Index){
           case 0:
               if(layer->bWrap && layer->parent->texpal_slot == 0xF)
               	layer->pal += 0x2000;
           break;
           case 1:
           	layer->pal += 0x1000;
               if(layer->parent->texpal_slot == 0xF && layer->bWrap)
					layer->pal += 0x2000;
           break;
       	case 2:
				if(layer->parent->texpal_slot == 0xF)
           	    layer->pal += 0x2000;
           break;
           case 3:
				if(layer->parent->texpal_slot == 0xF)
           	    layer->pal += 0x3000;
               else if(layer->parent->texpal_slot & 8)
               	layer->pal += 0x1000;
           break;
       }
   }
   else{
		layer->pal = layer->parent->tpal_bg;
       layer->bExtPal = 0;
   }
}
//---------------------------------------------------------------------------
u32 I_FASTCALL drawPixelMode0(LPLAYER layer)
{
   u32 tileX,x1;
   u16 tileData;

   if(layer->bDrawEnable < 3)
       return (u32)-1;
   x1 = crx[layer->parent->type];
   if(layer->bMosaic && layer->xMosaic)
       x1 -= (x1 % layer->xMosaic);
   x1 += layer->bg[0];
   tileX = (x1 >> 3) & layer->Width;
   if(tileX > 31 && layer->Width != 31)
       tileX = (tileX & 0x1F) + 0x400;
   tileX += layer->offsetToAdd1;
   if(tileX  > layer->maxScreenBase)
       return (u32)-1;
   tileX = ((tileData = *((u16 *)layer->scrbb + tileX)) & 0x3FF) << 6;
   if(tileData & 0x400){
       tileX += 7 - (x1 & 7);
       x1 = ~x1;
   }
   else
       tileX += (x1 & 7);
   tileX += (tileData & 0x800) ? 56 - layer->vFlip : layer->vFlip;
   if(!layer->bPalette){
       if(tileX > layer->maxCharBase || (tileX = (u32)layer->CharBaseBlock[tileX]) == 0)
           return (u32)-1;
       if(layer->bExtPal){
           tileData = layer->pal[tileX + ((tileData & 0xF000) >> 4)];
           return BGR(tileData);
       }
       return layer->pal[tileX];
   }
   if((tileX >>= 1) > layer->maxCharBase)
       return (u32)-1;
   if((tileX = ((layer->CharBaseBlock[tileX] >> ((x1 & 0x1) << 2)) & 0xF)) == 0)
       return (u32)-1;
   return layer->pal[tileX + ((tileData & 0xF000) >> 8)];
}
//---------------------------------------------------------------------------
void LGPU::drawLineModeTile()
{
   u8 prtSrc,idxSrc,i1;
   u16 zValueSrc,zValueTar;
   LPLAYER pLayer;
   u32 *p,*dw,i5w,*pcrx;

   i5w = 0;
   for(i1=0;i1<CountSource;i1++){
       Source[i1]->initDrawLine(Source[i1]);
       if(Source[i1]->postDraw != NULL)
           i5w = 1;
   }
   for(i1=0;i1<CountTarget;i1++){
       Target[i1]->initDrawLine(Target[i1]);
       if(Target[i1]->postDraw != NULL)
           i5w = 1;
   }
   p = (u32 *)SourceBuffer;
   pcrx = &crx[type];
   if(i5w)
       goto drawLineModeTile_0;
   switch(iBlend){
       case 0:
           for(*pcrx = 0;*pcrx < rcDisplay.right;(*pcrx)++){
               zValueSrc = (u16)NULLZPIXEL;
               for(i5w = -1,i1=0;i1<CountSource;i1++){
                   pLayer = Source[i1];
                   if((i5w = pLayer->drawPixel(pLayer)) != -1){
                       zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                       break;
                   }
               }
               *p++ = (u32)i5w;
               *pCurrentZBuffer++ = (u32)zValueSrc;
           }
       break;
       case 1:
           dw = (u32 *)TargetBuffer;
           for(*pcrx = 0;*pcrx<rcDisplay.right;(*pcrx)++,p++,dw++){
               *dw = *p = (u32)-1;
               zValueSrc = (u16)NULLZPIXEL;
               zValueTar = (u16)NULLZPIXEL;
               for(i5w=-1,i1 = 0;i1<CountSource;i1++){
                   pLayer = Source[i1];
                   if((i5w = pLayer->drawPixel(pLayer)) != -1){
                       zValueSrc = (u16)((prtSrc = pLayer->Priority)|((idxSrc = pLayer->Index) << 8));
                       *p = (u32)i5w;
                       break;
                   }
               }
               for(i5w = -1,i1=0;i1 < CountTarget;i1++){
                   pLayer = Target[i1];
                   if((i5w = pLayer->drawPixel(pLayer)) != -1){
                       if(*p != (u32)-1){
                           if(pLayer->Priority > prtSrc && pLayer->Tipo != 'T')
                               i5w |= 0x80000000;
                           else if(pLayer->Priority < prtSrc || (pLayer->Priority == prtSrc && idxSrc > pLayer->Index)){
                               *p |= 0x80000000;
                               zValueSrc = (u16)NULLZPIXEL;
                           }
                       }
                       else if((CountSource < 1 || CountTarget > 1) && (i5w & 0x40000000)){
                           zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                           *p = (u32)i5w;
                           i5w = -1;
                           continue;
                       }
                       *dw =(u32)((i5w & 0x80000000) | (u16)i5w);
                       zValueTar = (u16)(pLayer->Priority|(pLayer->Index << 8));
                       break;
                   }
               }
               *pCurrentZBuffer++ = (u32)(zValueSrc | (zValueTar << 16));
           }
       break;
       case 2:
       case 3:
           for(*pcrx=0;*pcrx<rcDisplay.right;(*pcrx)++){
               zValueSrc = NULLZPIXEL;
               for(i5w=-1,i1=0;i1<CountSource;i1++){
                  	pLayer = Source[i1];
               	if((i5w = pLayer->drawPixel(pLayer)) != -1){
                       zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                       if(pLayer->Tipo != 'S')
                           i5w |= 0x80000000;
                       break;
                   }
               }
               if(!i5w && !Source[6])
                   i5w = 0x80000000;
               *p++ = (u32)((i5w & 0x80000000) | (u16)i5w);
               *pCurrentZBuffer++ = (u32)zValueSrc;
           }
       break;
   }
   return;
drawLineModeTile_0:
   switch(iBlend){
       case 0:
           for(*pcrx = 0;*pcrx < rcDisplay.right;(*pcrx)++){
               zValueSrc = (u16)NULLZPIXEL;
               for(i5w = -1,i1=0;i1<CountSource;i1++){
                   if(i5w == -1 && (i5w = Source[i1]->drawPixel(Source[i1])) != -1)
                       zValueSrc = (u16)(Source[i1]->Priority|(Source[i1]->Index << 8));
                   if(Source[i1]->postDraw != NULL)
                       Source[i1]->postDraw(Source[i1]);
               }
               *p++ = (u32)i5w;
               *pCurrentZBuffer++ = (u32)zValueSrc;
           }
       break;
       case 1:
           dw = (u32 *)TargetBuffer;
           for(*pcrx=0;*pcrx<rcDisplay.right;(*pcrx)++,p++,dw++){
               *dw = *p = (u32)-1;
               zValueSrc = (u16)NULLZPIXEL;
               for(i5w=-1,i1 = 0;i1<CountSource;i1++){
                   pLayer = Source[i1];
                   if(i5w == -1 && (i5w = pLayer->drawPixel(pLayer)) != -1){
                       zValueSrc = (u16)((prtSrc = pLayer->Priority)|((idxSrc = pLayer->Index) << 8));
                       *p = (u32)i5w;
                   }
                   if(pLayer->postDraw != NULL)
                       pLayer->postDraw(pLayer);
               }
               zValueTar = (u16)NULLZPIXEL;
               for(i5w = -1,i1=0;i1 < CountTarget;i1++){
                   pLayer = Target[i1];
                   if(i5w == -1 && (i5w = pLayer->drawPixel(pLayer)) != -1){
                       if(*p != (u32)-1){
                           if(pLayer->Priority > prtSrc && pLayer->Tipo != 'T')
                               i5w |= 0x80000000;
                           else if(pLayer->Priority < prtSrc || (pLayer->Priority == prtSrc && idxSrc > pLayer->Index)){
                               *p |= 0x80000000;
                               zValueSrc = (u16)NULLZPIXEL;
                           }
                       }
                       else if((CountSource < 1 || CountTarget > 1) && (i5w & 0x40000000)){
                           zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                           *p = (u32)i5w;
                           i5w = -1;
                           if(pLayer->postDraw != NULL)
                               pLayer->postDraw(pLayer);
                           continue;
                       }
                       *dw =(u32)((i5w & 0x80000000) | (u16)i5w);
                       zValueTar = (u16)(pLayer->Priority|(pLayer->Index << 8));
                   }
                   if(pLayer->postDraw != NULL)
                       pLayer->postDraw(pLayer);
               }
               *pCurrentZBuffer++ = (u32)(zValueSrc | (zValueTar << 16));
           }
       break;
       case 2:
       case 3:
           for(*pcrx=0;*pcrx<rcDisplay.right;(*pcrx)++){
               zValueSrc = NULLZPIXEL;
               for(i5w=-1,i1=0;i1<CountSource;){
                  	pLayer = Source[i1++];
               	if(i5w == -1 && (i5w = pLayer->drawPixel(pLayer)) != -1){
                       zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                       if(pLayer->Tipo != 'S')
                           i5w |= 0x80000000;
                   }
                   if(pLayer->postDraw != NULL)
                       pLayer->postDraw(pLayer);
               }
               if(!i5w && !Source[6])
                   i5w = 0x80000000;
               *p++ = (u32)((i5w & 0x80000000) | (u16)i5w);
               *pCurrentZBuffer++ = (u32)zValueSrc;
           }
       break;
   }
   for(i1=0;i1<CountSource;i1++){
       if(Source[i1]->postDrawLine != NULL)
           Source[i1]->postDrawLine(Source[i1]);
   }
   for(i1=0;i1<CountTarget;i1++){
       if(Target[i1]->postDrawLine != NULL)
           Target[i1]->postDrawLine(Target[i1]);
   }
}
//---------------------------------------------------------------------------
void LGPU::drawLineModeTileWindow()
{
   LPWINCONTROL winc;
   LPLAYER S[6],T[6],*p1,pLayer,E[6];
   u8 nCS,nCT,CountPoint,iBlendCopy,nCE;
   u8 n,i1,i2,i5,prtSrc,idxSrc;
   u16 zValueSrc,zValueTar;
   u32 i5w,*dw,*p,*pcrx;
   int i,xStart;
   POINTWINDOW ptLine[5];
   GETSPRITEPIXEL oldGetPixelSprite;

   CountPoint = (u8)GetPointsWindow(ptLine);
   p1 = Source;
   for(n=0;n<CountSource;n++,p1++)
       ((LPLAYER)(*p1))->initDrawLine(*p1);
   p1 = Target;
   for(n=0;n<CountTarget;n++,p1++)
       ((LPLAYER)(*p1))->initDrawLine(*p1);
   oldGetPixelSprite = GetPixelSprite;
   iBlendCopy = iBlend;
   pcrx = &crx[type];
   for(*pcrx = i2 = 0;i2<CountPoint;i2++){
       winc = ptLine[i2].winc;
       i = ptLine[i2].width;
       nCS = nCT = nCE = 0;
       xStart = *pcrx;
       if(winc->EnableBlend && iBlendCopy == 1){
           iBlend = 1;
           p1 = Source;
           for(n=0;n<CountSource;n++,p1++){
               if(winc->EnableBg[((LPLAYER)(*p1))->Index])
                   S[nCS++] = *p1;
               else if(((LPLAYER)(*p1))->postDraw != NULL)
               	E[nCE++] = *p1;
           }
           p1 = Target;
           for(n=0;n<CountTarget;n++,p1++){
               if(winc->EnableBg[((LPLAYER)(*p1))->Index])
                   T[nCT++] = *p1;
               else if(((LPLAYER)(*p1))->postDraw != NULL)
               	E[nCE++] = *p1;
           }
       }
       else{
           if(winc->EnableBlend || winObj.Enable)
               iBlend = iBlendCopy;
           else
               iBlend = 0;
           p1 = Source;
           for(n=0;n<CountSource;n++,p1++){
               if(winc->EnableBg[((LPLAYER)(*p1))->Index])
                   S[nCS++] = *p1;
               else if(((LPLAYER)(*p1))->postDraw != NULL)
               	E[nCE++] = *p1;
           }
           p1 = Target;
           for(n=0;n<CountTarget;n++,p1++){
               if(winc->EnableBg[((LPLAYER)(*p1))->Index])
                   S[nCS++] = *p1;
               else if(((LPLAYER)(*p1))->postDraw != NULL)
               	E[nCE++] = *p1;
           }
           for(n=0;n<nCS;n++){
               pLayer = S[n];
               for(i5 = (u8)(n + 1);i5 < nCS;i5++){
                   if(pLayer->Priority > S[i5]->Priority ||
                       (pLayer->Priority == S[i5]->Priority && pLayer->Index > S[i5]->Index)){
                       S[n] = S[i5];
                       S[i5] = pLayer;
                       pLayer = S[n];
                   }
               }
           }
       }
       p = (u32 *)SourceBuffer + *pcrx;
       switch(iBlend){
           case 0:
               GetPixelSprite = &LGPU::GetPXSprite;
               for(;i > 0;i--,(*pcrx)++){
                   zValueSrc = (u16)NULLZPIXEL;
                   for(p1 = S,i5w=-1,i1=nCS;i1 > 0;i1--){
                       pLayer = *p1++;
                       if(i5w == -1 && (i5w = pLayer->drawPixel(pLayer)) != -1)
                           zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                       if(pLayer->postDraw != NULL)
                           pLayer->postDraw(pLayer);
                   }
                   *p++ = i5w;
                   *pCurrentZBuffer++ = zValueSrc;
                   for(i1=0;i1 <nCE;i1++)
                       E[i1]->postDraw(E[i1]);
               }
           break;
           case 1:
               GetPixelSprite = &LGPU::GetPXSpriteAlpha;
               dw = (u32 *)TargetBuffer + *pcrx;
               for(;i>0;i--,dw++,p++,(*pcrx)++){
                   *dw = *p = (u32)-1;
                   zValueSrc = (u16)NULLZPIXEL;
                   for(p1 = S,i5w=-1,i1=nCS;i1 > 0;i1--){
                       pLayer = *p1++;
                       if(i5w == -1 && (i5w = pLayer->drawPixel(pLayer)) != -1){
                           zValueSrc = (u16)((prtSrc = pLayer->Priority)|((idxSrc = pLayer->Index) << 8));
                           *p = i5w;
                       }
                       if(pLayer->postDraw != NULL)
                           pLayer->postDraw(pLayer);
                   }
                   zValueTar = (u16)NULLZPIXEL;
                   for(p1 = T,i5w = -1,i1=nCT;i1 > 0;i1--){
                       pLayer = *p1++;
                       if(i5w == -1 && (i5w = pLayer->drawPixel(pLayer)) != -1){
                           if(*p != (u32)-1){
                               if(pLayer->Priority > prtSrc && pLayer->Tipo != 'T')
                                   i5w |= 0x80000000;
                               else if(pLayer->Priority < prtSrc || (pLayer->Priority == prtSrc && idxSrc > pLayer->Index)){
                                   *p |= 0x80000000;
                                   zValueSrc = (u16)NULLZPIXEL;
                               }
                           }
                           *dw = (u32)((i5w & 0x80000000) | (u16)i5w);
                           zValueTar = (u16)(pLayer->Priority|(pLayer->Index << 8));
                       }
                       if(pLayer->postDraw != NULL)
                           pLayer->postDraw(pLayer);
                   }
                   *pCurrentZBuffer++ = (u32)(zValueSrc | (zValueTar << 16));
                   for(i1=0;i1 <nCE;i1++)
                       E[i1]->postDraw(E[i1]);
               }
           break;
           case 2:
           case 3:
               GetPixelSprite = &LGPU::GetPXSpriteBrightness;
               for(;i > 0;i--,(*pcrx)++){
                   zValueSrc = (u16)NULLZPIXEL;
                   for(p1 = S,i5w=-1,i1=nCS;i1 > 0;i1--){
                       pLayer = *p1++;
                       if(i5w == -1 && (i5w = pLayer->drawPixel(pLayer)) != -1){
                           zValueSrc = (u16)(pLayer->Priority|(pLayer->Index << 8));
                           if(pLayer->Tipo != 'S')
                               i5w |= 0x80000000;
                       }
                       if(pLayer->postDraw != NULL)
                           pLayer->postDraw(pLayer);
                   }
                   if(!i5w && !Source[6])
                       i5w = 0x80000000;
                   *p++ = (u32)((i5w & 0x80000000) | (u16)i5w);
                   *pCurrentZBuffer++ = (u32)zValueSrc;
                   for(i1=0;i1 <nCE;i1++)
                       E[i1]->postDraw(E[i1]);
               }
           break;
       }
       if(winc->EnableObj || winObj.Enable){
			ubDrawSprite = 1;
           draw_Sprite((u16)xStart,*pcrx);
       }
       else
           (this->*swapBuffer[iBlend])((u16)xStart,*pcrx);
   }
   p1 = Source;
   for(i1=0;i1<CountSource;i1++,p1++){
       if(((LPLAYER)(*p1))->postDrawLine != NULL)
           ((LPLAYER)(*p1))->postDrawLine(*p1);
   }
   p1 = Target;
   for(i1=0;i1<CountTarget;i1++,p1++){
       if(((LPLAYER)(*p1))->postDrawLine != NULL)
           ((LPLAYER)(*p1))->postDrawLine(*p1);
   }
   GetPixelSprite = oldGetPixelSprite;
   iBlend = iBlendCopy;
}
//---------------------------------------------------------------------------
void I_FASTCALL InitLayerMode3D(LPLAYER layer)
{
   layer->tileY = layer->parent->render_line;
}
//---------------------------------------------------------------------------
u32 I_FASTCALL drawPixelMode3D(LPLAYER layer)
{
	if(ds.get_ActiveVideoPlugIn() == NULL)
		return (u32)-1;
	return ds.get_ActiveVideoPlugIn()->Run(crx[0],layer->tileY);
}
//---------------------------------------------------------------------------
void I_FASTCALL InitLayerMode4(LPLAYER layer)
{
	layer->ScreenBaseBlock = video_mem;
   if(layer->parent->reg[0] & 0x10)
       layer->ScreenBaseBlock += 0xA000;
	layer->pal = layer->parent->tpal_bg;
   layer->CurrentScrollX = layer->CurrentX;
   layer->CurrentScrollY = layer->CurrentY;
}
//---------------------------------------------------------------------------
u32 I_FASTCALL drawPixelMode4(LPLAYER layer)
{
   s32 x,y;

	x = layer->CurrentScrollX >> 8;
   y = layer->CurrentScrollY >> 8;
   if(!layer->bWrap && (x < 0 || y < 0 || x > 239 || y > 159))
   	return (u32)-1;
	return layer->pal[layer->ScreenBaseBlock[x + y * 240]];
}






