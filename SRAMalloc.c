/*****************************************************************
	内存池管理
******************************************************************
*文件名：		SRAMalloc.c
*处理器名：	        MSP430F149
*编译器名：	        IAR Assembler for MSP430 V3.41A/W32
*公司名：		中科院无锡微系统分中心
*当前版本号：	        v1.0
*copyright? 2007, 中科院无锡微系统分中心 All rights reserved.
*
*作者			时间					备注
*ZhangYong		2009-10-22			
******************************************************************/
////////////////////////头文件引用部分//////////////////////////////
#include "types.h"

////////////////////变量定义和声明部分///////////////////////////

////////////////////小内存参数///////////////////////////
#define INT_SEGMENT_SIZE_MICRO   64                       //初始化分段内存尺寸(不允许奇数)
#define _MAX_HEAP_SIZE_MICRO  1024*13                      //(不允许奇数)
#define _MAX_SEGMENT_SIZE_MICRO   255                     //段允许的最大尺寸（8位最大值,即：1111,1111）
//#define FRAG_THRESHOLD_MICRO   INT_SEGMENT_SIZE_MICRO   //合并内存碎片阈值，回收内存时，判断是否合并用
#define FRAG_THRESHOLD_MICRO   8                          //合并内存碎片阈值，回收内存时，判断是否合并用

////////////////////大内存参数///////////////////////////
#define INT_SEGMENT_SIZE_LARGE   256                      //初始化分段内存尺寸(不允许奇数)
#define _MAX_HEAP_SIZE_LARGE  1024                        //(不允许奇数)
#define _MAX_SEGMENT_SIZE_LARGE   32767                   //段允许的最大尺寸（15位最大值,即：111,1111,1111,1111）
#define FRAG_THRESHOLD_LARGE   INT_SEGMENT_SIZE_LARGE     //合并内存碎片阈值，回收内存时，判断是否合并用

typedef struct _SALLOC
{   
    unsigned count:15;//记录每段内存的尺寸   
    unsigned alloc:1;//记录是否已分配 
    
}SALLOC;



#define SALLOC_HEAD_SIZE sizeof(SALLOC)	  //定义头结点的内存尺寸
#define _RESERVE_BYTE SALLOC_HEAD_SIZE   //内存分段用的保留字节，最少是SALLOC_HEAD_SIZE


static INT8U _uDynamicHeap_micro[_MAX_HEAP_SIZE_MICRO]; //开辟内存空间
static INT8U _uDynamicHeap_large[_MAX_HEAP_SIZE_LARGE];


////////////////////私有函数声明////////////////////////////////////
BOOLEAN _SRAMmerge(SALLOC *  pSegA);
INT8U * MovePtr(INT8U * ptr,INT16U offset);
void InitHeap(INT8U * pMem,INT16U maxheapsize,INT16U intsegsize);

////////////////////函数实体部分////////////////////////////////////

