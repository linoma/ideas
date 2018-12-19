#include "ideastypes.h"
#include "lzari.h"
                                  
//---------------------------------------------------------------------------
LZ::LZ()
{
   pFileCps = NULL;
}
//---------------------------------------------------------------------------
LZ::~LZ()
{
   if(pFileCps != NULL)
       delete pFileCps;
}
//---------------------------------------------------------------------------
int LZ::GetBit()
{
   if((char_mask >>= 1) == 0) {
       if(pFileCps->Read(&char_buffer,1) != 1)
           char_buffer = (unsigned int)-1;
       iFileSize--;
       char_mask = 128;
   }
   return ((char_buffer & char_mask) != 0);
}
//---------------------------------------------------------------------------
void LZ::StartModel()
{
   int ch, sym, i;

   sym_cum[N_CHAR] = 0;
   for(sym = N_CHAR; sym >= 1; sym--) {
       ch = sym - 1;
       char_to_sym[ch] = sym;
		sym_to_char[sym] = ch;
       sym_freq[sym] = 1;
       sym_cum[sym - 1] = sym_cum[sym] + sym_freq[sym];
   }
   sym_freq[0] = 0;
   position_cum[N] = 0;
   for(i = N; i >= 1; i--)
	    position_cum[i - 1] = position_cum[i] + 10000 / (i + 200);
}
//---------------------------------------------------------------------------
void LZ::UpdateModel(int sym)
{
   int i, c, ch_i, ch_sym;

   if(sym_cum[0] >= MAX_CUM){
       c = 0;
       for(i = N_CHAR;i > 0;i--){
           sym_cum[i] = c;
           c += (sym_freq[i] = (sym_freq[i] + 1) >> 1);
       }
       sym_cum[0] = c;
   }
   for(i = sym;sym_freq[i] == sym_freq[i - 1];i--);
   if(i < sym){
       ch_i                = sym_to_char[i];
       ch_sym              = sym_to_char[sym];
       sym_to_char[i]      = ch_sym;
       sym_to_char[sym]    = ch_i;
       char_to_sym[ch_i]   = sym;
       char_to_sym[ch_sym] = i;
   }
   sym_freq[i]++;
   while(--i >= 0)
       sym_cum[i]++;
}
//---------------------------------------------------------------------------
int LZ::BinarySearch(unsigned int *table,int max,unsigned int x)
{
   int i, k;

   i = 1;
   while(i < max) {
       k = (max + i) >> 1;
       if(table[k] > x)
           i = k + 1;
       else
			max = k;
   }
   return i;
}
//---------------------------------------------------------------------------
int LZ::DecodeChar()
{
   int  sym, ch;
   unsigned long int  range;

   range   = high - low;
   sym     = BinarySearch(sym_cum,N_CHAR,(unsigned int)(((value - low + 1) * sym_cum[0] - 1) / range));
   high    = low + (range * sym_cum[sym - 1]) / sym_cum[0];
   low     += (range * sym_cum[sym]) / sym_cum[0];
   for(;;){
       if(low >= Q2) {
           value   -= Q2;
           low     -= Q2;
           high    -= Q2;
       }
       else if(low >= Q1 && high <= Q3){
           value   -= Q1;
           low     -= Q1;
           high    -= Q1;
       }
       else if (high > Q2)
           break;
       low     += low;
       high    += high;
       value   = 2 * value + GetBit();
   }
   ch = sym_to_char[sym];
   UpdateModel(sym);
   return ch;
}
//---------------------------------------------------------------------------
int LZ::DecodePosition()
{
   int position;
   unsigned long int  range;

   range       = high - low;
   position    = BinarySearch(position_cum,N,(unsigned int)(((value - low + 1) * position_cum[0] - 1) / range))-1;
   high        = low + (range * position_cum[position]) / position_cum[0];
   low         += (range * position_cum[position + 1]) / position_cum[0];
   for(;;){
       if(low >= Q2){
           value   -= Q2;
			low     -= Q2;
           high    -= Q2;
       }
       else if(low >= Q1 && high <= Q3){
           value   -= Q1;
           low     -= Q1;
           high    -= Q1;
       }
       else if(high > Q2)
			break;
       low     += low;
       high    += high;
       value   = 2 * value + GetBit();
   }
   return position;
}
//---------------------------------------------------------------------------
BOOL LZ::ScompattaFile(char *source,int size,LStream *pFile)
{
   char *pBuffer;
   USHORT i;
   ULONG i1,i2,i3;
   int c;
   BOOL res = FALSE;

   if(pFileCps != NULL)
       delete pFileCps;
   if((pFileCps = new LMemoryFile()) == NULL)
       return FALSE;
   if(!pFile->IsOpen())
       return FALSE;
   pBuffer = (char *)GlobalAlloc(GPTR,N + F + 1 + (N_CHAR * 4 + 4 + N) * sizeof(int));
   if(pBuffer == NULL)
       goto Ex_ScompattaFile_1;
   char_to_sym     = (int *)(pBuffer + N + F + 1);
   sym_to_char     = (int *)(char_to_sym + N_CHAR);
   sym_freq        = (unsigned int *)sym_to_char + N_CHAR + 1;
   sym_cum         = (unsigned int *)sym_freq + N_CHAR + 1;
   position_cum    = (unsigned int*)sym_cum + N_CHAR + 1;

   iFileSize = size;
   pFileCps->Open();
   pFileCps->Write(source,iFileSize);
   pFileCps->SeekToBegin();
   pFile->SeekToBegin();
   high        = Q4;
   low         = value = shifts = 0;
   char_buffer = char_mask = 0;
   for(i = 0;i < M + 2;i++)
       value = 2 * value + GetBit();
   StartModel();
   i = N - F;
   ::FillMemory(pBuffer,i,32);
   while(iFileSize >= 0){
       c = DecodeChar();
       if (c < 256) {
           pFile->Write(&c,1);
           pBuffer[i++] = (char)c;
           i &= (N - 1);
       }
       else {
           i1 = (i - DecodePosition() - 1) & (N - 1);
           i2 = c - 255 + THRESHOLD;
           for (i3 = 0; i3 < i2;i3++) {
               c = pBuffer[(i1 + i3) & (N - 1)];
               pFile->Write(&c,1);
               pBuffer[i++] = (char)c;
               i &= (N - 1);
           }
       }
   }
Ex_ScompattaFile_1:
   if(pBuffer != NULL)
       GlobalFree((HGLOBAL)pBuffer);
   return res;
}
//---------------------------------------------------------------------------
BOOL LZ::ScompattaFile(char *source,int size,char *dst)
{
   char *pBuffer;
   USHORT i;
   ULONG i1,i2,i3,iPos;
   int c;
   BOOL res = FALSE;

   if(pFileCps != NULL)
       delete pFileCps;
   if((pFileCps = new LMemoryFile()) == NULL)
       return FALSE;
   iPos = 0;
   pBuffer = (char *)GlobalAlloc(GPTR,N + F + 1 + (N_CHAR * 4 + 4 + N) * sizeof(int));
   if(pBuffer == NULL)
       goto Ex_ScompattaFile_1;
   char_to_sym     = (int *)(pBuffer + N + F + 1);
   sym_to_char     = (int *)(char_to_sym + N_CHAR);
   sym_freq        = (unsigned int *)sym_to_char + N_CHAR + 1;
   sym_cum         = (unsigned int *)sym_freq + N_CHAR + 1;
   position_cum    = (unsigned int*)sym_cum + N_CHAR + 1;
   iFileSize = size;
   pFileCps->Open();
   pFileCps->Write(source,iFileSize);
   pFileCps->SeekToBegin();
   high        = Q4;
   low         = value = shifts = 0;
   char_buffer = char_mask = 0;
   for(i = 0;i < M + 2;i++)
       value = 2 * value + GetBit();
   StartModel();
   i = N - F;
   ::FillMemory(pBuffer,i,32);
   while(iFileSize >= 0){
       c = DecodeChar();
       if (c < 256) {
           dst[iPos++] = (char)c;
           pBuffer[i++] = (char)c;
           i &= (N - 1);
       }
       else {
           i1 = (i - DecodePosition() - 1) & (N - 1);
           i2 = c - 255 + THRESHOLD;
           for (i3 = 0; i3 < i2;i3++) {
               c = pBuffer[(i1 + i3) & (N - 1)];
               dst[iPos++] = (char)c;
               pBuffer[i++] = (char)c;
               i &= (N - 1);
           }
       }
   }
Ex_ScompattaFile_1:
   if(pBuffer != NULL)
       GlobalFree((HGLOBAL)pBuffer);
   return res;
}
//---------------------------------------------------------------------------
BOOL LZ::ScompattaFile(HMODULE hModule,WORD wName,LPCTSTR lpType,LStream *pFile)
{
#ifdef __WIN32__
   char *pBuffer;
   USHORT i;
   ULONG i1,i2,i3;
   int c;
   HRSRC hrsc;
   BOOL res = FALSE;
   HGLOBAL h;
   LPVOID memResource;

   if(pFileCps != NULL)
       delete pFileCps;
   if((pFileCps = new LMemoryFile()) == NULL)
       return FALSE;
   if(!pFile->IsOpen())
       return FALSE;
   if((hrsc = FindResource(hModule,MAKEINTRESOURCE(wName),lpType)) == NULL)
       return FALSE;
   pBuffer = (char *)GlobalAlloc(GPTR,N + F + 1 + (N_CHAR * 4 + 4 + N) * sizeof(int));
   if(pBuffer == NULL)
       goto Ex_ScompattaFile_1;
   char_to_sym     = (int *)(pBuffer + N + F + 1);
   sym_to_char     = (int *)(char_to_sym + N_CHAR);
   sym_freq        = (unsigned int *)sym_to_char + N_CHAR + 1;
   sym_cum         = (unsigned int *)sym_freq + N_CHAR + 1;
   position_cum    = (unsigned int*)sym_cum + N_CHAR + 1;

   h = LoadResource(hModule,hrsc);
   if(h == NULL)
       goto Ex_ScompattaFile_1;
   if((memResource = LockResource(h)) == NULL)
       goto Ex_ScompattaFile_1;
   iFileSize = (int)SizeofResource(hModule,hrsc);
   pFileCps->Open();
   pFileCps->Write(&((char *)memResource)[4],iFileSize-4);
   pFileCps->SeekToBegin();
   pFile->SeekToBegin();
   high        = Q4;
   low         = value = shifts = 0;
   char_buffer = char_mask = 0;
   for(i = 0;i < M + 2;i++)
       value = 2 * value + GetBit();
   StartModel();
   i = N - F;
   ::FillMemory(pBuffer,i,32);
   while(iFileSize >= 0){
       c = DecodeChar();
       if (c < 256) {
           pFile->Write(&c,1);
           pBuffer[i++] = (char)c;
           i &= (N - 1);
       }
       else {
           i1 = (i - DecodePosition() - 1) & (N - 1);
           i2 = c - 255 + THRESHOLD;
           for (i3 = 0; i3 < i2;i3++) {
               c = pBuffer[(i1 + i3) & (N - 1)];
               pFile->Write(&c,1);
               pBuffer[i++] = (char)c;
               i &= (N - 1);
           }
       }
   }
Ex_ScompattaFile_1:
   if(pBuffer != NULL)
       GlobalFree((HGLOBAL)pBuffer);
   return res;
#else
	return FALSE;
#endif
}

