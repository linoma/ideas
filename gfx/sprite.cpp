#include "util.h"
#include "gpu.h"

//---------------------------------------------------------------------------
static u8 sprite_sizes_x[0x10] ={8,16,32,64,16,32,32,64, 8, 8,16,32,0,0,0,0};
static u8 sprite_sizes_y[0x10] ={8,16,32,64, 8, 8,16,32,16,32,32,64,0,0,0,0};
//---------------------------------------------------------------------------
void LGPU::reset_Sprite()
{
   u8 i;

   for(i=0;i<128;i++){
       ZeroMemory(&sprite_buffer[i],sizeof(SPRITE));
       sprite_buffer[i].Index = sprite_buffer[i].Priority = 0xFF;
       sprite_buffer[i].parent = this;
   }
   for(i=0;i<4;i++){
       FillMemory(sprite_priority[i],130,0xFF);
       sprite_priority[i][129] = 0;
   }
   ZeroMemory(rotMatrix,32*sizeof(SROTMATRIX));
}
//---------------------------------------------------------------------------
void LGPU::draw_Sprite(u16 xStart,u16 xEnd)
{
   u8 *b;
   LPSPRITE pSprite;
   s8 i;

   if(!SpriteEnable || !layers[4].Visible || !layers[4].bDrawEnable)
       goto ex_DrawSprite;
   i = 3;
   do{
       b = sprite_priority[i--];
       while(*b != 0xFF){
           pSprite = &sprite_buffer[*b++];
           if(pSprite->Enable == 0)
               continue;
           (this->*pSprite->pFunc)(pSprite,xStart,xEnd);
       }
   }
   while(i > -1);
ex_DrawSprite:
   //   if(winObj.Enable)
   //       DrawLineObjWindow(xStart,xEnd);
   //   else
   (this->*swapBuffer[(ubDrawSprite * 4) + iBlend])(xStart,xEnd);
}
//---------------------------------------------------------------------------
u32 LGPU::GetPXSprite(LPSPRITE pSprite,u16 xPos,u16 iColor)
{
   if(pSprite->Priority > (u8)((u32 *)ZBuffer)[xPos])
       return (u32)-1;
   switch(pSprite->VisibleMode){
       case 0:
       case 3:
       	ubDrawSprite = 0;
           return (u32)iColor;
       case 1:
           ubDrawSprite = 0;
           if(Target[7] != 0)
               return (u32)(0x81000000 | iColor);
           return iColor;
        case 2:
        	ubDrawSprite = 0;
           WinObjSprite[xPos] = 1;
           return ((u32 *)SpriteBuffer)[xPos];
        default:
           return (u32)-1;
   }
}
//---------------------------------------------------------------------------
u32 LGPU::GetPXSpriteBrightness(LPSPRITE pSprite,u16 xPos,u16 iColor)
{
   if(pSprite->Priority > (u8)((u32 *)ZBuffer)[xPos])
       return (u32)-1;
   switch(pSprite->VisibleMode){
       case 0:
       case 3:
       	ubDrawSprite = 0;
           if(Source[5] == NULL)
               return (u32)(0x80000000 | iColor);
           return (u32)iColor;
       case 1:
           ubDrawSprite = 0;
           if(Target[7] != 0)
               return (u32)(0x81000000 | iColor);
           return (u32)iColor;
       case 2:
       	ubDrawSprite = 0;
           WinObjSprite[xPos] = 1;
           return ((u32 *)SpriteBuffer)[xPos];
       default:
           return (u32)-1;
   }
}
//---------------------------------------------------------------------------
u32 LGPU::GetPXSpriteAlpha(LPSPRITE pSprite,u16 xPos,u16 iColor)
{
   u8 prtSrc,idxSrc,prtTar,idxTar;
   u32 value;

   switch(pSprite->VisibleMode){
       case 0:
       case 3:
           prtSrc = GETZPRTSRC((value = ((u32 *)ZBuffer)[xPos]));
           prtTar = GETZPRTTAR(value);
           idxTar = GETZIDXTAR(value);
        //           idxSrc = GETZIDXSRC(value);
           if(Source[5] != NULL){
               if(prtSrc != 0xFF){
                   if(pSprite->Priority <= prtSrc){
                   	ubDrawSprite = 0;
                       return (u32)(0x80000000 | iColor);
                   }
                   return (u32)-1;
               }
               else if(idxTar != 0xFF){
                   if(layers[idxTar].Tipo == 'T'){
                   	ubDrawSprite = 0;
                       if(pSprite->Priority > prtTar)
                           return (u32)(0x80000000 | iColor);
                       return (u32)iColor;
                   }
                   else{
                       if(pSprite->Priority <= prtTar){
                       	ubDrawSprite = 0;
                           return (u32)iColor;
                       }
                       return (u32)-1;
                   }
               }
           }
           else if(Target[5] != NULL){
               if(prtSrc != 0xFF){
                   if(prtTar != 0xFF){
                       if(pSprite->Priority < prtTar){
                       	ubDrawSprite = 0;
                           if(pSprite->Priority <= prtSrc)
                               return (u32)iColor;
                           return (u32)(0x80000000 | iColor);
                       }
                       else if(pSprite->Priority == prtTar){
                       	ubDrawSprite = 0;
                           if(prtTar > prtSrc)
                               return (u32)(0x80000000 | iColor);
                           return (u32)iColor;
                       }
                       else{
                           if(layers[idxTar].Tipo == 'T'){
                           	ubDrawSprite = 0;
                               return (u32)(0x84000000 | iColor);
                           }
                           return (u32)-1;
                       }
                   }
                   else{
                   	ubDrawSprite = 0;
                   	if(pSprite->Priority <= prtSrc)
                       	return (u32)iColor;
                   	return (u32)(0x04000000 | iColor);
                   }
               }
               else if(prtTar != 0xFF){
               	ubDrawSprite = 0;
                   if(pSprite->Priority > prtTar)
                       return (u32)(0x04000000 | iColor);
                   else if(pSprite->Priority == prtTar){
                       if(layers[idxTar].Tipo == 'T')
                           return (u32)(0x80000000 | iColor);
                       return (u32)iColor;
                   }
                   return (u32)iColor;
               }
           }
           else{
               if(prtSrc < pSprite->Priority){
               	ubDrawSprite = 0;
                   return (u32)(0x80000000|iColor);
               }
               if(prtTar < pSprite->Priority)
                   return (u32)-1;
           }
       break;
       case 1:
           if(pSprite->Priority > (prtTar = GETZPRTTAR((value = ((u32 *)ZBuffer)[xPos]))))
               return (u32)-1;
           idxSrc = GETZIDXSRC(value);
           if(idxSrc != 0xFF && layers[idxSrc].Tipo == ' '){
               if(Target[7] == 0){
               	ubDrawSprite = 0;
                   return (u32)iColor;
               }
               return (u32)-1;
           }
			ubDrawSprite = 0;
           if(idxSrc == 0xFF){
               if(prtTar == 0xFF)
                   return (u32)iColor;
               if(layers[GETZIDXTAR(value)].Tipo != 'T')
                   return (u32)iColor;
           }
           return (u32)(iColor | 0x81000000);
       case 2:
           ubDrawSprite = 0;
           WinObjSprite[xPos] = 1;
           return ((u32 *)SpriteBuffer)[xPos];
   }
	ubDrawSprite = 0;
   return (u32)iColor;
}
//---------------------------------------------------------------------------
void LGPU::drawPixelSprite(LPSPRITE pSprite,u16 xStart,u16 xEnd)
{
   s16 xPos,sx,sy;
   u8 *p1,b;
   u16 x8,y8,sxEnd,x,*pal;
   u32 *p;

   if(render_line < (xPos = pSprite->yPos) || render_line >= xPos + (sy = pSprite->SizeY))
       return;
   y8 = (u16)(render_line - xPos);
   if((xPos = pSprite->xPos) > xEnd || (xPos + (sxEnd = sx = pSprite->SizeX)) < xStart)
       return;
   p = (u32 *)SpriteBuffer + xPos;
   if(pSprite->bMosaic != 0 && spryMosaic)
       y8 = (u16)(y8 - (y8 % spryMosaic));
   y8 = (u16)(pSprite->vFlip ? sy - 1 - y8 : y8);
   p1 = &vram_obj[(y8 & 0x7) << 3];
   if(ds.get_EmuMode() != EMUM_GBA){
       pal = (b = (u8)(reg[3] & 0x80)) != 0 ? &texpal_obj[pSprite->iPalette<<4] : tpal_obj;
       p1 += (((pSprite->tileBaseNumber << ((reg[2] & 0x30) >> 4)) + ((y8 >> 3) << pSprite->tileNumberYIncr)) << 5);
   }
   else{
       b = FALSE;
		pal = tpal_obj;
       p1 += ((pSprite->tileBaseNumber + ((y8 >> 3) << pSprite->tileNumberYIncr)) << 5);
   }
   if(((u32)p1 - (u32)video_mem) > 0xA3FFF)
      return;
   if((sy = (s16)(xEnd - xPos)) < sx)
       sxEnd = (u16)sy;
   for(x=0;x < sxEnd;x++,p++,xPos++){
       if(xPos < xStart)
           continue;
       if(pSprite->bMosaic != 0 && sprxMosaic)
           x8 = (u16)(x - (x % sprxMosaic));
       else
           x8 = x;
       x8 = (u16)((y8 = (u16)(pSprite->hFlip ? sx - 1 - x8 : x8)) & 0x7);
       if((y8 = p1[((y8 >> 3) << 6) + x8]) == 0)
           continue;
       y8 = pal[y8];
       if(b)
           y8 = BGR(y8);
       *p = (this->*GetPixelSprite)(pSprite,xPos,y8)|(pSprite->Priority << 16);
   }
}
//---------------------------------------------------------------------------
void LGPU::drawPixelSpritePalette(LPSPRITE pSprite,u16 xStart,u16 xEnd)
{
	s16 xPos,sx,sy;
   u8 *p1;
   u16 x8,y8,sxEnd,x;
   u32 *p;

   if(render_line < (xPos = pSprite->yPos) || render_line >= xPos + (sy = pSprite->SizeY))
        return;
   y8 = (u16)(render_line - xPos);
   if((xPos = pSprite->xPos) > xEnd || (xPos + (sxEnd = sx = pSprite->SizeX)) < xStart)
       return;
   p = (u32 *)SpriteBuffer + xPos;
   if(pSprite->bMosaic != 0 && spryMosaic)
       y8 = (u16)(y8 - (y8 % spryMosaic));
   y8 = (u16)(pSprite->vFlip ? sy - 1 - y8 : y8);
   p1 = &vram_obj[(y8 & 0x7) << 2];
	if(ds.get_EmuMode() != EMUM_GBA)
   	p1 += (((pSprite->tileBaseNumber << ((reg[2] & 0x30) >> 4)) + ((y8 >> 3) << pSprite->tileNumberYIncr)) << 5);
   else
   	p1 += ((pSprite->tileBaseNumber + ((y8 >> 3) << pSprite->tileNumberYIncr)) << 5);
   if(((u32)p1 - (u32)video_mem) > 0xA3FFF)
      return;
   if((sy = (s16)(xEnd - xPos)) < sx)
       sxEnd = (u16)sy;
	for(x=0;x < sxEnd;x++,p++,xPos++){
   	if(xPos < xStart)
       	continue;
       if(pSprite->bMosaic != 0 && sprxMosaic)
           x8 = (u16)(x - (x % sprxMosaic));
       else
           x8 = x;
       x8 = (u16)((y8 = (u16)(pSprite->hFlip ? sx - 1 - x8 : x8)) & 0x7);
       y8 = p1[((y8 >> 3) << 5) + (x8 >> 1)];
       if((y8 = (u16)((y8 >> ((x8 & 0x1) << 2)) & 0xF)) == 0)
           continue;
       y8 = tpal_obj[y8 + (u16)pSprite->iPalette];
       *p = (this->*GetPixelSprite)(pSprite,xPos,y8)|(pSprite->Priority << 16);
	}
}
//---------------------------------------------------------------------------
void LGPU::drawPixelSpriteBMP(LPSPRITE pSprite,u16 xStart,u16 xEnd)
{
   s16 xPos,sx,sy;
   u8 *p1;
   u16 x8,y8,sxEnd,x;
   u32 *p;

   if(render_line < (xPos = pSprite->yPos) || render_line >= xPos + (sy = pSprite->SizeY))
       return;
   y8 = (u16)(render_line - xPos);
   if((xPos = pSprite->xPos) > xEnd || (xPos + (sxEnd = sx = pSprite->SizeX)) < xStart)
       return;
   p = (u32 *)SpriteBuffer + xPos;
   if(pSprite->bMosaic != 0 && spryMosaic)
       y8 = (u16)(y8 - (y8 % spryMosaic));
   y8 = (u16)(pSprite->vFlip ? sy - 1 - y8 : y8);
   switch(reg[0] & 0x60){
       case 0:
          p1 = &vram_obj[((pSprite->tileBaseNumber & 0xF) << 4) + ((pSprite->tileBaseNumber & ~0xF) << 7) + (y8 << 8)];
       break;
       case 0x20:
          p1 = &vram_obj[((pSprite->tileBaseNumber & 0x1F) << 4) + ((pSprite->tileBaseNumber & ~0x1F) << 7) + (y8<<9)];
       break;
       case 0x40:
       	x = (u16)(7 + ((reg[2] & 0x40) >> 6));
          	p1 = &vram_obj[(pSprite->tileBaseNumber<<x) + (y8 << pSprite->tileNumberYIncr)];
       break;
   }
   if(((u32)p1 - (u32)video_mem) > 0xA3FFF)
      return;
   if((sy = (s16)(xEnd - xPos)) < sx)
       sxEnd = (u16)sy;
   for(x=0;x < sxEnd;x++,p++,xPos++){
       if(xPos < xStart)
           continue;
       x8 = x;
       if(pSprite->bMosaic != 0 && sprxMosaic)
           x8 -= (x % sprxMosaic);
       y8 = (u16)(pSprite->hFlip ? sx - 1 - x8 : x8);
       if(!pSprite->iPalette || !((y8 = ((u16 *)p1)[y8]) & 0x8000))
           continue;
		x8 = ((y8 & 0x1F) * pSprite->iPalette >> 8) << 10;
       x8 |= (((y8 >> 5) & 0x1F) * pSprite->iPalette >> 8) << 5;
       x8 |= ((y8 >> 10) & 0x1F) * pSprite->iPalette >> 8;
       *p = (this->*GetPixelSprite)(pSprite,xPos,x8)|(pSprite->Priority << 16);
   }
}
//---------------------------------------------------------------------------
void LGPU::drawPixelSpriteRot(LPSPRITE pSprite,u16 xStart,u16 xEnd)
{
	u16 iColor,*pal;
   int xc,yc,y2,y3,x3,y,xc2,yTile,x2PA,x2PC;
   u8 sx,sy,x;
   s16 yPos,xPos;
   u32 tileBaseNumber,PA,PC,*p;
   u8 xTile,xSubTile,ySubTile,yScr,xNbTile,yNbTile,b;

   xNbTile = sx = pSprite->SizeX;
   yNbTile = sy = pSprite->SizeY;
   xc = sx >> 1;
   yc = sy >> 1;
   sx <<= pSprite->bDouble;
   sy <<= pSprite->bDouble;
   if((yScr = render_line) < (yPos = pSprite->yPos) || yScr >= yPos + sy)
       return;
   y = yScr - yPos;
   p = (u32 *)SpriteBuffer + (xPos = pSprite->xPos);
   tileBaseNumber = pSprite->tileBaseNumber;
   if(ds.get_EmuMode() != EMUM_GBA)
   	 tileBaseNumber <<= ((reg[2] & 0x30) >> 4);
   xc2 = (0 - (sx >> 1));
   y2 = (y - (sy >> 1)) << 8;
   if(pSprite->rotMatrix == NULL)
       pSprite->rotMatrix = rotMatrix;
   x2PA = xc2 * (PA = pSprite->rotMatrix->PA << 8) + (y2 * pSprite->rotMatrix->PB);
   x2PC = xc2 * (PC = pSprite->rotMatrix->PC << 8) + (y2 * pSprite->rotMatrix->PD);
   if((x3 = xEnd - xPos) < sx)
       sx = (u8)x3;
	if(ds.get_EmuMode() != EMUM_GBA){
       pal = (b = (u8)(reg[3] & 0x80)) != 0 ? &texpal_obj[pSprite->iPalette<<4] : tpal_obj;
   }
   else{
       b = 0;
       pal = tpal_obj;
   }
   for(x = 0; x < sx;x++,p++,x2PA += PA,x2PC += PC,xPos++){
       if(xPos < xStart)
           continue;
       x3 = (x2PA >> 16) + xc;
       y3 = (x2PC >> 16) + yc;
       if(!(x3 >= 0 && x3 < xNbTile && y3 >= 0 && y3 < yNbTile))
           continue;
       xTile = (u8)(x3 >> 3);
       yTile = (y3 >> 3) << pSprite->tileNumberYIncr;
       xSubTile = (u8)(x3 & 0x07);
       ySubTile = (u8)(y3 & 0x07);
       if((iColor = vram_obj[((tileBaseNumber + (xTile << 1) + yTile) << 5) + xSubTile + (ySubTile << 3)]) == 0)
           continue;
       iColor = b ? BGR(pal[iColor]) : pal[iColor];
       *p = (this->*GetPixelSprite)(pSprite,xPos,iColor)|(pSprite->Priority << 16);
   }
}
//---------------------------------------------------------------------------
void LGPU::drawPixelSpriteRotBMP(LPSPRITE pSprite,u16 xStart,u16 xEnd)
{
    u16 iColor;
    int xc,yc,y2,y3,x3,y,xc2,x2PA,x2PC;
    u8 sx,sy,x;
    s16 yPos,xPos;
    u32 tileBaseNumber,PA,PC,*p;
    u8 yScr,xNbTile,yNbTile;

    xNbTile = sx = pSprite->SizeX;
    yNbTile = sy = pSprite->SizeY;
    xc = sx >> 1;
    yc = sy >> 1;
    sx <<= pSprite->bDouble;
    sy <<= pSprite->bDouble;
    if((yScr = render_line) < (yPos = pSprite->yPos) || yScr >= yPos + sy)
        return;
    y = yScr - yPos;
    p = (u32 *)SpriteBuffer + (xPos = pSprite->xPos);
    xc2 = (0 - (sx >> 1));
    y2 = (y - (sy >> 1)) << 8;
    switch(reg[0] & 0x60){
        case 0:
           tileBaseNumber = ((pSprite->tileBaseNumber & 0xF) << 4) + ((pSprite->tileBaseNumber & ~0xF) << 7);
			sy = 7;
        break;
        case 0x20:
           tileBaseNumber = ((pSprite->tileBaseNumber & 0x1F) << 4) + ((pSprite->tileBaseNumber & ~0x1F) << 7);
			sy = 8;
        break;
        case 0x40:
           tileBaseNumber = pSprite->tileBaseNumber << ((reg[2] & 0x40) ? 8 : 7);
           sy = (u8)pSprite->tileNumberYIncr;
        break;
    }
    if(pSprite->rotMatrix == NULL)
        pSprite->rotMatrix = rotMatrix;
    x2PA = xc2 * (PA = pSprite->rotMatrix->PA << 8) + (y2 * pSprite->rotMatrix->PB);
    x2PC = xc2 * (PC = pSprite->rotMatrix->PC << 8) + (y2 * pSprite->rotMatrix->PD);
    if((x3 = xEnd - xPos) < sx)
        sx = (u8)x3;
    for(x = 0; x < sx;x++,p++,x2PA += PA,x2PC += PC,xPos++){
        if(xPos < xStart)
            continue;
        x3 = (x2PA >> 16) + xc;
        y3 = (x2PC >> 16) + yc;
        if(!(x3 >= 0 && x3 < xNbTile && y3 >= 0 && y3 < yNbTile))
            continue;
        iColor = *((u16 *)&vram_obj[tileBaseNumber + (x3 + (y3<<sy) << 1)]);
        if(!(iColor & 0x8000))
            continue;
        iColor = BGR(iColor);
        *p = (this->*GetPixelSprite)(pSprite,xPos,iColor)|(pSprite->Priority << 16);
    }
}
//---------------------------------------------------------------------------
void LGPU::drawPixelSpriteRotPalette(LPSPRITE pSprite,u16 xStart,u16 xEnd)
{
    u16 iColor,*pal;
    int xc,yc,y2,y3,x3,y,xc2,yTile,x2PA,x2PC;
    u8 sx,sy,x;
    s16 yPos,xPos;
    u32 tileBaseNumber,PA,PC,*p;
    u8 xTile,xSubTile,ySubTile,yScr,xNbTile,yNbTile;

    xNbTile = sx = pSprite->SizeX;
    yNbTile = sy = pSprite->SizeY;
    xc = sx >> 1;
    yc = sy >> 1;
    sx <<= pSprite->bDouble;
    sy <<= pSprite->bDouble;
    if((yScr = render_line) < (yPos = pSprite->yPos) || yScr >= yPos + sy)
        return;
    y = yScr - yPos;
    p = (u32 *)SpriteBuffer + (xPos = pSprite->xPos);
    tileBaseNumber = pSprite->tileBaseNumber;
    if(ds.get_EmuMode() != EMUM_GBA)
    	tileBaseNumber <<=  ((reg[2] & 0x30) >> 4);
    xc2 = (0 - (sx >> 1));
    y2 = (y - (sy >> 1)) << 8;
    if(pSprite->rotMatrix == NULL)
        pSprite->rotMatrix = rotMatrix;
    x2PA = xc2 * (PA = pSprite->rotMatrix->PA << 8) + (y2 * pSprite->rotMatrix->PB);
    x2PC = xc2 * (PC = pSprite->rotMatrix->PC << 8) + (y2 * pSprite->rotMatrix->PD);
    if((x3 = xEnd - xPos) < sx)
        sx = (u8)x3;
    pal = tpal_obj;
    for(x = 0; x < sx;x++,p++,x2PA += PA,x2PC += PC,xPos++){
        if(xPos < xStart)
            continue;
        x3 = (x2PA >> 16) + xc;
        y3 = (x2PC >> 16) + yc;
        if(!(x3 >= 0 && x3 < xNbTile && y3 >= 0 && y3 < yNbTile))
            continue;
        xTile = (u8)(x3 >> 3);
        yTile = (y3 >> 3) << pSprite->tileNumberYIncr;
        xSubTile = (u8)(x3 & 0x07);
        ySubTile = (u8)(y3 & 0x07);
        iColor = vram_obj[((tileBaseNumber + xTile + yTile) << 5) + (xSubTile >> 1) + (ySubTile << 2)];
        if((iColor = (u16)((iColor >> ((xSubTile & 0x1) << 2)) & 0xf)) == 0)
            continue;
        iColor = pal[iColor + pSprite->iPalette];
        *p = (this->*GetPixelSprite)(pSprite,xPos,iColor)|(pSprite->Priority << 16);
    }
}
//---------------------------------------------------------------------------
void LGPU::SetrotMatrix(u16 *p,LPSPRITE pSprite)
{
    u16 i;

    if((pSprite->bRot = (u8)((*p >> 8) & 1)) == 0)
        return;
    i = (u16)((*(p+1) >> 9) & 0x1f);
    if(rotMatrix[i].bWrite != pSprite->Index){
        rotMatrix[i].bWrite = pSprite->Index;
        pSprite->rotMatrix = &rotMatrix[i];
    }
}
//---------------------------------------------------------------------------
void LGPU::CalcAttr0(LPSPRITE pSprite,u16 *p)
{
    u16 i;

    SetrotMatrix(p,pSprite);
    pSprite->bDouble = (u8)((*p >> 9) & 1);
    pSprite->yPos = (u8)*p;
    if(pSprite->yPos > 200)
        pSprite->yPos = (s16)(pSprite->yPos - 255);
    pSprite->bPalette = (u8)(!((*p >> 13) & 1));
    pSprite->VisibleMode = (u8)((*p >> 10) & 3);
    if(pSprite->bRot == 0){
        if(pSprite->VisibleMode == 3)
            pSprite->pFunc = &LGPU::drawPixelSpriteBMP;
        else if(pSprite->bPalette)
            pSprite->pFunc = &LGPU::drawPixelSpritePalette;
        else
            pSprite->pFunc = &LGPU::drawPixelSprite;
        pSprite->bMosaic = 0;
    }
    else{
        if(pSprite->VisibleMode == 3)
            pSprite->pFunc = &LGPU::drawPixelSpriteRotBMP;
        else if(pSprite->bPalette)
            pSprite->pFunc = &LGPU::drawPixelSpriteRotPalette;
        else
            pSprite->pFunc = &LGPU::drawPixelSpriteRot;
        pSprite->bMosaic = (u8)((*p >> 12) & 1);
    }
    pSprite->a0 = *p;
    i = (u16)((*p++ >> 14) << 2);
    i += (u16)(*p >> 14);
    pSprite->SizeX = sprite_sizes_x[i];
    pSprite->SizeY = sprite_sizes_y[i];
    CalcSpriteYIncr(pSprite);
}
//---------------------------------------------------------------------------
void LGPU::RemapAllSprite()
{
	u8 i;

   for(i=0;i<128;i++){
   	if(sprite_buffer[i].Priority != 0xFF)
       	CalcSpriteYIncr(&sprite_buffer[i]);
	}
}
//---------------------------------------------------------------------------
void LGPU::CalcSpriteYIncr(LPSPRITE pSprite)
{
   u16 i;

   if(pSprite->VisibleMode == 3)
       i = (u16)(pSprite->SizeX << 1);
   else
       i = (u16)(bDoubleSize != 0 ? (!pSprite->bPalette ? pSprite->SizeX >> 2 : pSprite->SizeX >> 3) : 32);
   pSprite->tileNumberYIncr = i > 1 ? log2(i) : 0;
}
//---------------------------------------------------------------------------
void LGPU::CalcAttr1(LPSPRITE pSprite,u16 *p)
{
    s16 s;
    u16 i;

    SetrotMatrix(p,pSprite);
    i = (u16)((*p++ >> 14) << 2);
    i += (u16)(*p >> 14);
    pSprite->SizeX = sprite_sizes_x[i];
    pSprite->SizeY = sprite_sizes_y[i];
    if((s = (s16)(*p & 0x1FF)) > 255)
        s = (s16)(0 - (512 - s));
    pSprite->xPos = s;
    pSprite->a1 = *p;
    pSprite->hFlip = (u8)((*p >> 12) & 1);
    pSprite->vFlip = (u8)((*p >> 13) & 1);
    CalcSpriteYIncr(pSprite);
}
//---------------------------------------------------------------------------
void LGPU::SpriteSort(u8 *tab,int left,int right)
{
	u8 *p,*p1,*p2,value,value1;
   int i;

   if(left >= right)
       return;
   value = *(p = p2 = &tab[left]);
   *p = *(p1 = &tab[(left + right) >> 1]);
   *p1 = value;
   for(p1 = p,value = *p++,i = left + 1;i<=right;i++,p++){
   	if(*p <= value)
           continue;
       value1 = *(++p1);
       *p1 = *p;
       *p = value1;
   }
   value = *p2;
   *p2 = *p1;
   *p1 = value;
   SpriteSort(tab,left,(i = p1 - tab) - 1);
   SpriteSort(tab,i + 1,right);
}
//---------------------------------------------------------------------------
void LGPU::CalcAttr2(LPSPRITE pSprite,u16 *p,u8 Index)
{
   u8 i1,i2,*p1,*p2,i,i3,i4,*p4;

   p += 2;
   i4 = pSprite->Priority;
   if((i3 = pSprite->Priority = (u8)((*p >> 10) & 3)) != i4){
   	p4 = sprite_priority[i3];
       p4[i2 = p4[129]] = Index;
       if(pSprite->Index != 0xFF){
       	p1 = p2 = sprite_priority[i4];
           p1[pSprite->Index] = 0xFF;
           p1[129]--;
           for(i1=0,i=0;i<128;i++,p1++){
           	if(*p1 != 0xFF)
                   sprite_buffer[*p2++ = *p1].Index = i1++;
           }
           for(;i1<128;i1++)
               *p2++ = 0xFF;
       }
       pSprite->Index = i2;
       SpriteSort(p4,0,i1 = p4[129]++);
       for(i=0;i<=i1;i++)
           sprite_buffer[*p4++].Index = i;
   }
   pSprite->iPalette = (u16)((*p >> 12) << 4);
   pSprite->tileBaseNumber = (*p & 0x3ff);
   if(ds.get_EmuMode() != EMUM_GBA)
   	i1 = (u8)(DISPCNT & 0x10);
   else
   	i1 = (u8)(DISPCNT & 0x40);
   if(!pSprite->bPalette && !i1)
       pSprite->tileBaseNumber &= 0xfffe;
   pSprite->a2 = *p;
}
//---------------------------------------------------------------------------
void LGPU::CalcRotMatrix(u32 adress)
{
   u8 Index;
   u16 i;
   PSROTMATRIX p;

   Index = (u8)(adress >> 5);
   i = (u16)(Index << 4);
   p = &rotMatrix[Index];
   switch(((adress - (Index << 5)) >> 3)){
       case 0:
           p->PA = ((u16 *)obj)[i + 3];
       break;
       case 1:
           p->PB = ((u16 *)obj)[i + 7];
       break;
       case 2:
           p->PC = ((u16 *)obj)[i + 11];
       break;
       case 3:
           p->PD = ((u16 *)obj)[i + 15];
       break;
   }
   p->bWrite = 0xFF;
}
//---------------------------------------------------------------------------
void LGPU::WriteSprite(u16 adress,u8 mode)
{
   u8 Index;
   LPSPRITE pSprite;
   u16 *p,ofsMem;
   int x,y;

   adress &= 0x7FF;
   ofsMem = (u16)(type == LCD_SUB ? 0x400 : 0);
   if(adress < ofsMem || adress >= (ofsMem + 0x400))
       return;
   adress -= ofsMem;
   pSprite = &sprite_buffer[(Index = (u8)(adress >> 3))];
   p = &((u16 *)obj)[Index << 2];                              //8003f0c
   switch((adress & 7)){
       case 0:
       case 1:
           CalcAttr0(pSprite,p);
           if(mode == AMM_WORD)
               CalcAttr1(pSprite,p);
       break;
       case 2:
       case 3:
           CalcAttr1(pSprite,p);
           if(mode == AMM_WORD)
               CalcAttr2(pSprite,p,Index);
       break;
       case 4:
       case 5:
           CalcAttr2(pSprite,p,Index);
           if(mode == AMM_WORD)
               CalcRotMatrix(adress+2);
       break;
       case 6:
       case 7:
           CalcRotMatrix(adress);
       break;
   }
   if((pSprite->Enable = (u8)((!pSprite->a0 && !pSprite->a1) ? 0 : 1))){
   	x = pSprite->SizeX + pSprite->xPos;
   	y = pSprite->SizeY + pSprite->yPos;
   	if(!pSprite->bRot && pSprite->bDouble || y <= 0 || pSprite->yPos > 191 || x < 0 || pSprite->xPos > 255)
       	pSprite->Enable = 0;
   }
}

