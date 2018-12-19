//---------------------------------------------------------------------------
#include "ideastypes.h"

#include "mode6.h"

//---------------------------------------------------------------------------
void I_FASTCALL InitLayerMode6Rot(LPLAYER layer)
{
   layer->CurrentScrollX = layer->CurrentX;
   layer->CurrentScrollY = layer->CurrentY;
/*   if((layer->parent->reg[3] & 0x40)){
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
}
//---------------------------------------------------------------------------
void I_FASTCALL PostDrawMode6Rot(LPLAYER layer)
{
   layer->CurrentScrollX += layer->rotMatrix[0];
   layer->CurrentScrollY += layer->rotMatrix[2];
}
//---------------------------------------------------------------------------
u32 I_FASTCALL drawPixelMode6(LPLAYER layer)
{
   int xTile,yTile,x,y;

	if(layer->pal == NULL)
   	return (u16)-1;
   xTile = (x = layer->CurrentScrollX >> 8) >> 3;
   yTile = (y = layer->CurrentScrollY >> 8) >> 3;
   if(layer->bWrap || (xTile >= 0 && xTile < layer->Width && yTile >= 0 && yTile < layer->Height)){
       x &= (layer->Width << 3)-1;
       y &= (layer->Height << 3)-1;
       if((x = layer->ScreenBaseBlock[x + (y << layer->log2)]) == 0)
          	return -1;
       return layer->pal[x];
   }
   else
       return (u32)-1;

}

