//#include <stdio.h>
#include "SRAMalloc.h"

#include "types.h"
#include "time.h"
#include<stdlib.h>


#define LOOP 200
#define TBYTES 7

void TestMalloc();
void TestMallocRand();

int main( void )
{
  
  SRAMInitHeap();
 
  TestMalloc();
  
  //TestMallocRand();
  
  
 
}

//测试内存分配性能
void TestMalloc()
{  
    clock_t start, finish;
    double duration;
  
    start = clock();
  
    INT16U i;
    for(i=0;i<LOOP;i++)
    {
      INT8U * p=(INT8U *)SRAMalloc(TBYTES);
      
      if(p==NULL)
      {
        break;
      }
      SRAMfree(p);
    }
    
    finish = clock();
    duration = (double)(finish - start) / (double)CLOCKS_PER_SEC;
}

//测试随机分配内存，随机回收内存
void TestMallocRand()
{
    INT16U i;
    INT16U k;
    INT16U tag;
    INT8U * listp[300];
    INT8U * listsndp[100];
    for(i=0;i<300;i++)
    {
      k=1+(INT8U)(20.0*rand()/(RAND_MAX+1.0));//1-10随机数
      
      INT8U * p=(INT8U *)SRAMalloc(k);
      listp[i]=p;
    }
    for(i=0;i<300;i++)
    {
      tag=(INT8U)(2.0*rand()/(RAND_MAX+1.0));//0-1随机数
      if(tag==1)
      {
        if(listp[i]!=NULL)
          SRAMfree(listp[i]);
      }
    }
    for(i=0;i<100;i++)
    {
      k=1+(INT8U)(20.0*rand()/(RAND_MAX+1.0));//1-20随机数
      
      INT8U * p=(INT8U *)SRAMalloc(k);
      listsndp[i]=p;
    }
    
    for(i=0;i<300;i++)
    {
      if(listp[i]!=NULL)
        SRAMfree(listp[i]);
      if(i<100)
      {
        if(listsndp[i]!=NULL)
          SRAMfree(listsndp[i]);      
      }
    }
}

