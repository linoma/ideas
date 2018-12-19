#include "ideastypes.h"
#include "3d.h"
#include "dsmem.h"
#include "dsreg.h"

extern float mtxTexture[16];
extern float mtxProjection[16];
extern float mtxView[16];
extern float mtxPosition[16];
//---------------------------------------------------------------------------
stack_MTX::stack_MTX()
{
	mtx = NULL;
   type = 0;
	reset();
}
//---------------------------------------------------------------------------
stack_MTX::~stack_MTX()
{
	if(mtx != NULL)
   	delete []mtx;
   mtx = NULL;
}
//---------------------------------------------------------------------------
void stack_MTX::init(int size)
{
   if((size+1) != maxElem){
	    maxElem = size;
       if(mtx != NULL)
   	    delete []mtx;
       mtx = new float[(size+1)*16];                   
   }
}
//---------------------------------------------------------------------------
void stack_MTX::reset()
{
   int i;

	idxStack = 0;
   if(mtx == NULL)
       return;
   ZeroMemory(mtx,(maxElem+1)*16*sizeof(float));
   for(i=0;i < maxElem+1;i++)
       mtx[(i << 4)] = mtx[(i<<4) + 5] = mtx[(i << 4) + 10] = mtx[(i<<4)+15] = 1;
}
//---------------------------------------------------------------------------
void stack_MTX::store(int index)
{
   switch(type){
       case NTR_MODELVIEW_MATRIX:
	        memcpy(&mtx[(index & maxElem) << 4],mtxView,sizeof(mtxView));
       break;
       case NTR_POSITION_MATRIX:
	        memcpy(&mtx[(index & maxElem) << 4],mtxPosition,sizeof(mtxPosition));
       break;
       case NTR_PROJECTION_MATRIX:
           memcpy(&mtx[(index & maxElem) << 4],mtxProjection,sizeof(mtxProjection));
       break;
       case NTR_TEXTURE_MATRIX:
           memcpy(mtxTexture,&mtx[(index & maxElem) << 4],sizeof(mtxTexture));
       break;
   }
   if(index >= maxElem)
   	GXSTAT |= 0x8000;
}
//---------------------------------------------------------------------------
void stack_MTX::restore(int index)
{
   switch(type){
       case NTR_MODELVIEW_MATRIX:
	        memcpy(mtxView,&mtx[(index & maxElem) << 4],sizeof(mtxView));
       break;
       case NTR_POSITION_MATRIX:
	        memcpy(mtxPosition,&mtx[(index & maxElem) << 4],sizeof(mtxPosition));
       break;
       case NTR_PROJECTION_MATRIX:
           memcpy(mtxProjection,&mtx[(index & maxElem) << 4],sizeof(mtxProjection));
       break;
       case NTR_TEXTURE_MATRIX:
           memcpy(mtxTexture,&mtx[(index & maxElem) << 4],sizeof(mtxTexture));
       break;
   }
   if(index >= maxElem)
   	GXSTAT |= 0x8000;
}
//---------------------------------------------------------------------------
void stack_MTX::push()
{
  	if(idxStack > maxElem)
       idxStack %= (maxElem+1);
   switch(type){
       case NTR_MODELVIEW_MATRIX:
           memcpy(&mtx[idxStack++ << 4],mtxView,sizeof(mtxView));
       break;
       case NTR_POSITION_MATRIX:
	        memcpy(&mtx[idxStack++ << 4],mtxPosition,sizeof(mtxPosition));
       break;
       case NTR_PROJECTION_MATRIX:
           memcpy(&mtx[idxStack++ << 4],mtxProjection,sizeof(mtxProjection));
       break;
       case NTR_TEXTURE_MATRIX:
       	memcpy(&mtx[idxStack++ << 4],mtxTexture,sizeof(mtxTexture));
       break;
   }
}
//---------------------------------------------------------------------------
BOOL stack_MTX::pop(int index)
{
   if(index == 0)            
       index = 1;
   if(abs(index) > idxStack)
       return TRUE;
 	idxStack -= index;
   switch(type){
       case NTR_MODELVIEW_MATRIX:
           memcpy(mtxView,&mtx[idxStack << 4],sizeof(mtxView));
       break;
       case NTR_POSITION_MATRIX:
           memcpy(mtxPosition,&mtx[idxStack << 4],sizeof(mtxPosition));
       break;
       case NTR_PROJECTION_MATRIX:
           memcpy(mtxProjection,&mtx[idxStack << 4],sizeof(mtxProjection));
       break;
       case NTR_TEXTURE_MATRIX:
       	memcpy(mtxTexture,&mtx[idxStack << 4],sizeof(mtxTexture));
       break;
   }
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL stack_MTX::Save(LStream *pFile)
{
   if(mtx != NULL)
       pFile->Write(mtx,(maxElem+1)*16*sizeof(float));
   return TRUE;
}
//---------------------------------------------------------------------------
BOOL stack_MTX::Load(LStream *pFile,int ver)
{
   if(mtx != NULL)
       pFile->Read(mtx,(maxElem+1)*16*sizeof(float));
   return TRUE;
}
//---------------------------------------------------------------------------
void stack_MTX::operator=(stack_MTX &v)
{
   int size;

   if(mtx == NULL || v.get_matrix() == NULL)
       return;
   size = v.get_size() > maxElem ? maxElem : v.get_size();
   memcpy(mtx,v.get_matrix(),size * sizeof(float));
   idxStack = v.get_index();
   if(idxStack > maxElem)
       idxStack = maxElem;
}
//---------------------------------------------------------------------------
void stack_MV::push()
{
	stack_MTX::push();
   GXSTAT &= ~(0x1F << 8);
	GXSTAT |= (idxStack << 8);
   if(idxStack >= maxElem)
   	GXSTAT |= 0x8000;
}
//---------------------------------------------------------------------------
BOOL stack_MV::pop(int index)
{                                              
	BOOL res;

	res = stack_MTX::pop(index);
	GXSTAT &= ~(0x1F << 8);
	GXSTAT |= (idxStack << 8);
   if(idxStack >= maxElem)
   	GXSTAT |= 0x8000;
   return res;
}
//---------------------------------------------------------------------------
void stack_MV::store(int index)
{
	stack_MTX::store(index);
}
//---------------------------------------------------------------------------
void stack_MV::restore(int index)
{
	stack_MTX::restore(index);
}
//---------------------------------------------------------------------------
void stack_PM::push()
{
 	stack_MTX::push();
	GXSTAT |= (1 << 13);
}
//---------------------------------------------------------------------------
BOOL stack_PM::pop(int index)
{
	GXSTAT &= ~(1 << 13);
   return stack_MTX::pop(index);
}
//---------------------------------------------------------------------------
void stack_PM::store(int index)
{
	stack_MTX::store(0);
}
//---------------------------------------------------------------------------
void stack_PM::restore(int index)
{
	stack_MTX::restore(0);
}

