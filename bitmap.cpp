#include "ideastypes.h"

#ifndef __WIN32__
//--------------------------------------------------------------------------------
HBITMAP CreateBitmapMask(HBITMAP src,guint32 color0)
{
    HBITMAP hbitmap;
	BYTE *ptr0,*ptr1;
	UINT color,x,y,width,height;

	if(src == NULL)
		return NULL;
	width = gdk_pixbuf_get_width (src);
	height = gdk_pixbuf_get_height(src);
	ptr1 = (BYTE *)gdk_pixbuf_get_pixels(src);
	if(width == 0 || height == 0 || ptr1 == NULL)
		return NULL;
	hbitmap = gdk_pixbuf_new(GDK_COLORSPACE_RGB,true,8,width,height);
	if(hbitmap == NULL)
		return NULL;
	ptr0 = (BYTE *)gdk_pixbuf_get_pixels(hbitmap);	
	if(ptr0 == NULL)
		return NULL;
	for(y=0;y<height;y++){
		for(x=0;x<width;x++,ptr1 += 3,ptr0 += 4){
			color = ptr1[2];
			color <<= 8;
			color |= ptr1[1];
			color <<= 8;
			color |= ptr1[0];
			if(color == color0)
				*((LPDWORD)ptr0) = 0;
			else 
				*((LPDWORD)ptr0) = 0xFFFFFFFF;
			
		}
	}
	return hbitmap;
}
//--------------------------------------------------------------------------------
HBITMAP LoadBitmap(HINSTANCE instance,LPCTSTR name)
{
    HBITMAP hbitmap,hbitmap0;
    void *hRsrc;
    unsigned char *ptr;
    BITMAPINFO *info;
    LONG bytes_perline;   
	BYTE code0, code1,*lpIn,*pal,*sbuf;
	BYTE r,g,b,a,r1,g1,b1;
	BOOL bFlag;
	int extra_byte,pixel_ptr,bytes_per_pixel,i,w,h;

    if (!(hRsrc = FindResource(instance,name,RT_BITMAP))) 
		return 0;
    if ((info = (BITMAPINFO *)LockResource(hRsrc)) == NULL) 
		return 0;
	hbitmap = gdk_pixbuf_new(GDK_COLORSPACE_RGB,false,8,info->bmiHeader.biWidth,info->bmiHeader.biHeight);
	pal = (BYTE *)((char *)info + info->bmiHeader.biSize);
	ptr = (BYTE *)gdk_pixbuf_get_pixels(hbitmap);
	bytes_perline = gdk_pixbuf_get_rowstride(hbitmap);
	lpIn = (BYTE *)pal;
	if(info->bmiHeader.biBitCount <= 8){
		i = sizeof(RGBQUAD);
		if(info->bmiHeader.biClrUsed)
			i *= info->bmiHeader.biClrUsed;
		else
			i *= (1 << info->bmiHeader.biBitCount);			
		lpIn += i;
	}
	pixel_ptr  = 0;
	bytes_per_pixel = 3;
	switch(info->bmiHeader.biCompression){
		case BI_RGB:
			switch(info->bmiHeader.biBitCount){
				case 1:
        			for (pixel_ptr=h=0;h<info->bmiHeader.biHeight;h++){
            			for (w =0;w<info->bmiHeader.biWidth;){
							i = *((int *)lpIn);
							lpIn += 4;
							for(b=0;b<24 && w<info->bmiHeader.biWidth;b+=8){
								for(a=0;a<8 && w<info->bmiHeader.biWidth;a++,w++){
									r = (i >> (7-a)) & 1;
									ptr[w*3+0] = pal[r*4+0];
									ptr[w*3+1] = pal[r*4+1];
									ptr[w*3+2] = pal[r*4+2];
								}					
								i >>= 8;
							}
            			}
            			ptr += bytes_perline;
        			}					
				break;
			}
		break;
		case BI_RLE4:
			bFlag = FALSE;
			do {
    			code0 = *lpIn++;
    			code1 = *lpIn++;
    			if (code0 == 0) {   
      				switch (code1) {
      					case  0: /* EOL - end of line  */
							pixel_ptr = 0;
							ptr += bytes_perline;
						break;
      					case  1: /* EOI - end of image */
							bFlag = TRUE;
						break;
      					case  2: /* skip */
							pixel_ptr += *lpIn++ * bytes_per_pixel;
							ptr     += *lpIn++ * bytes_perline;
							if (pixel_ptr >= info->bmiHeader.biWidth * bytes_per_pixel) {
	  							pixel_ptr = 0;
	  							ptr    += bytes_perline;
							}
						break;
      					default: /* absolute mode */
							extra_byte = (((code1 + 1) & (~1)) / 2) & 0x01;
							if (pixel_ptr/bytes_per_pixel + code1 > info->bmiHeader.biWidth)
	  							goto ex_LoadBitmap_err;
							code0 = code1;
							for (i = 0; i < code0 / 2; i++) {
	    						code1 = lpIn[i] >> 4;
	    						ptr[pixel_ptr + 0] = pal[code1 * 4 + 2];
	    						ptr[pixel_ptr + 1] = pal[code1 * 4 + 1];
	    						ptr[pixel_ptr + 2] = pal[code1 * 4 + 0];
	    						pixel_ptr += bytes_per_pixel;
	    						if (2 * i + 1 <= code0) {
	      							code1 = lpIn[i] & 0x0F;
	      							ptr[pixel_ptr + 0] = pal[code1 * 4 + 2];
	      							ptr[pixel_ptr + 1] = pal[code1 * 4 + 1];
	      							ptr[pixel_ptr + 2] = pal[code1 * 4 + 0];
	      							pixel_ptr += bytes_per_pixel;
	    						}
	  						}
							if (code0 & 0x01) {
	    						code1 = lpIn[i] >> 4;
	    						ptr[pixel_ptr + 0] = pal[code1 * 4 + 2];
	    						ptr[pixel_ptr + 1] = pal[code1 * 4 + 1];
	    						ptr[pixel_ptr + 2] = pal[code1 * 4 + 0];
	    						pixel_ptr += bytes_per_pixel;
								lpIn++;
	  						}  						
							lpIn += code0 / 2;
							if (extra_byte)
	  							lpIn++;
						break;
					}
    			} 
				else {      
      				if (pixel_ptr/bytes_per_pixel + code0 > info->bmiHeader.biWidth)
						goto ex_LoadBitmap_err;
					b = pal[(code1 >> 4) * 4 + 2];
					g = pal[(code1 >> 4) * 4 + 1];
					r = pal[(code1 >> 4) * 4 + 0];
					b1 = pal[(code1 & 0x0F) * 4 + 2];
					g1 = pal[(code1 & 0x0F) * 4 + 1];
					r1 = pal[(code1 & 0x0F) * 4 + 0];
					for (i = 0; i < code0; i++) {
	  					if ((i & 1) == 0) {
	    					ptr[pixel_ptr + 0] = b;
	    					ptr[pixel_ptr + 1] = g;
	    					ptr[pixel_ptr + 2] = r;
	  					} 
						else {
	    					ptr[pixel_ptr + 0] = b1;
	    					ptr[pixel_ptr + 1] = g1;
	    					ptr[pixel_ptr + 2] = r1;
	  					}
	  					pixel_ptr += bytes_per_pixel;
					}
      			}
  			} while (!bFlag);			
		break;
		case BI_RLE8:
			bFlag = FALSE;
			do {
   				code0 = *lpIn++;
   				code1 = *lpIn++;
				if(code0 == 0){
					switch(code1){
						case 0:
							pixel_ptr = 0;
							ptr += bytes_perline;
						break;
						case 1:
							bFlag = TRUE;
						break;
						case 2:
							pixel_ptr += *lpIn++ * bytes_per_pixel;
							ptr += *lpIn++ * bytes_perline;
							if (pixel_ptr >= info->bmiHeader.biWidth * bytes_per_pixel) {
  								pixel_ptr = 0;
  								ptr += bytes_perline;
							}
						break;
						default:
							if (pixel_ptr / bytes_per_pixel + code1 > info->bmiHeader.biWidth)
  								goto ex_LoadBitmap_err;
							extra_byte = code1 & 0x01;
							code0 = code1;
							while (code0--) {
  								code1 = *lpIn++;
  								ptr[pixel_ptr + 0] = pal[code1 * 4 + 2];
    							ptr[pixel_ptr + 1] = pal[code1 * 4 + 1];
    							ptr[pixel_ptr + 2] = pal[code1 * 4 + 0];
  								pixel_ptr += bytes_per_pixel;
							}
							if (extra_byte)
  								lpIn++;
   							break;
					}
				}
				else{
 					if (pixel_ptr/bytes_per_pixel + code0 > info->bmiHeader.biWidth)
						goto ex_LoadBitmap_err;
					r = pal[code1 * 4 + 2];
					g = pal[code1 * 4 + 1];
					b = pal[code1 * 4 + 0];
					while (code0--) {
  						ptr[pixel_ptr + 0] = r;
  						ptr[pixel_ptr + 1] = g;
  						ptr[pixel_ptr + 2] = b;
  						pixel_ptr += bytes_per_pixel;
					}
   				}
			}while(!bFlag);
		break;
	}				
	hbitmap0 = gdk_pixbuf_flip(hbitmap,FALSE);
	g_object_unref (hbitmap);
    return hbitmap0;
ex_LoadBitmap_err:
	if(hbitmap != NULL)
		g_object_unref (hbitmap);
	return NULL;
}
//--------------------------------------------------------------------------------
HICON LoadIcon(HINSTANCE instance,LPCTSTR name, INT desiredx, INT desiredy, UINT loadflags)
{
	CURSORICONDIR *p;
	CURSORICONDIRENTRY *p1,*entry;
 	UINT iTotalDiff, iXDiff, iYDiff, iColorDiff;
    UINT iTempXDiff, iTempYDiff, iTempColorDiff;
    int i,colors,x,y,bDoStretch;
    BITMAPINFO *bmi;
	unsigned char *pBits,*pc;
	unsigned long xinc,yinc,x1,y1;
	INT width,height;
	HBITMAP hbitmap;
	LPDWORD bit;
	
	p = (CURSORICONDIR *)FindResource(instance,name,(char *)RT_GROUP_ICON);
	p = (CURSORICONDIR *)LockResource(p);
	if(p == NULL || p->idCount < 1)
		return NULL;
 	p1 = NULL;
 	if(p->idCount == 1)
 		p1 = &p->idEntries[0];
	else{
    	colors = 256;
    	iXDiff = iYDiff = 0;
    	iTotalDiff = 0xFFFFFFFF;
    	iColorDiff = 0xFFFFFFFF;
    	for(i = 0,entry = &p->idEntries[0]; i < p->idCount;i++,entry++){
        	iTempXDiff = abs(desiredx - entry->ResInfo.icon.bWidth);
        	iTempYDiff = abs(desiredy - entry->ResInfo.icon.bHeight);
        	if(iTotalDiff > (iTempXDiff + iTempYDiff)){
            	iXDiff = iTempXDiff;
            	iYDiff = iTempYDiff;
            	iTotalDiff = iXDiff + iYDiff;
        	}
    	}
    	for(i = 0,entry = &p->idEntries[0]; i < p->idCount; i++,entry++){
        	if(abs(desiredx - entry->ResInfo.icon.bWidth) == iXDiff &&
           		abs(desiredy - entry->ResInfo.icon.bHeight) == iYDiff){
            	iTempColorDiff = abs(colors - (1<<entry->wBitCount));
            	if(iColorDiff > iTempColorDiff){
                	p1 = entry;
                	iColorDiff = iTempColorDiff;
            	}
        	}
    	}
	}
	if(p1 == NULL)
		return NULL;
	bmi = (BITMAPINFO *)FindResource(instance,MAKEINTRESOURCE(p1->wResId),RT_ICON);
	bmi = (BITMAPINFO *)LockResource(bmi);
	if(bmi == NULL)
		return NULL;
    if(!desiredx)
    	desiredx = bmi->bmiHeader.biWidth;
    if(!desiredy)
    	desiredy = bmi->bmiHeader.biHeight / 2;
    width = desiredx;
    height = desiredy;
    bDoStretch = (bmi->bmiHeader.biHeight/2 != desiredy) ||
      (bmi->bmiHeader.biWidth != desiredx);
    if((bmi->bmiHeader.biSize != sizeof(BITMAPCOREHEADER)) &&
         (bmi->bmiHeader.biSize != sizeof(BITMAPINFOHEADER)  ||
          bmi->bmiHeader.biCompression != BI_RGB) )
			return NULL;
	hbitmap = gdk_pixbuf_new(GDK_COLORSPACE_RGB,true,8,desiredx,desiredx);
	if(hbitmap == NULL)
		goto ex_LoadError;
	bit = (LPDWORD)gdk_pixbuf_get_pixels(hbitmap);
    pBits = (unsigned char *)(char *)bmi + sizeof(BITMAPINFOHEADER);
    if(bmi->bmiHeader.biBitCount <= 8)
    	pBits += (1 << bmi->bmiHeader.biBitCount) * sizeof(RGBQUAD);
    if(bDoStretch){
    	xinc = bmi->bmiHeader.biWidth * 256 / desiredx;
     	yinc = bmi->bmiHeader.biHeight * 128 / desiredy;
   	}
   	else
   		xinc = yinc = 256;
    for(y = 0;y < desiredy;y++){    	
		for(x = 0;x < desiredx;x++){
			x1 = ((desiredy - y - 1) * bmi->bmiHeader.biWidth*yinc + x*xinc) >> 8;
			pc = (unsigned char *)(pBits + x1 * 3);
			y1 = pc[0] | (pc[1] << 8) | (pc[2] << 16);
			bit[y*width+x] = y1;
		}
	}
    pBits += (((bmi->bmiHeader.biWidth*bmi->bmiHeader.biBitCount + 31) >> 5) << 2) *
    	(bmi->bmiHeader.biHeight / 2);	
    for(y = 0;y < desiredy;y++){    	
		for(x = 0;x < desiredx;x++){
			y1 = ((desiredy - y - 1) * bmi->bmiHeader.biWidth*yinc) >> 8;
			x1 = (x*xinc) >> 8;		
			y1 = (pBits[(y1 + x1 >> 3)] & (1 << (7-(x1 & 7))));	
			x1 = bit[y*width+x];
			x1 |=  (y1 != 0 ? 0 : 0xFF000000);
			bit[y*width+x] = x1;
		}
	}	
	return hbitmap;
ex_LoadError:
	if(hbitmap != NULL)
		g_object_unref (hbitmap);
	return NULL;
}
//--------------------------------------------------------------------------------
HICON LoadIcon(HINSTANCE instance,LPCSTR lpIconName)
{
	return LoadIcon(instance,lpIconName,16,16,0);
}

#endif

