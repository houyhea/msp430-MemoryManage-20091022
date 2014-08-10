/*****************************************************************
	�ڴ�ع���
******************************************************************
*�ļ�����		SRAMalloc.c
*����������	        MSP430F149
*����������	        IAR Assembler for MSP430 V3.41A/W32
*��˾����		�п�Ժ����΢ϵͳ������
*��ǰ�汾�ţ�	        v1.0
*copyright? 2007, �п�Ժ����΢ϵͳ������ All rights reserved.
*
*����			ʱ��					��ע
*ZhangYong		2009-10-22			
******************************************************************/
////////////////////////ͷ�ļ����ò���//////////////////////////////
#include "types.h"

////////////////////�����������������///////////////////////////

////////////////////С�ڴ����///////////////////////////
#define INT_SEGMENT_SIZE_MICRO   64                       //��ʼ���ֶ��ڴ�ߴ�(����������)
#define _MAX_HEAP_SIZE_MICRO  1024*13                      //(����������)
#define _MAX_SEGMENT_SIZE_MICRO   255                     //����������ߴ磨8λ���ֵ,����1111,1111��
//#define FRAG_THRESHOLD_MICRO   INT_SEGMENT_SIZE_MICRO   //�ϲ��ڴ���Ƭ��ֵ�������ڴ�ʱ���ж��Ƿ�ϲ���
#define FRAG_THRESHOLD_MICRO   8                          //�ϲ��ڴ���Ƭ��ֵ�������ڴ�ʱ���ж��Ƿ�ϲ���

////////////////////���ڴ����///////////////////////////
#define INT_SEGMENT_SIZE_LARGE   256                      //��ʼ���ֶ��ڴ�ߴ�(����������)
#define _MAX_HEAP_SIZE_LARGE  1024                        //(����������)
#define _MAX_SEGMENT_SIZE_LARGE   32767                   //����������ߴ磨15λ���ֵ,����111,1111,1111,1111��
#define FRAG_THRESHOLD_LARGE   INT_SEGMENT_SIZE_LARGE     //�ϲ��ڴ���Ƭ��ֵ�������ڴ�ʱ���ж��Ƿ�ϲ���

typedef struct _SALLOC
{   
    unsigned count:15;//��¼ÿ���ڴ�ĳߴ�   
    unsigned alloc:1;//��¼�Ƿ��ѷ��� 
    
}SALLOC;



#define SALLOC_HEAD_SIZE sizeof(SALLOC)	  //����ͷ�����ڴ�ߴ�
#define _RESERVE_BYTE SALLOC_HEAD_SIZE   //�ڴ�ֶ��õı����ֽڣ�������SALLOC_HEAD_SIZE


static INT8U _uDynamicHeap_micro[_MAX_HEAP_SIZE_MICRO]; //�����ڴ�ռ�
static INT8U _uDynamicHeap_large[_MAX_HEAP_SIZE_LARGE];


////////////////////˽�к�������////////////////////////////////////
BOOLEAN _SRAMmerge(SALLOC *  pSegA);
INT8U * MovePtr(INT8U * ptr,INT16U offset);
void InitHeap(INT8U * pMem,INT16U maxheapsize,INT16U intsegsize);

////////////////////����ʵ�岿��////////////////////////////////////

/****************************************************
*	��������INT8U *  SRAMalloc( INT8U nBytes)
*	���ߣ�  ��Ʒ������
*	���ʱ�䣺2009-10-22
*	��������˵����
*		���ڴ�ط���̶���С�ڴ�
*	������
*		unsigned short nBytes ��Ҫ������ڴ��ֽ�����
*	����ֵ��
*		void ָ�룬��ָ��ָ����䵽���ڴ棻���û���ڴ棬�򷵻�NULL 
*******************************************************/
void *  SRAMalloc( unsigned short nBytes)
{
    SALLOC *        pHeap;
    
    SALLOC          segHeader;
    INT16U          segLen;
    SALLOC *        temp;
    INT16U          act_nBytes;               //ʵ�������ֽ���
    INT16U          current_frag_threshold;   //��ǰʵ����Ƭ�ߴ���ֵ
    
    if ( (nBytes == 0)||                      //�������0
      (nBytes>=_MAX_SEGMENT_SIZE_LARGE))      //����������ߴ磨15λ����ʾ���������
    {
        return (0);
    }   
    act_nBytes=(nBytes%2)? (nBytes+1):nBytes;            //��ֹ������������ڴ��ֽ���

    if(act_nBytes<=_MAX_SEGMENT_SIZE_MICRO)   //�������С��255����С�ڴ�������
    {
      pHeap = (SALLOC *)_uDynamicHeap_micro;
      current_frag_threshold=FRAG_THRESHOLD_MICRO;
    }
    else    //�������255�ģ��ڴ��ڴ�������
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
    		  //�ƶ�����һ��  
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
                //�����ǰ�η�����ʣ���ڴ治���ڱ����ֽ��������ʣ�µ�ֱ�ӷ����ȥ����ֹ�ڴ���Ƭ
                if((segLen-act_nBytes)<=_RESERVE_BYTE)
                {            
                    return (pHeap + 1);
                }
                //�����ǰ�γߴ�С����Ƭ�ߴ���ֵ���򲻷ָֱ�ӷ��䵱ǰ�Σ���ֹ�ڴ���Ƭ
                if(segLen < current_frag_threshold)
                {            
                    return (pHeap + 1);
                }
                                
                (*pHeap).count=act_nBytes;//����Ϊ�ѷ��䣬ͬʱ�ı����Ķγߴ�
                
                temp = pHeap + 1;

		pHeap=(SALLOC *)MovePtr((INT8U *) pHeap,(INT16U)(act_nBytes + SALLOC_HEAD_SIZE));            
                (*pHeap).count = segLen - (act_nBytes+SALLOC_HEAD_SIZE);               
                return (temp);
            }
        }
        else
        {
            //�ƶ�����һ��
	    pHeap=(SALLOC *)MovePtr((INT8U *) pHeap,(INT16U)(segHeader.count+SALLOC_HEAD_SIZE));
        }
    }
}


