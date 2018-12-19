#include "ideastypes.h"
//#include <vector>
                                                 
//---------------------------------------------------------------------------
#ifndef __lvecH__
#define __lvecH__
//---------------------------------------------------------------------------
template <class T> class LVector
{
public:
   LVector(){
	    data = NULL;
       size = 0;
       last = 0;
   };
   ~LVector(){clear();};
   inline T& operator[](unsigned long index){
       return items(index);
   };
	T& operator=(const T &C){
   	if(this == &C) return *this;
       *this = C;
       return *this;
   };
   void clear(){
       if(data != NULL)
           LocalFree(data);
       data = NULL;
       size = 0;
       last = 0;
   };
   inline void free(){
   	last = 0;
   }
   void push(const T& x){
       insert(last + 1,x);
   };
  	T& pop(){return data[--last];};
 	void set_start(unsigned long index){
   	if(index < last){
           CopyMemory(data,&data[index-1],(last - index) * sizeof(T));
           last -= index;
       }
       else
       	free();
   }
	/*T& popfirst()
	{
   		T c;

       	c = data[0];
       	CopyMemory(data,&data[1],sizeof(T)*--last);
   		return c;
   };*/
   inline T& items(unsigned long index){
  	    return data[index-1];
   }
   inline unsigned long count(){return last;};
   inline T *buffer(){return data;};
//   inline unsigned long size(){return size;};
   bool operator!=(const LVector &v){
       if(v.count() != last)
           return true;
       if(memcmp(data,v.data,last*sizeof(T)) != 0)
           return true;
       return false;
   }
	void operator=(const LVector &v){
		unsigned long len;

       free();
       len = v.count();
       if(!alloc_buffer(len+1))
       	return;
		memcpy(data,v.data,len*sizeof(T));
       last = len;
   }
protected:
   bool insert(unsigned long index,const T& x){
       if(!alloc_buffer(index+1))
       	return false;
       data[last++] = x;
       return true;
   };
   bool alloc_buffer(unsigned long len){
       T* old;
		unsigned long oldsize,i;

		if(len < size)
       	return true;
       old = data;
      	oldsize = size;
      	i = ((len >> 10) + 1) << 10;
       data = (T*)LocalAlloc(LPTR,sizeof(T) * i);
       if(data == NULL)
       	return false;
       if(oldsize)
       	memcpy(data,old,oldsize * sizeof(T));
		if(old)
       	LocalFree(old);
       size = i;
		return true;
   }
   T *data;
   unsigned long last,size;
};

#endif
