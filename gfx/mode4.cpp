#include "ideastypes.h"
#include "mode4.h"

//---------------------------------------------------------------------------
u32 I_FASTCALL drawPixelMode4Rot(LPLAYER layer)
{
	s32 x,y,xTile;

	x = layer->CurrentScrollX >> 8;
   y = layer->CurrentScrollY >> 8;
   if(!layer->bWrap && (x < 0 || y < 0 || x > (layer->Width << 3) || y > (layer->Height << 3)))
   	return (u32)-1;
	switch(layer->Control){
       case 3:
           if(layer->bDrawEnable < 3)
               return (u32)-1;
           x &= ((layer->Width << 3) - 1);
           y &= ((layer->Height << 3) - 1);
           x += (y << (layer->log2 + 3));
           if((x << 1) > layer->maxScreenBase)
               return (u32)-1;
           x = ((u16 *)layer->ScreenBaseBlock)[x];
           if(!(x & 0x8000))
               return (u32)-1;
           return BGR(x);
       case 2:
           x &= ((layer->Width << 3) - 1);
           y &= ((layer->Height << 3) - 1);
           x += (y << (layer->log2 + 3));
           if(x > layer->maxScreenBase)
               return -1;
           y = layer->ScreenBase + x;
           x = video_mem[((VRAMmap[(y >> 14) & 0x3FF] << 14) | (y & 0x3FFF))];
           if(x == 0)
               return (u32)-1;
           return layer->pal[x];
       default:
           if(layer->pal == NULL || !layer->bDrawEnable)
               return (u32)-1;
           layer->vFlip = (u8)((y & 0x7) << 3);
           xTile = ((x >> 3) & (layer->Width - 1)) + (((y >> 3) & (layer->Height - 1)) << layer->log2);
//           if((xTile << 1) > layer->maxScreenBase)
//               return -1;
           y = ((u16 *)layer->ScreenBaseBlock)[xTile];
           xTile = (y & 0x3FF) << 6;
           if((y & 0x0400) != 0)
               xTile += 7 - (x & 7);
           else
               xTile += (x & 7);
           xTile = layer->CharBaseBlock[xTile + ((y & 0x800) ? 56 - layer->vFlip : layer->vFlip)];
           if(xTile == 0)
               return -1;
           if(layer->bExtPal){
               y = layer->pal[xTile + ((y & 0xF000) >> 4)];
               return BGR(y);
           }
           return layer->pal[xTile];
   }
}
//---------------------------------------------------------------------------
void I_FASTCALL InitLayerMode4Rot(LPLAYER layer)
{
   layer->CurrentScrollX = layer->CurrentX;
   layer->CurrentScrollY = layer->CurrentY;
   if((layer->parent->reg[3] & 0x40) && layer->bPalette){         
   	layer->pal = layer->parent->texpal_bg;
       layer->bExtPal = 1;
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
   	layer->bExtPal = 0;
		layer->pal = layer->parent->tpal_bg;
   }
}