/****************************************************
*	��������void SRAMfree(INT8U *  pSRAM)
*	���ߣ�  ��Ʒ������
*	���ʱ�䣺2009-10-22
*	��������˵����
*		�ͷ��ڴ�
*	������
*		INT8U *  pSRAM INT8U����ָ�룬ָ����Ҫ�ͷŵ��ڴ档
*	����ֵ��
*		�� 
*******************************************************/
void SRAMfree(void *  pSRAM)
{
    INT16U current_frag_threshold;//��ǰ��Ƭ�ߴ���ֵ
    SALLOC * p=(SALLOC *)((INT8U *)pSRAM - SALLOC_HEAD_SIZE);
    (*p).alloc = 0; 
    
    if((*p).count<=_MAX_SEGMENT_SIZE_MICRO) //�����ǰ���յĶγߴ��Ǵ���255�ģ������ڴ��ڴ�
    {
      current_frag_threshold=FRAG_THRESHOLD_MICRO;
    }
    else
    {
      current_frag_threshold=FRAG_THRESHOLD_LARGE;
    }
      
    if((*p).count < current_frag_threshold)//�����ǰ�γߴ�С�����õ���Ƭ�ߴ���ֵ���ϲ��ڴ����
      _SRAMmerge(p);
    
}

     

/****************************************************
*	��������BOOLEAN _SRAMmerge(SALLOC *  pSegA)
*	���ߣ�  ��Ʒ������
*	���ʱ�䣺2009-10-22
*	��������˵����
*		�ϲ��ڴ���е��ڴ��
*	������
*		SALLOC *  pSegA ָ���ڴ����ĳһ�ε�ָ�롣
*	����ֵ��
*		BOOLEAN  ����ϲ��ɹ�������TRUE�����ʧ�ܣ�����FALSE 
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
    
    //���ﲻ���ж��ǲ��Ǻϲ���ֵ���ڶγߴ磬ֱ�Ӻϲ���һ����Ρ�
    (*pSegA).count = uSegA.count + uSegB.count+SALLOC_HEAD_SIZE;
    return (TRUE);

}

/****************************************************
*	��������void SRAMInitHeap(void)
*	���ߣ�  ��Ʒ������
*	���ʱ�䣺2009-10-22
*	��������˵����
*		��ʼ���ڴ�أ��ú���һ��Ҫ�ڷ����ڴ�֮ǰ��������һ�Ρ�
*	������
*		��
*	����ֵ��
*		��
*******************************************************/
void SRAMInitHeap(void)
{
    //��ʼ��С�ڴ�
    InitHeap(_uDynamicHeap_micro,_MAX_HEAP_SIZE_MICRO,INT_SEGMENT_SIZE_MICRO);
    //��ʼ�����ڴ�
    InitHeap(_uDynamicHeap_large,_MAX_HEAP_SIZE_LARGE,INT_SEGMENT_SIZE_LARGE);
    
}

/****************************************************
*	��������void InitHeap(INT8U * pMem,INT16U maxheapsize,INT16U intsegsize)
*	���ߣ�  ��Ʒ������
*	���ʱ�䣺2009-10-22
*	��������˵����
*		��ʼ���ڴ�ѣ�˽�к���
*	������
*		INT8U * pMem �ڴ���׵�ַ��
*               INT16U maxheapsize �ѳߴ�
*               INT16U intsegsize   �γߴ�
*	����ֵ��
*		�� 
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
            (*temp).count=intsegsize;//���öγߴ磨��ʼ��Ϊ���γߴ磩
            pHeap += intsegsize+SALLOC_HEAD_SIZE;//�γߴ����ͷ���ߴ�
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
                *pHeap=0;//����tail
                return;
            }
            else
            {
                temp=(SALLOC *)pHeap;
                (*temp).count=count-SALLOC_HEAD_SIZE-SALLOC_HEAD_SIZE;//����һλ����TAIL
                *(pHeap + (count-SALLOC_HEAD_SIZE)) = 0;//����tail
                return;
            }
        }
    }
    
}

/****************************************************
*	��������INT8U * MovePtr(INT8U * ptr,INT16U offset)
*	���ߣ�  ��Ʒ������
*	���ʱ�䣺2009-10-22
*	��������˵����
*		����ƫ�������ֽ�Ϊ��λ�ƶ�ָ��
*	������
*		INT8U * ptr ��Ҫ���ƶ���ָ�롣
*               INT16U offset ƫ����
*	����ֵ��
*		INT8U ָ�룬ָ���ƶ���ƫ�����ĵ�ַ 
*******************************************************/
INT8U * MovePtr(INT8U * ptr,INT16U offset)
{
    return ptr+offset;
}