/****************************************************
*	函数名：INT8U *  SRAMalloc( INT8U nBytes)
*	作者：  产品开发组
*	完成时间：2009-10-22
*	函数功能说明：
*		从内存池分配固定大小内存
*	参数：
*		unsigned short nBytes 需要分配的内存字节数。
*	返回值：
*		void 指针，该指针指向分配到的内存；如果没有内存，则返回NULL 
*******************************************************/
void *  SRAMalloc( unsigned short nBytes)
{
    SALLOC *        pHeap;
    
    SALLOC          segHeader;
    INT16U          segLen;
    SALLOC *        temp;
    INT16U          act_nBytes;               //实际请求字节数
    INT16U          current_frag_threshold;   //当前实际碎片尺寸阈值
    
    if ( (nBytes == 0)||                      //如果等于0
      (nBytes>=_MAX_SEGMENT_SIZE_LARGE))      //如果大于最大尺寸（15位所表示的最大数）
    {
        return (0);
    }   
    act_nBytes=(nBytes%2)? (nBytes+1):nBytes;            //防止请求的是奇数内存字节数

    if(act_nBytes<=_MAX_SEGMENT_SIZE_MICRO)   //如果请求小于255，在小内存堆里分配
    {
      pHeap = (SALLOC *)_uDynamicHeap_micro;
      current_frag_threshold=FRAG_THRESHOLD_MICRO;
    }
    else    //请求大于255的，在大内存堆里分配
    {
      pHeap = (SALLOC *)_uDynamicHeap_large;
      current_frag_threshold=FRAG_THRESHOLD_LARGE;
    }
    
    while (1)
    {
        segHeader = *pHeap;
	segLen = segHeader.count ;

        if (segHeader.count == 0)
        {
            return (0);
        }

        if (!(segHeader.alloc))
        {
            if (act_nBytes > segLen)
            {
		if (!(_SRAMmerge(pHeap)))
                {
    		  //移动到下一段  
                  pHeap=(SALLOC *)MovePtr((INT8U *) pHeap,(INT16U)(segHeader.count+SALLOC_HEAD_SIZE));					
                }
            }
            else if (act_nBytes == segLen)
            {
                (*pHeap).alloc = 1;               
                return (pHeap + 1);
            }
            else
            {
                (*pHeap).alloc=1;
                //如果当前段分配后的剩余内存不大于保留字节数，则把剩下的直接分配出去，防止内存碎片
                if((segLen-act_nBytes)<=_RESERVE_BYTE)
                {            
                    return (pHeap + 1);
                }
                //如果当前段尺寸小于碎片尺寸阈值，则不分割，直接分配当前段，防止内存碎片
                if(segLen < current_frag_threshold)
                {            
                    return (pHeap + 1);
                }
                                
                (*pHeap).count=act_nBytes;//设置为已分配，同时改变它的段尺寸
                
                temp = pHeap + 1;

		pHeap=(SALLOC *)MovePtr((INT8U *) pHeap,(INT16U)(act_nBytes + SALLOC_HEAD_SIZE));            
                (*pHeap).count = segLen - (act_nBytes+SALLOC_HEAD_SIZE);               
                return (temp);
            }
        }
        else
        {
            //移动到下一段
	    pHeap=(SALLOC *)MovePtr((INT8U *) pHeap,(INT16U)(segHeader.count+SALLOC_HEAD_SIZE));
        }
    }
}


/****************************************************
*	函数名：void SRAMfree(INT8U *  pSRAM)
*	作者：  产品开发组
*	完成时间：2009-10-22
*	函数功能说明：
*		释放内存
*	参数：
*		INT8U *  pSRAM INT8U类型指针，指向需要释放的内存。
*	返回值：
*		无 
*******************************************************/
void SRAMfree(void *  pSRAM)
{
    INT16U current_frag_threshold;//当前碎片尺寸阈值
    SALLOC * p=(SALLOC *)((INT8U *)pSRAM - SALLOC_HEAD_SIZE);
    (*p).alloc = 0; 
    
    if((*p).count<=_MAX_SEGMENT_SIZE_MICRO) //如果当前回收的段尺寸是大于255的，则属于大内存
    {
      current_frag_threshold=FRAG_THRESHOLD_MICRO;
    }
    else
    {
      current_frag_threshold=FRAG_THRESHOLD_LARGE;
    }
      
    if((*p).count < current_frag_threshold)//如果当前段尺寸小于设置的碎片尺寸阈值，合并内存操作
      _SRAMmerge(p);
    
}

     

