/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "16307130015",
    /* First member's full name */
    "WANG YINGMING",
    /* First member's email address */
    "16307130015@fudan.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void *place(void *bp, size_t asize);
static int mm_check(void);
static int checkblock(void *bp);

#define ALIGNMENT 8   //����Ҫ������Ϊ˫�ֶ���

#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)   
//��һ���洢��С����

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//�����ĳ����ͺ궨��
#define WSIZE		4	// һ���ֵĳ��ȣ��ֽ�Ϊ��λ��
#define DSIZE		8	//˫�ֳ��ȣ��ֽ�Ϊ��λ
#define CHUNKSIZE	(1 << 12)	// ��ʼ���ѵĴ�СΪ4KB 
#define OVERHEAD	8	//�ѷ���Ŀ�ֻ����ͷ�� 

#define MAXN	10
#define MAX(x, y)	((x) > (y) ? (x) : (y))

#define PACK(size, alloc)	((size)|(alloc))  
// ��һ����Ĵ�С�������Ƿ���䣬ǰһ���Ƿ�����װ��һ������

//��һ���ֵĶ�д�������޸�һ���ֵ�ͷ����β����ǰ��ָ��
#define GET(p)	(*(size_t *)(p))    
#define PUT(p, val)	(*(size_t *)(p) = (val))  

#define GET_SIZE(p)	(GET(p) & ~0x7)  // ����Ĵ�С 
#define GET_ALLOC(p)	(GET(p) & 0x1)  //��÷�����ֽ�

//ͨ����ָ�������ͷ����β��
#define HDRP(bp)	((char *)(bp) - WSIZE)   
#define FTRP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

//ͨ����ָ�룬����ǰһ��ͺ�һ��Ŀ�ָ��
#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))  
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))  

// ����������ʹ��һ����һ��˽��ȫ�ֱ��� 
//ָ���ʼ��
static char *heap_listp;
int counter = 0;	//ȫ�ּ�����

//˫������
static char **head;
static char **foot;
static char * valuePtr(char *ptr);
static void putPtr(char *ptr, char *newPtr);
static void addfreelist(char *bp);
static void deletefreelist(char *bp);
static int range(size_t size);
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)   
{
	// ��ʼ����Ϊ������MAXN�ֳ��Ŀռ䣬һ����Ϊ���Կ飬һ����Ϊ��β 
	head = mem_sbrk(MAXN*WSIZE);
	if(head == NULL)
		return -1;
	int i;
	for(i = 0; i < MAXN; i++)
		head[i] = NULL;
	foot = mem_sbrk(MAXN*WSIZE);
	if(foot == NULL)
		return -1;
	for(i = 0; i < MAXN; i++)
		foot[i] = NULL;

	//���û�гɹ����뵽��ַ����ʼ��ʧ�ܣ�����-1
	if((heap_listp = mem_sbrk(4*WSIZE)) == NULL)
		return -1;
	PUT(heap_listp, 0); 	//�������
	PUT(heap_listp+WSIZE, PACK(OVERHEAD, 1));	// ���Կ�ͷ��
	PUT(heap_listp+DSIZE, PACK(OVERHEAD, 1));	// ���Կ�β��
	PUT(heap_listp+WSIZE+DSIZE, PACK(0, 1));	// ��β��ͷ�� 
	heap_listp += DSIZE;
	// �ó�ʼ���ѵĴ�С��չ���п�
	if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0;
}

