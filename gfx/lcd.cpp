#include "gpu.h"
#include "mode4.h"
#include "modetile.h"
#include "util.h"

//---------------------------------------------------------------------------
void LGPU::FillListBackground()
{
   u8 n,n1,i1,i;
   LPLAYER layer;

   n1 = n = 0;
   switch(iBlend){
       case 0:
       case 2:
       case 3:
           for(i=0;i<4;i++){
               layer = layers;
               for(i1=0;i1<4;i1++,layer++){
                   if(layer->Enable != 0 && layer->Priority == i && layer->Visible && layer->drawPixel != NULL){
                       layer->Tipo = (u8)((BLENDCNT & (1 << i1)) ? 'S' : 32);
                       Source[n++] = layer;
                   }
               }
           }
       break;
       case 1:
           for(i=0;i<4;i++){
               layer = layers;
               for(i1=0;i1<4;i1++,layer++){
                   if(layer->Enable != 0 && layer->Priority == i && layer->Visible && layer->drawPixel != NULL){
                       if((BLENDCNT & (1 << i1))){
                           layer->Tipo = 'S';
                           Source[n++] = layer;
                       }
                       else{
                           Target[n1++] = layer;
                           layer->Tipo = (BLENDCNT & (1 << (i1 + 8))) ? 'T' : ' ';
                       }
                   }
               }
           }
       break;
   }
   CountSource = n;
   CountTarget = n1;
   for(;n<5;n++)
       Source[n] = (LPLAYER)0xFFFFFFFF;
   for(;n1<5;n1++)
       Target[n1] = (LPLAYER)0xFFFFFFFF;
}
//---------------------------------------------------------------------------
void LGPU::GetBackgroundRect(u8 num)
{
   LPLAYER layer;
   u16 *p;
   
   layer = &layers[num];
   p = (u16 *)reg + 4 + num;
   if(layer->drawPixel == drawPixelMode0){
       switch((*p>>14) & 0x3){
           case 0:
               layer->Width = 31;
               layer->Height = 31;
           break;
           case 1:
               layer->Width = 63;
               layer->Height = 31;
           break;
           case 2:
               layer->Width = 31;
               layer->Height = 63;
           break;
           case 3:
               layer->Width = 63;
               layer->Height = 63;
           break;
       }
   }
   else if(layer->drawPixel == drawPixelMode1){
       switch ((*p>>14)&0x3) {
           case 0:
               layer->Width = 16;
               layer->Height = 16;
               layer->log2 = 4;
           break;
           case 1:
               layer->Width = 32;
               layer->Height = 32;
               layer->log2 = 5;
           break;
           case 2:
               layer->Width = 64;
               layer->Height = 64;
               layer->log2 = 6;
           break;
           case 3:
               layer->Width = 128;
               layer->Height = 128;
               layer->log2 = 7;
           break;
       }
   }
   else if(layer->drawPixel == drawPixelMode4Rot){
       switch ((*p>>14)&0x3) {
           case 0:
               layer->Width = 16;
               layer->Height = 16;
               layer->log2 = 4;
           break;
           case 1:
               layer->Width = 32;
               layer->Height = 32;
               layer->log2 = 5;
           break;
           case 2:
               layer->Width = 64;
               layer->Height = (u16)((layer->Control & 2) ? 32 : 64);
               layer->log2 = 6;
           break;
           case 3:
               if(layer->Control & 2){
                   layer->Width = 64;
                   layer->Height = 64;
                   layer->log2 = 6;
               }
               else{
                   layer->Width = 128;
                   layer->Height = 128;
                   layer->log2 = 7;
               }
           break;
       }
   }
   else if(DrawMode == 6 && num == 2){
       switch ((*p>>14)&0x1) {
           case 0:
               layer->Width = 64;
               layer->Height = 128;
               layer->log2 = 9;
           break;
           case 1:
               layer->Width = 128;
               layer->Height = 64;
               layer->log2 = 10;
           break;
       }
   }
}
//---------------------------------------------------------------------------
u8 LGPU::get_LayerBank(u32 adr,u8 *ret,u32 *size)
{
	u8 bank;
   u32 value,adr1;

   switch(type){
       case LCD_MAIN:
           for(value = 0,bank=0;bank<7;bank++){
               if((io_mem[0x240+bank] & 0x87) == 0x81){
                   value += VRAMBankSize[bank];
                   if(adr <= value)
                       break;
               }
           }
           adr = (adr & 0xFFF00000) | (adr & (value - 1));
           for(bank++;bank<7;bank++)
               if((io_mem[0x240+bank] & 0x87) == 0x81){
                   value += VRAMBankSize[bank];
           }
       break;
       case LCD_SUB:
           value = 0;
           adr1 = adr & ~0x200000;
           for(bank = 2;bank < 10;bank++){
               if((bank == 2 && (io_mem[0x242] & 0x87) == 0x84) ||
                   (bank > 7 && (io_mem[0x240+bank] & 0x87) == 0x81))
               {
                   value += VRAMBankSize[bank];
                   if(adr1 <= value)
                       break;
               }
           }
           adr = (adr & 0xFFF00000) | (adr & (value - 1));
           for(bank++;bank < 10;bank++){
               if((bank == 2 && (io_mem[0x242] & 0x87) == 0x84) ||
                   (bank > 7 && (io_mem[0x240+bank] & 0x87) == 0x81))
               {
                   value += VRAMBankSize[bank];
               }
           }
       break;
   }
   if(size != NULL)
       *size = value;
   bank = mapVRAM[VRAMmap[(adr>>14) & 0x3FF]];
   if(ret != NULL)
       *ret = bank;
	if(!(io_mem[0x240+bank] & 0x80))
   	return 0;
   switch(type){
   	case LCD_SUB:
       	if(bank == 2 && (io_mem[0x240+bank] & 7) == 4)
           	return 1;
           if((io_mem[0x240+bank] & 7) == 1 && (bank == 8 || bank == 9))
           	return 1;
           return 0;
       case LCD_MAIN:
       	if(bank < 7 && (io_mem[0x240+bank] & 7) == 1){
///               if(bank == 6 && (io_mem[0x240] & 0x87) == 0x83)
//                   return 0;
               return 1;
           }
           return 0;
   }
   return 0;
}
//---------------------------------------------------------------------------
u32 LGPU::get_LayerAddress(LPLAYER layer,BOOL scr)
{
	u32 adr;
	u16 Control;

	Control = *((u16 *)reg + 4 + layer->Index);
	if(scr){
   	adr = ((Control >> 8) & 0x1F) << 11;
   	if(layer->parent->type == LCD_SUB)
       	adr = 0x00200000 | adr;
   	else
       	adr += ((layer->parent->reg[3] & 56) << 13);
   }
   else{
   	adr = ((Control >> 2) & 0xf) << 14;
   	if(layer->parent->type == LCD_SUB)
       	adr = 0x00200000 | adr;
   	else
       	adr += ((layer->parent->reg[3] & 7) << 16);
   }
   return adr;                        
}
//---------------------------------------------------------------------------
void LGPU::FillBaseLayer_Internal(LPLAYER layer)
{
   u32 adr,value;
	u16 Control;
   u8 bank;

	Control = *((u16 *)reg + 4 + layer->Index);
   layer->Control = (u16)(((Control & 0x80) >> 6) | ((Control >> 2) & 1));
   layer->ScreenBase = ((Control >> 8) & 0x1F);
   layer->Priority = (u8)(Control & 3);
   layer->bPalette = !((Control >> 7) & 1);
   layer->bMosaic = (u8)((Control >> 6) & 1);
   layer->bWrap = (u8)((Control >> 13) & 1);
   if(ds.get_EmuMode() == 0){
   	layer->CharBase = ((Control >> 2) & 0xf) << 14;
       adr = get_LayerAddress(layer,FALSE);
       layer->bDrawEnable = get_LayerBank(adr,&bank,&value);
       layer->CharBaseBlock = video_mem + ((VRAMmap[(adr>>14)&0x3FF]<<14) | (adr & 0x3FFF));
       adr &= 0x1FFFF;
       if(adr > VRAMBankSize[bank])
           adr &= VRAMBankSize[bank] - 1;
//       if(adr > value)
//           adr = adr;
       layer->maxCharBase = value - adr;
       if(type == LCD_SUB){
           if(bank == 8 && io_mem[0x249] == 0x81)
               layer->maxCharBase += 0x4000;
       }
       adr = get_LayerAddress(layer,TRUE);
       layer->ScreenBaseBlock = video_mem + ((VRAMmap[(adr>>14)&0x3FF]<<14) | (adr & 0x3FFF));
       switch(DrawMode){
           case 3:
           case 4:
               if(layer->Index == 3 && (layer->Control & 2)){
                   adr = layer->ScreenBase << 14;
                   if(layer->parent->type == LCD_SUB)
                       adr |= 0x00200000;
                   layer->ScreenBase = adr;
                   layer->ScreenBaseBlock = video_mem + ((VRAMmap[(adr>>14) & 0x3FF]<<14) | (adr & 0x3FFF));
               }
           break;
           case 5:
               if((layer->Control & 2) != 0){
                   if(layer->Index > 1){
                       adr = layer->ScreenBase << 14;
                       if(layer->parent->type == LCD_SUB)
                           adr |= 0x00200000;
                       layer->ScreenBase = adr;
                       layer->ScreenBaseBlock = video_mem + ((VRAMmap[(adr>>14) & 0x3FF]<<14) | (adr & 0x3FFF));
                   }
               }
           break;
           case 6:
               if(layer->Index == 2){
                   layer->ScreenBaseBlock = video_mem;
                   adr = 0;
               }
           break;
       }
       layer->bDrawEnable |= (u8)(get_LayerBank(adr,&bank,&value) << 1);
       if(value != 0){
           adr &= 0x1FFFF;
           if(value > adr)
               value -= adr;
           else
               value = 0;
       }
       layer->maxScreenBase = value;
   }
   else{
       layer->bDrawEnable = 3;
       layer->CharBase = ((Control >> 2) & 0x3) << 14;
       layer->CharBaseBlock = video_mem + layer->CharBase;
       layer->ScreenBaseBlock = video_mem + (layer->ScreenBase<<11);
       layer->maxScreenBase = 0xFFFFF;
       layer->maxCharBase = 0xFFFFF;
   }
}
//---------------------------------------------------------------------------
void LGPU::FillBaseLayer(LPLAYER layer)
{
   FillBaseLayer_Internal(layer);
   FillListBackground();
}
//---------------------------------------------------------------------------
void LGPU::rgbAlphaLayerLine(u16 xStart,u16 xEnd)
{
   u8 r,g,b,*tpeva,*tpevb;
   u32 cols,*pTarget,col1,col2,col;

   pTarget = &((u32 *)TargetBuffer)[xStart];
   for(xEnd -= xStart;xEnd != 0;xEnd--){
  		r = 0;
       g = 1;
       col2 = *pTarget++;
       cols = *pCurOB;
       *pCurOB++ = (u32)-1;
       if((col1 = *pCurSB++) < 0x80000000){
           b = 0;
           if(col2 >= 0x80000000)
               col2 = (u32)-1;
           else if((cols & 0x4000000))
               cols = (u32)-1;
       }
       else if(col2 < 0x80000000){
       	b = 0;
           col1 = (u32)-1;
           if((cols & 0x04000000))
               cols = (u32)-1;
       }
       else{
           col2 = col1 = tpal_bg[0];
           b = 1;                        
       }
       if(cols != (u32)-1 && !(cols & 0x01000000)){
           if((cols & 0x80000000)){
               if(Source[5] != NULL){
                  	col2 = col1;
                   col1 = (u16)cols;
                   r = 1;
                   if(CountSource == 0 && CountTarget < 2)
                   	g = 0;
               }
               else{
                   col2 = (u16)cols;
                   r = (u8)(2|((cols & 0x30000) << 4));
               }
           }
           else{
               if(Source[5] != NULL){
                   col1 = (u16)cols;
                   r = 1;
               }
               else{
                   if(Target[5] != NULL){
                       if(CountSource < 1) {
                           col2 = (u16)cols;
                           col1 = (u32)-1;
                           r = (u8)(2|((cols & 0x30000) << 4));
                       }
						else{
                       	if(!b){
                               col2 = (u16)cols;
                               r = (u8)(2|((cols & 0x30000) << 4));
                           }
                           else if(Source[6] == 0/* && Target[6] == 0*/){// Scribblenauts
                           	col1 = (u16)cols;
                               col2 = (u32)-1;
                               r = 1;
                           }
                       }
                   }
                   else{
                       col1 = (u16)cols;
                       col2 = (u32)-1;
                       r = 1;
                   }
               }
           }
           cols = (u32)-1;
       }
       if(col1 == (u32)-1)
           col = col2;
       else if(col2 == (u32)-1)
           col = col1;
       else{
           if((col1 & 0x40000000)){
               g = (u8)(((col1 & 0x00FF0000) >> 20) + 1);
           	if(g > 1 && (r & 0xF) == 2){
               	cols = ((u32 *)ZBuffer)[((u32)pTarget - (u32)TargetBuffer) >> 2];
                   if((r >>= 4) < (b = GETZPRTSRC(cols)))
                   	g = 0;
                   else if(r == b)//lostmagic
                       g >>= 1;
                   cols = (u32)-1;
               }
               tpeva = tabColor + (g << 5);
               tpevb = tabColor + ((16 - g) << 5);
           }
           else if((col2 & 0x40000000)){
               r = (u8)(((col2 & 0x00FF0000) >> 20) + 1);
				if(g){
                   tpeva = tabColor + (r << 5);
                   tpevb = tabColor + ((16 - r) << 5);
               }
               else{
                   tpevb = tabColor + (r << 5);
                   tpeva = tabColor + ((16 - r) << 5);
               }
           }
           else{
               tpeva = peva;
               tpevb = pevb;
           }
          	if((r = (u8)(tpeva[(col1 >> 10) & 0x1F] + tpevb[(col2 >> 10) & 0x1F])) > 31)
           	r = 31;
          	if((g = (u8)(tpeva[(col1 >> 5) & 0x1F] + tpevb[(col2 >> 5) & 0x1F])) > 31)
           	g = 31;
          	if((b = (u8)(tpeva[col1 & 0x1F] + tpevb[col2 & 0x1F])) > 31)
           	b = 31;
          	col = (u32)((r << 10)| (g << 5) | b);
       }
       if(cols != (u32)-1){
           tpeva = peva;
           tpevb = pevb;
       	if(Target[5] != NULL){
           	col2 = (u16)cols;
               col1 = ((u32 *)ZBuffer)[((u32)pTarget - (u32)TargetBuffer) >> 2];
               if(GETZPRTSRC(col1) > (u8)(cols >> 16)){
                   tpeva = tabColor;
                   tpevb = &tabColor[16 << 5];
               }
               col1 = col;
           }
           else{
               col1 = (u16)cols;
               col2 = col;
           }
           if((r = (u8)(tpeva[(col1 >> 10) & 0x1F] + tpevb[(col2 >> 10) & 0x1F])) > 31)
               r = 31;
           col = r << 10;
           if((r = (u8)(tpeva[(col1 >> 5) & 0x1F] + tpevb[(col2 >> 5) & 0x1F])) > 31)
               r = 31;
           col |= r << 5;
           if((r = (u8)(tpeva[col1 & 0x1F] + tpevb[col2 & 0x1F])) > 31)
               r = 31;
           col |= r;
       }
       if(col & 0x40000000){
			col1 = ((col & 0xF00000) >> 20) + 1;
       	b = (u8)((col & 0x1F) * col1 >> 4);
           g = (u8)(((col >> 5) & 0x1F) * col1 >> 4);
           r = (u8)(((col >> 10) & 0x1F) * col1 >> 4);
          	col = (u32)((r << 10)| (g << 5) | b);
       }
       *toutBuffer++ = (u16)col;
   }
}
//---------------------------------------------------------------------------
void LGPU::rgbAlphaLayerLineNoOAM(u16 xStart,u16 xEnd)
{
   u8 r,*tpeva,*tpevb;
   u32 *pTarget,col1,col2,col;

   pTarget = &((u32 *)TargetBuffer)[xStart];
   pCurOB += (xEnd - xStart);
   for(;xStart < xEnd;xStart++){
       col2 = *pTarget++;
       if((col1 = *pCurSB++) < 0x80000000){
           if(col2 >= 0x80000000)
               col2 = (u32)-1;
           else
           	col2 = col2;
       }
       else if(col2 < 0x80000000)
           col1 = (u32)-1;
       else
           col2 = col1 = tpal_bg[0];
       if(col1 == (u32)-1)
           col = col2;
       else if(col2 == (u32)-1)
           col = col1;
       else{
           if(col1 & 0x40000000){
               r = (u8)((col1 & 0x00FF0000) >> 20);
               tpeva = tabColor + (r << 5);
               tpevb = tabColor + ((15 - r) << 5);
           }
           else{
               tpeva = peva;
               tpevb = pevb;
           }
          	if((r = (u8)(tpeva[(col1 >> 10) & 0x1F] + tpevb[(col2 >> 10) & 0x1F])) > 31)
           	r = 31;
           col = r << 10;
          	if((r = (u8)(tpeva[(col1 >> 5) & 0x1F] + tpevb[(col2 >> 5) & 0x1F])) > 31)
           	r = 31;
           col |= (r << 5);
          	if((r = (u8)(tpeva[col1 & 0x1F] + tpevb[col2 & 0x1F])) > 31)
           	r = 31;
           col |= r;
       }
       if(col & 0x40000000){
			col1 = ((col & 0xF00000) >> 20) + 1;
       	col2 = ((col & 0x1F) * col1 >> 4);
           r = (u8)(((col >> 5) & 0x1F) * col1 >> 4);
           col2 |= r << 5;
           r = (u8)(((col >> 10) & 0x1F) * col1 >> 4);
          	col = col2 | (r << 10);
       }
       *toutBuffer++ = (u16)col;
   }
}
//---------------------------------------------------------------------------
void LGPU::rgbNormalLine(u16 xStart,u16 xEnd)
{
   u16 r;
   u32 o,b;

   for(;xStart < xEnd;pCurOB++,xStart++){
   	b = (u32)*pCurSB++;
       if((o = *pCurOB) != (u32)-1){
           *pCurOB = (u32)-1;
           if((o & 0x81000000) != 0){
               if((r = (u16)(peva[(o >> 10) & 0x1F] + pevb[(b >> 10) & 0x1F])) > 31)
               	r = 31;
               *toutBuffer = (u16)(r << 10);
               if((r = (u16)(peva[(o >> 5) & 0x1F] + pevb[(b >> 5) & 0x1F])) > 31)
               	r = 31;
               *toutBuffer |= (u16)(r << 5);
               if((r = (u16)(peva[o & 0x1F] + pevb[b & 0x1F])) > 31)
               	r = 31;
               *toutBuffer++ |= r;
           }
           else
               *toutBuffer++ = (u16)o;
       }
       else if(b != (u32)-1)
           *toutBuffer++ = (u16)(b & 0x7FFF);
       else
       	*toutBuffer++ = tpal_bg[0];
   }
}
//---------------------------------------------------------------------------
void LGPU::rgbNormalLineNoOAM(u16 xStart,u16 xEnd)
{
   u32 b;

   pCurOB += (xEnd - xStart);
   for(;xStart < xEnd;xStart++){
   	b = (u32)*pCurSB++;
       *toutBuffer++ = (b != (u32)-1) ? (u16)(b & 0x7FFF) : tpal_bg[0];
   }
}
//---------------------------------------------------------------------------
void LGPU::rgbFadeLineUp(u16 xStart,u16 xEnd)
{
   u16 col1;
   u32 col,cols,*p;
   u8 r,g,b;

   for(p = pCurSB + xEnd - xStart;pCurSB < p;pCurSB++){
       cols = col = *pCurOB;
       *pCurOB++ = (u32)-1;
       if(col == (u32)-1){
           col = *pCurSB;
           col1 = (u16)col;
           if(col1 == (u16)-1)
               col1 = tpal_bg[0];
       }
       else{
           col1 = (u16)col;
           col = cols;
       }
       if(!(col & 0x80000000)){
           b = (u8)(col1 & 0x1F);
           g = (u8)((col1 >> 5) & 0x1F);
           r = (u8)((col1 >> 10) & 0x1F);
           if((r += pevy1[r]) > 31) r = 31;
           if((g += pevy1[g]) > 31) g = 31;
           if((b += pevy1[b]) > 31) b = 31;
           col1 = (u16)((r << 10)| (g << 5) | b);
       }
       else if(cols != (u32)-1 && (cols & 0x01000000)){
           col1 = (u16)cols;
           col = *pCurSB;
           if((r = (u8)(peva[(col1 >> 10) & 0x1F] + pevb[(col >> 10) & 0x1F])) > 31) r = 31;
           if((g = (u8)(peva[(col1 >> 5) & 0x1F] + pevb[(col >> 5) & 0x1F])) > 31) g = 31;
           if((b = (u8)(peva[col1 & 0x1F] + pevb[col & 0x1F])) > 31) b = 31;
           col1 = (u16)((r << 10)| (g << 5) | b);
       }
       *toutBuffer++ = (u16)col1;
   }
}
//---------------------------------------------------------------------------
void LGPU::rgbFadeLineUpNoOAM(u16 xStart,u16 xEnd)
{
   u16 col1;
   u32 col;
   u8 r,g,b;

	pCurOB += (xEnd - xStart);
   for(;xStart<xEnd;xStart++){
   	col = *pCurSB++;
       col1 = (u16)col;
       if(col1 == (u16)-1)
       	col1 = tpal_bg[0];
       if(!(col & 0x80000000)){
           b = (u8)(col1 & 0x1F);
           g = (u8)((col1 >> 5) & 0x1F);
           r = (u8)((col1 >> 10) & 0x1F);
           if((r += pevy1[r]) > 31) r = 31;
           if((g += pevy1[g]) > 31) g = 31;
           if((b += pevy1[b]) > 31) b = 31;
           col1 = (u16)((r << 10)| (g << 5) | b);
       }
       *toutBuffer++ = (u16)col1;
   }
}
//---------------------------------------------------------------------------
void LGPU::rgbFadeLineDown(u16 xStart,u16 xEnd)
{
   u16 col1;
   u32 col,cols,*p;
   u8 r,g,b;

   for(p = pCurSB + xEnd - xStart;pCurSB < p;pCurSB++){
       cols = col = *pCurOB;
       *pCurOB++ = (u32)-1;
       if(col == (u32)-1){
           col = *pCurSB;
           col1 = (u16)col;
           if(col1 == (u16)-1)
               col1 = tpal_bg[0];
       }
       else{
           col1 = (u16)col;
           col = cols;
       }
       if(!(col & 0x80000000)){
           b = (u8)(col1 & 0x1F);
           g = (u8)((col1 >> 5) & 0x1F);
           r = (u8)((col1 >> 10) & 0x1F);
           r -= pevy[r];
           g -= pevy[g];
          	b -= pevy[b];
           col1 = (u16)((r << 10) | (g << 5) | b);
       }
       else if(cols != (u32)-1 && (cols & 0x01000000)){
           col1 = (u16)cols;
           col = *pCurSB;
           if((r = (u8)(peva[(col1 >> 10) & 0x1F] + pevb[(col >> 10) & 0x1F])) > 31) r = 31;
           if((g = (u8)(peva[(col1 >> 5) & 0x1F] + pevb[(col >> 5) & 0x1F])) > 31) g = 31;
           if((b = (u8)(peva[col1 & 0x1F] + pevb[col & 0x1F])) > 31) b = 31;
           col1 = (u16)((r << 10)| (g << 5) | b);
       }
       *toutBuffer++ = (u16)col1;
   }
}
//---------------------------------------------------------------------------
void LGPU::rgbFadeLineDownNoOAM(u16 xStart,u16 xEnd)
{
   u16 col1;
   u32 col;
   u8 r,g,b;

   pCurOB += (xEnd - xStart);
   for(;xStart < xEnd;xStart++){
       col = *pCurSB++;
       col1 = (u16)col;
       if(col1 == (u16)-1)
       	col1 = tpal_bg[0];
       if(!(col & 0x80000000)){
           b = (u8)(col1 & 0x1F);
           g = (u8)((col1 >> 5) & 0x1F);
           r = (u8)((col1 >> 10) & 0x1F);
           r -= pevy[r];
           g -= pevy[g];
          	b -= pevy[b];
           col1 = (u16)((r << 10)| (g << 5) | b);
       }
       *toutBuffer++ = (u16)col1;
   }
}
//---------------------------------------------------------------------------
void LGPU::GetBrightnessIndex()
{
   u8 i;

   evy = (u8)((i = (u8)(BLENDY & 31)) > 16 ? 16 : i);
   pevy = tabColor + (evy << 5);
   pevy1 = tabColor1 + (evy << 5);
}
//---------------------------------------------------------------------------
void LGPU::GetAlphaIndex()
{
   u8 i;

   eva = (u8)((i = (u8)(BLENDV & 31)) > 16 ? 16 : i);
   evb = (u8)((i = (u8)((BLENDV >> 8) & 31)) > 16 ? 16 : i);
   peva = tabColor + (eva << 5);
   pevb = tabColor + (evb << 5);
}
//---------------------------------------------------------------------------
int LGPU::GetPointsWindow(LPPOINTWINDOW pt)
{
   LPWINGBA Win[2],w;
   u8 CountPoint,check;
   int i,x,i1,CountWindow;

   for(CountWindow = -1,x = 0,i1 = 1;x < 2;x++,i1 <<= 1){
       if(!(WinEnable & i1) || !win[x].Visible)
           continue;
       w = &win[x];
       if(w->left == w->right || w->right < w->left)
           continue;
       if(w->Enable && render_line >= w->top && render_line < w->bottom)
           Win[++CountWindow] = w;
   }
   if(CountWindow > 0 && Win[1]->left <= Win[0]->left){
       w = Win[0];
       Win[0] = Win[1];
       Win[1] = w;
   }
   CountPoint = 0;
   x = i = 0;
   for(i1 = 0;i1 <= CountWindow;i1++){
       check = 0;
       w = Win[i1];
       if(w->left > x){
           if(w->left > w->right && w->right >= i){
               i += pt[CountPoint].width = (u16)((x = w->right) - i);
               pt[CountPoint++].winc = &w->Control;
               w->right = (u8)(256-1);
           }
           i += pt[CountPoint].width = (u16)(w->left - x);
           pt[CountPoint++].winc = &winOut;
       }
       if(w->left < w->right && x < w->right){
           if(i1 != CountWindow && Win[i1+1]->left < w->right){
               x = Win[i1+1]->left - x;
               check = 1;
           }
           else
               x = w->right;
           i += pt[CountPoint].width = (u16)(x - i);
           if(i > 256)
               pt[CountPoint].width = (u16)(256 - (i - 256));
           pt[CountPoint++].winc = &w->Control;
           if(!check)
               continue;
           x = Win[i1+1]->right;
           i += pt[CountPoint].width = (u16)(x - i);
           if(i > rcDisplay.right)
               pt[CountPoint].width = (u16)(256 - (i - 256));
           pt[CountPoint++].winc = &Win[i1+1]->Control;
           if(i >= w->right)
               continue;
           x = w->right;
           i += pt[CountPoint].width = (u16)(x - i);
           if(i > rcDisplay.right)
               pt[CountPoint].width = (u8)(256 - (i - 256));
           pt[CountPoint++].winc = &w->Control;
       }
   }
   if(i < 256){
       pt[CountPoint].width = (u16)(256 - i);
       pt[CountPoint++].winc = &winOut;
   }
   return CountPoint;
}
//---------------------------------------------------------------------------
void LGPU::WritePalette(u16 adress,u8 mode)
{
   u16 col;

   if((type == LCD_MAIN && adress > 0x3FF) || (type == LCD_SUB && adress < 0x400))
   	return;
   adress &= 0x3FF;
   col = BGR(((u16 *)pal_bg)[adress >> 1]);
   if(adress > 0x1FE){
   	tpal_obj[(adress - 0x200) >> 1] = col;
       if(mode == AMM_WORD){
       	adress += (u16)2;
           col = BGR(((u16 *)pal_bg)[adress >> 1]);
			tpal_obj[(adress - 0x200) >> 1] = col;
		}
   }
   else{
   	tpal_bg[adress >> 1] = col;
		if(mode == AMM_WORD){
       	adress += (u16)2;
           col = BGR(((u16 *)pal_bg)[adress >> 1]);
			tpal_bg[adress >> 1] = col;
		}
   }
}