/****************************************************
*	函数名：BOOLEAN _SRAMmerge(SALLOC *  pSegA)
*	作者：  产品开发组
*	完成时间：2009-10-22
*	函数功能说明：
*		合并内存池中的内存段
*	参数：
*		SALLOC *  pSegA 指向内存池中某一段的指针。
*	返回值：
*		BOOLEAN  如果合并成功，返回TRUE，如果失败，返回FALSE 
*******************************************************/
BOOLEAN _SRAMmerge(SALLOC *  pSegA)
{
    SALLOC *  pSegB;
    SALLOC uSegA, uSegB;

    pSegB=(SALLOC *)MovePtr((INT8U *) pSegA,(INT16U)((*pSegA).count+SALLOC_HEAD_SIZE));

    uSegA = *pSegA;
    uSegB = *pSegB;

    if (uSegB.count == 0)
    {
        return (FALSE);
    }

    if (uSegA.alloc || uSegB.alloc)
    {
        return (FALSE);
    }
    
    if((uSegA.count + uSegB.count+SALLOC_HEAD_SIZE) >= _MAX_SEGMENT_SIZE_LARGE)
    {
        return (FALSE);
    }
    
    //这里不用判断是不是合并的值大于段尺寸，直接合并成一个大段。
    (*pSegA).count = uSegA.count + uSegB.count+SALLOC_HEAD_SIZE;
    return (TRUE);

}

/****************************************************
*	函数名：void SRAMInitHeap(void)
*	作者：  产品开发组
*	完成时间：2009-10-22
*	函数功能说明：
*		初始化内存池，该函数一定要在分配内存之前调用最少一次。
*	参数：
*		无
*	返回值：
*		无
*******************************************************/
void SRAMInitHeap(void)
{
    //初始化小内存
    InitHeap(_uDynamicHeap_micro,_MAX_HEAP_SIZE_MICRO,INT_SEGMENT_SIZE_MICRO);
    //初始化大内存
    InitHeap(_uDynamicHeap_large,_MAX_HEAP_SIZE_LARGE,INT_SEGMENT_SIZE_LARGE);
    
}

/****************************************************
*	函数名：void InitHeap(INT8U * pMem,INT16U maxheapsize,INT16U intsegsize)
*	作者：  产品开发组
*	完成时间：2009-10-22
*	函数功能说明：
*		初始化内存堆，私有函数
*	参数：
*		INT8U * pMem 内存堆首地址。
*               INT16U maxheapsize 堆尺寸
*               INT16U intsegsize   段尺寸
*	返回值：
*		无 
*******************************************************/
void InitHeap(INT8U * pMem,INT16U maxheapsize,INT16U intsegsize)
{
    INT8U *  pHeap;
    unsigned int count;
    SALLOC * temp;
    
    pHeap = pMem;
    count = maxheapsize;

    while (1)
    {
        if (count > (intsegsize+SALLOC_HEAD_SIZE))
        {            
            temp=(SALLOC *)pHeap;
            (*temp).count=intsegsize;//设置段尺寸（初始化为最大段尺寸）
            pHeap += intsegsize+SALLOC_HEAD_SIZE;//段尺寸加上头结点尺寸
            count = count - (intsegsize+SALLOC_HEAD_SIZE);
        }
        else
        {
            
            //if(count==0)
            //{
            //    *(pHeap-(INT_SEGMENT_SIZE_MICRO+SALLOC_HEAD_SIZE))=INT_SEGMENT_SIZE_MICRO;
            //    return;
            //}
            if(count<=_RESERVE_BYTE)
            {
                *pHeap=0;//设置tail
                return;
            }
            else
            {
                temp=(SALLOC *)pHeap;
                (*temp).count=count-SALLOC_HEAD_SIZE-SALLOC_HEAD_SIZE;//留下一位设置TAIL
                *(pHeap + (count-SALLOC_HEAD_SIZE)) = 0;//设置tail
                return;
            }
        }
    }
    
}

/****************************************************
*	函数名：INT8U * MovePtr(INT8U * ptr,INT16U offset)
*	作者：  产品开发组
*	完成时间：2009-10-22
*	函数功能说明：
*		按照偏移量以字节为单位移动指针
*	参数：
*		INT8U * ptr 需要分移动的指针。
*               INT16U offset 偏移量
*	返回值：
*		INT8U 指针，指向移动到偏移量的地址 
*******************************************************/
INT8U * MovePtr(INT8U * ptr,INT16U offset)
{
    return ptr+offset;
}
