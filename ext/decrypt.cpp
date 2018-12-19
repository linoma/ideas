#include "ideastypes.h"
#include "lds.h"

static u8 keybuf[0x1048];
static u32 keycode[4];
//---------------------------------------------------------------------------
void decrypt_64bit_up(u32 *ptr)
{
   u32 x,y,z;
   int i;

   z = ptr[1] ^ *((u32 *)&keybuf[0x44]);
   x = ptr[0] ^ *((u32 *)&keybuf[0x40]);
   y =  *((u32 *)&keybuf[0x048 + ((z >> 24) & 0xFF) * 4]);
   y += *((u32 *)&keybuf[0x448 + ((z >> 16) & 0xFF) * 4]);
   y ^= *((u32 *)&keybuf[0x848 + ((z >> 8)  & 0xFF) * 4]);
   y += *((u32 *)&keybuf[0xC48 + (z & 0xFF) * 4]);
   y = x ^ y;
   for(i=15;i>=0;i--){
       y =  *((u32 *)&keybuf[0x048 + ((z >> 24) & 0xFF) * 4]);
       y += *((u32 *)&keybuf[0x448 + ((z >> 16) & 0xFF) * 4]);
       y ^= *((u32 *)&keybuf[0x848 + ((z >> 8)  & 0xFF) * 4]);
       y += *((u32 *)&keybuf[0xC48 + (z & 0xFF) * 4]);
       y = x ^ y;
       x = *((u32 *)&keybuf[i*4]) ^ z;
       z = y;
   }
   ptr[0] = y;
   ptr[1] = x;
}
//---------------------------------------------------------------------------
void crypt_64bit_up(u32 *ptr)
{
   u32 x,y,z;
   int i;

   y = ptr[0];
   x = ptr[1];//a0869a82
   for(i=0;i<16;i++){
       z = *((u32 *)&keybuf[i*4]) ^ x;//0xFFA64F1B
       x =  *((u32 *)&keybuf[0x048 + ((z >> 24) & 0xFF) * 4]);
       x += *((u32 *)&keybuf[0x448 + ((z >> 16) & 0xFF) * 4]);
       x ^= *((u32 *)&keybuf[0x848 + ((z >> 8)  & 0xFF) * 4]);
       x += *((u32 *)&keybuf[0xC48 + (z & 0xFF) * 4]);
       x = y ^ x; //2d8dac1
       y = z;
   }
   ptr[0] = x ^ *((u32 *)&keybuf[0x40]); //1e884cb9   be295f4d
   ptr[1] = y ^ *((u32 *)&keybuf[0x44]); //28b24b79   a0223d3d
}
//---------------------------------------------------------------------------
void crypt_64bit_down(u32 *ptr)
{
   u32 x,y,z;
   int i;

   y = ptr[0];
   x = ptr[1];
   for(i=17;i >= 2;i--){
       z = keybuf[i*4] ^ x;
       x = keybuf[0x48  + ((z >> 24) & 0xFF) * 4];
       x += keybuf[0x448 + ((z >> 16) & 0xFF) * 4];
       x ^= keybuf[0x848 + ((z >> 8) & 0xFF) * 4];
       x += keybuf[0xc48 + (z & 0xFF) * 4];
       x = y ^ x;
       y = z;
   }
   ptr[0] = x ^ *((u32 *)&keybuf[0x4]);
   ptr[1] = y ^ *((u32 *)&keybuf[0x0]);
}
//---------------------------------------------------------------------------
void apply_keycode(u32 modulo)
{
   int i;
   u32 m;
	u64 value;

   crypt_64bit_up(&keycode[1]);
   crypt_64bit_up(keycode);//3758b7b7 86a7d19e e8ec4a4a
	for(i=0;i<=0x44;i+=4){
        m = keycode[(i % modulo) / 4]; //5220d599 ^ b7b75837
        m = ((m & 0xFF) << 24) | ((m & 0xFF00) << 8) | ((m & 0xFF0000) >> 8) | (m >> 24);
        *((u32 *)&keybuf[i]) ^= m;
	}//E8978DAE 2724e3d1
	value = 0;
	for(i=0;i<0x1040;i+=8){
        crypt_64bit_up((u32 *)&value);//25b629ad66c1c770
        *((u32 *)&keybuf[i]) = (u32)(value >> 32);
        *((u32 *)&keybuf[i+4]) = (u32)value;
	}//BB678e2b
}//49f69ecd
//---------------------------------------------------------------------------
void init_keycode(u32 idcode,u32 level,u32 modulo)
{
   memcpy(keybuf,bios7_mem+0x30,0x1048);
   keycode[0] = idcode;
   keycode[1] = idcode/2;
   keycode[2] = idcode * 2;
   if(level >= 1)
       apply_keycode(modulo);
   if(level >= 2)
       apply_keycode(modulo);
   keycode[1] *= 2;
   keycode[2] /= 2;
   if(level >= 3)
		apply_keycode(modulo);
/*   value = 0x2847928759275943;
	fp = fopen("f:\\crypt.txt","wb");
   crypt_64bit_up((u32 *)&value);
   fprintf(fp,"\r\n\r\n");
   fflush(fp);
   decrypt_64bit_up((u32 *)&value);
   fclose(fp);*/
}
//---------------------------------------------------------------------------
static u32 * decryptFirmwareBlock(const char *blockName, u32 *src, u32 &size)
{
   u32 curBlock[2],*dst,i,j,xIn,xOut,len,offset,windowOffset,xLen,blockSize;
	u8 d;
	u16 data;

/*	xIn = 4;
   xOut = 0;
	memcpy(curBlock, src, 8);
	crypt_64bit_down(curBlock);
	blockSize = (curBlock[0] >> 8);
	size = blockSize;
	dst = (u32*)new u8[blockSize];
	xLen = blockSize;
	while(xLen > 0)
	{
		d = *((u8*)&curBlock[xIn % 8]);
		xIn++;
		if((xIn % 8) == 0)
		{
			memcpy(curBlock, (((u8*)src) + xIn), 8);
			crypt_64bit_down(curBlock);
		}
		for(i = 0; i < 8; i++)
		{
			if(d & 0x80)
			{
				data = *((u8*)&curBlock[xIn % 8]) << 8;
				xIn++;
				if((xIn % 8) == 0)
				{
					memcpy(curBlock, (((u8*)src) + xIn), 8);
					crypt_64bit_down(curBlock);
				}
				data |= *((u8*)&curBlock[xIn % 8]);
				xIn++;
				if((xIn % 8) == 0)
				{
					memcpy(curBlock, (((u8*)src) + xIn), 8);
					crypt_64bit_down(curBlock);
				}

				len = (data >> 12) + 3;
				offset = (data & 0xFFF);
				windowOffset = (xOut - offset - 1);

				for(j = 0; j < len; j++)
				{
					T1WriteByte((u8*)dst, xOut, T1ReadByte((u8*)dst, windowOffset));
					xOut++;
					windowOffset++;

					xLen--;
					if(xLen == 0)
						goto lz77End;
				}
			}
			else
			{
				T1WriteByte((u8*)dst, xOut, T1ReadByte((u8*)curBlock, (xIn % 8)));
				xOut++;
				xIn++;
				if((xIn % 8) == 0)
				{
					memcpy(curBlock,(((u8*)src) + xIn), 8);
					crypt_64bit_down(curBlock);
				}
				xLen--;
				if(xLen == 0)
					goto lz77End;
			}
			d = ((d << 1) & 0xFF);
		}
	}
lz77End:      */
	return dst;
}
//---------------------------------------------------------------------------
BOOL decryptFirmware()
{
   u16 shifts;
	u16 shift1, shift2, shift3, shift4;
	u32 part1addr, part2addr, part3addr, part4addr, part5addr;
	u32 part1ram, part2ram;

	shifts = *((u16 *)&fw_mem[0x14]);
	shift1 = (u16)(shifts & 0x7);
	shift2 = (u16)((shifts >> 3) & 0x7);
	shift3 = (u16)((shifts >> 6) & 0x7);
	shift4 = (u16)((shifts >> 9) & 0x7);

	part1addr = *((u16 *)&fw_mem[0x0C]);
	part1addr = (part1addr << (2+shift1));

	part1ram = *((u16 *)&fw_mem[0xE]);
	part1ram = (0x02800000 - (part1ram << (2+shift2)));

	part2addr = *((u16 *)&fw_mem[0x10]);
	part2addr = (part2addr << (2+shift3));

	part2ram = *((u16 *)&fw_mem[0x12]);
	part2ram = (0x03810000 - (part2ram << (2+shift4)));

	part3addr = *((u16 *)&fw_mem[0]);
	part3addr = (part3addr << 3);

	part4addr = *((u16 *)&fw_mem[0x2]);
	part4addr = (part4addr << 3);

	part5addr = *((u16 *)&fw_mem[0x16]);
	part5addr = (part5addr << 3);

	init_keycode(*((u32 *)&fw_mem[0x8]), 1, 0xC);

	crypt_64bit_down((u32*)&fw_mem[0x18]);
	init_keycode(*((u32 *)&fw_mem[0x08]), 2, 0xC);

/*	nds.FW_ARM9BootCode = (u8*)decryptFirmwareBlock("ARM9 boot code", (u32*)&data[part1addr], nds.FW_ARM9BootCodeSize);
	nds.FW_ARM7BootCode = (u8*)decryptFirmwareBlock("ARM7 boot code", (u32*)&data[part2addr], nds.FW_ARM7BootCodeSize);

	nds.FW_ARM9BootCodeAddr = part1ram;
	nds.FW_ARM7BootCodeAddr = part2ram;

	u16 crc16_header = T1ReadWord(data, 0x06);
	u16 crc16_mine = getBootCodeCRC16();
	if(crc16_header != crc16_mine)
	{
		return FALSE;
	}*/
	return TRUE;
}
