#include "ideastypes.h"
#include "fstream.h"

#ifndef _LZARIH_
#define _LZARIH_

#define N        4096
#define F          60
#define THRESHOLD   2
#define NIL         N
#define M           15
#define Q1          (1UL << M)
#define Q2          (2 * Q1)
#define Q3          (3 * Q1)
#define Q4          (4 * Q1)
#define MAX_CUM     (Q1 - 1)
#define N_CHAR      (256 - THRESHOLD + F)

//---------------------------------------------------------------------------
class LZ
{
public:
   LZ();
   ~LZ();
   BOOL ScompattaFile(HMODULE hModule,WORD wName,LPCTSTR lpType,LStream *pFile);
   BOOL ScompattaFile(char *source,int size,LStream *pFile);
   BOOL ScompattaFile(char *source,int size,char *dst);   
protected:
   int GetBit();
   void StartModel();
   void UpdateModel(int sym);
   int BinarySearch(unsigned int *table,int max,unsigned int x);
   int DecodeChar();
   int DecodePosition();
//---------------------------------------------------------------------------
   unsigned int char_buffer,char_mask;
   unsigned long int low,high,value,bFineFile;
   int *char_to_sym, *sym_to_char,shifts,iFileSize;
   unsigned int *sym_freq,*sym_cum,*position_cum;
   LStream *pFileCps;
};
//---------------------------------------------------------------------------
#endif