static void *extend_heap(size_t words)   
	//����ʼ�������Ҳ������ʵ�ʱ��չ�ѵô�С 
{
	char *bp;
	size_t size;
	// ����һ��ż�����ֶ���
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if((int)(bp = mem_sbrk(size)) == -1)
		return NULL;
	// ��ʼ�����п�ͷβ�Լ���β��ͷ��
	PUT(HDRP(bp), PACK(size, 0));	// ���п�ͷ 
	PUT(FTRP(bp), PACK(size, 0));	// ���п�β 
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));	//�µ���Ļ�� 
	// ���֮ǰ�Ŀ��ǿյ���ϲ� 
	return coalesce(bp);
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)  
{
	size_t asize;	// �����Ŀ��С 
	size_t extendsize;	// ��������ʵĻ�������չ��С 
	char *bp;
	// ����������� 
	if(size <= 0)
		return NULL;
	// ������Ĵ�Сʹ����� 
	if(size <= DSIZE)
		asize = DSIZE + OVERHEAD;
	else
		asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);
	// Ѱ�Һ��ʵĿ��п� 
	if((bp = find_fit(asize)) != NULL){
		return place(bp, asize);	
	}
	// �Ҳ������ʵĿ��п�����չ�洢�ռ�
	extendsize = MAX(asize, CHUNKSIZE);
	size_t firstsize = 16;
	if((bp = extend_heap(firstsize/WSIZE)) == NULL)
		return NULL;
	if(GET_SIZE(HDRP(bp)) >= asize)
	{
		return place(bp, asize);
	}
	if((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	return place(bp, asize);
}

static void *find_fit(size_t asize){                          
	void *bp;
	// ѡ���б� 
	int i = range(asize);
	// ����ʵ����� 
	for(; i < MAXN; i++)
	{   
		// �ӵ͵�ַ���ߵ�ַ�����б� 
		for(bp = foot[i]; bp != NULL; bp = valuePtr(bp))
		{ //����һ�����п���ͬһ���б�
			if(asize <= GET_SIZE(HDRP(bp)))
				return bp;
		}
	}
	return NULL; //���ʺ��򷵻�
}

static void *place(void *bp, size_t asize){
	size_t csize = GET_SIZE(HDRP(bp));
	counter++;
	void *ret;
	deletefreelist(bp);	//�ӿ����б���ɾ����ǰ�� 
	if((csize - asize) >= (DSIZE + OVERHEAD)){		
		if(counter % 2 == 1){
			ret = bp;		
			PUT(HDRP(bp), PACK(asize, 1));
			PUT(FTRP(bp), PACK(asize, 1));
			bp = NEXT_BLKP(bp);
			PUT(HDRP(bp), PACK(csize-asize, 0));
			PUT(FTRP(bp), PACK(csize-asize, 0));
			addfreelist(bp);	// ���뵽���п��� 
		}
		else{
			PUT(HDRP(bp), PACK(csize-asize, 0));
			PUT(FTRP(bp), PACK(csize-asize, 0));
			addfreelist(bp);	// ���뵽���п��� 
			ret = NEXT_BLKP(bp);
			PUT(HDRP(ret), PACK(asize, 1));
			PUT(FTRP(ret), PACK(asize, 1));
		}
	}
	else{
		ret = bp;
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
	return ret;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	size_t size = GET_SIZE(HDRP(ptr));
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	coalesce(ptr);
}


static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	// ��Ӻ�ɾ����Ŀ 
	//ǰ���ѷ��䲻��Ҫ�ϲ���û�в���
	if(prev_alloc && next_alloc) {                 
		addfreelist((char *)bp);
		return bp;
	}
	// ǰ���ѷ��䣬������У��������ϲ�
	else if(prev_alloc && !next_alloc) {              	
		deletefreelist((char *)NEXT_BLKP(bp));
		deletefreelist((char *)bp);
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
		addfreelist((char *)bp);
		return (bp);
	}
	// ǰ����У������ѷ��䣬��ǰ����ϲ� 
	else if(!prev_alloc && next_alloc) {            	
		deletefreelist((char *)PREV_BLKP(bp));
		deletefreelist((char *)bp);
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		addfreelist((char *)PREV_BLKP(bp));
		return (PREV_BLKP(bp));
	}
	// ǰ�󶼿��У���ǰ��ϲ�
	else {	                                           
		deletefreelist((char *)NEXT_BLKP(bp));
		deletefreelist((char *)PREV_BLKP(bp));
		deletefreelist((char *)bp);
		size+=GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		addfreelist((char *)PREV_BLKP(bp));
		return (PREV_BLKP(bp));
	}
}
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)  
{
	if(ptr == NULL)	//ָ��Ϊ��
		return mm_malloc(size);
	else if(size == 0)   // ��СΪ0
	{ mm_free(ptr);
	return NULL;
	}
	//���������
	size_t curr_size = GET_SIZE(HDRP(ptr));   // ��ǰ��Ĵ�С
	size_t asize;
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
	size_t prev_size = GET_SIZE(FTRP(PREV_BLKP(ptr)));
	size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));

	void *newptr;     
	//������ǰ��Ĵ�Сʹ������ռ�Ͷ��� 
	if(size <= DSIZE)
		asize = DSIZE + OVERHEAD;
	else
		asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);
	// ���������� 
	if(prev_alloc && next_alloc && asize <= curr_size)	
	{
		ptr = place(ptr, asize);
		return ptr;
	}
	//��һ�����У�����������
	else if(prev_alloc && !next_alloc && asize <= (curr_size + next_size))
	{
		deletefreelist(NEXT_BLKP(ptr));
		PUT(HDRP(ptr), PACK(curr_size + next_size, 1));
		PUT(FTRP(ptr), PACK(curr_size + next_size, 1));
		ptr = place(ptr, asize);
		return ptr;
	}
	else if(!prev_alloc && next_alloc && asize <= (curr_size + prev_size))
	{
		newptr = PREV_BLKP(ptr);
		deletefreelist(newptr);
		PUT(HDRP(newptr), PACK(curr_size + prev_size, 1));
		PUT(FTRP(newptr), PACK(curr_size + prev_size, 1));
		memcpy(newptr, ptr, curr_size-OVERHEAD);     // ��ԭ�ȵı������Ƶ��µĵ�ַ
		newptr = place(newptr, asize);
		return newptr;
	}
	//���ǿ���״̬ 
	else if(!prev_alloc && !next_alloc && asize <= (curr_size + prev_size + next_size))
	{
		newptr = PREV_BLKP(ptr);
		deletefreelist(newptr);
		deletefreelist(NEXT_BLKP(ptr));
		PUT(HDRP(newptr), PACK(curr_size + prev_size + next_size, 1));
		PUT(FTRP(newptr), PACK(curr_size + prev_size + next_size, 1));
		memcpy(newptr, ptr, curr_size-OVERHEAD);   
		//�Ѿɵ�ֵ���Ƶ��µĵط� 		
		newptr = place(newptr, asize);
		return newptr;
	}
	else
	{
		newptr = mm_malloc(size);
		memcpy(newptr, ptr, curr_size-OVERHEAD);
		mm_free(ptr);
		return newptr;
	}
}
/*******************************************************/
static char *valuePtr(char *ptr)   // �б��з�����һ�����п�ָ��
{
	return *(char **)ptr;
}
/********************************************************/
static void putPtr(char *ptr, char *newPtr)  // ��ָ���赽ϴ��ָ���� 
{
	*(char **)ptr = newPtr;
}
/******************************************************/

static void addfreelist(char *bp)   // ���б�������һ�����п�
{
	int i = range(GET_SIZE(HDRP(bp)));
	putPtr(bp, NULL);
	// empty list 
	if(head[i] == NULL)
		foot[i] = bp;
	else
		putPtr(head[i], bp);
	putPtr(bp+WSIZE, head[i]);
	head[i] = bp;
}


static void deletefreelist(char *bp)      // ����Ѿ��������˾�ɾ��һ���� 
{
	int i = range(GET_SIZE(HDRP(bp)));
	//Ѱ��Ҫɾ����Ԫ��
	char *temp = head[i];
	while(temp != NULL)
	{
		if(temp == bp)
			break;
		temp = valuePtr(temp+WSIZE);
	}
	if(temp == NULL)    //�Ҳ���
		return;
	char *prev_bp = valuePtr(bp);   // �ҵ���
	char *next_bp = valuePtr(bp+WSIZE);
	if(bp == head[i]){
		head[i] = next_bp;
		if(head[i] != NULL)
			putPtr(head[i], NULL);
	}
	else
		putPtr(prev_bp+WSIZE, next_bp);
	if(bp == foot[i])
	{
		foot[i] = prev_bp;
		if(foot[i] != NULL)
			putPtr(foot[i]+WSIZE, NULL);
	}
	else
		putPtr(next_bp, prev_bp);
}


static int range(size_t size)      //���ݴ�С���ط�Χ 
{
	if(size < 64)           
		return 0;
	if(size < 128)
		return 1;
	if(size < 256)
		return 2;
	if(size < 512)
		return 3;
	if(size < 1024)
		return 4;
	if(size < 2048)
		return 5;
	if(size < 4096)
		return 6;
	if(size < 8192)
		return 7;
	if(size < 16384)
		return 8;
	return 9;
}

static int mm_check(void)    
{
	char *bp = heap_listp;
	int isRight = 1;
	//���������
	if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp))){
		printf("Header is wrong!\n");
		isRight--;
	}					     
	isRight -= checkblock(heap_listp);
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
		isRight -= checkblock(bp);
	if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp)))){
		printf("Ender is wrong!\n");
		isRight--;
	}								
	return isRight;
}
static int checkblock(void *bp) 	  
{
	int errorNum = 0;	//���ִ���
	if ((size_t)bp % 8){
		printf("Error: %p is not doubleword aligned\n", bp);
		errorNum++;
	}

	if (GET(HDRP(bp)) != GET(FTRP(bp))){
		printf("Error: header does not equal footer\n");
		errorNum++;
	}
	return errorNum;
}
