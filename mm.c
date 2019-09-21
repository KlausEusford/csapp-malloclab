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

#define ALIGNMENT 8   //对齐要求，设置为双字对齐

#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)   
//对一个存储大小对齐

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//基本的常数和宏定义
#define WSIZE		4	// 一个字的长度（字节为单位）
#define DSIZE		8	//双字长度，字节为单位
#define CHUNKSIZE	(1 << 12)	// 初始化堆的大小为4KB 
#define OVERHEAD	8	//已分配的块只具有头部 

#define MAXN	10
#define MAX(x, y)	((x) > (y) ? (x) : (y))

#define PACK(size, alloc)	((size)|(alloc))  
// 把一个块的大小，本块是否分配，前一块是否分配封装在一个字中

//对一个字的读写，用于修改一个字的头部，尾部和前后指针
#define GET(p)	(*(size_t *)(p))    
#define PUT(p, val)	(*(size_t *)(p) = (val))  

#define GET_SIZE(p)	(GET(p) & ~0x7)  // 读块的大小 
#define GET_ALLOC(p)	(GET(p) & 0x1)  //获得分配的字节

//通过块指针计算块的头部和尾部
#define HDRP(bp)	((char *)(bp) - WSIZE)   
#define FTRP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

//通过块指针，计算前一块和后一块的块指针
#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))  
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))  

// 分配器总是使用一个单一的私有全局变量 
//指向初始块
static char *heap_listp;
int counter = 0;	//全局计数器

//双向链表
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
	// 初始化先为堆申请MAXN字长的空间，一个作为序言块，一个作为结尾 
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

	//如果没有成功申请到地址，初始化失败，返回-1
	if((heap_listp = mem_sbrk(4*WSIZE)) == NULL)
		return -1;
	PUT(heap_listp, 0); 	//对齐填充
	PUT(heap_listp+WSIZE, PACK(OVERHEAD, 1));	// 序言块头部
	PUT(heap_listp+DSIZE, PACK(OVERHEAD, 1));	// 序言块尾部
	PUT(heap_listp+WSIZE+DSIZE, PACK(0, 1));	// 结尾块头部 
	heap_listp += DSIZE;
	// 用初始化堆的大小扩展空闲块
	if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0;
}

static void *extend_heap(size_t words)   
	//当初始化或者找不到合适的时扩展堆得大小 
{
	char *bp;
	size_t size;
	// 分配一个偶数保持对齐
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if((int)(bp = mem_sbrk(size)) == -1)
		return NULL;
	// 初始化空闲块头尾以及结尾块头部
	PUT(HDRP(bp), PACK(size, 0));	// 空闲块头 
	PUT(FTRP(bp), PACK(size, 0));	// 空闲块尾 
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));	//新的序幕块 
	// 如果之前的块是空的则合并 
	return coalesce(bp);
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)  
{
	size_t asize;	// 调整的块大小 
	size_t extendsize;	// 如果不合适的话计算扩展大小 
	char *bp;
	// 忽略虚假请求 
	if(size <= 0)
		return NULL;
	// 调整块的大小使其合适 
	if(size <= DSIZE)
		asize = DSIZE + OVERHEAD;
	else
		asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);
	// 寻找合适的空闲块 
	if((bp = find_fit(asize)) != NULL){
		return place(bp, asize);	
	}
	// 找不到合适的空闲块则扩展存储空间
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
	// 选择列表 
	int i = range(asize);
	// 最合适的搜索 
	for(; i < MAXN; i++)
	{   
		// 从低地址到高地址搜索列表 
		for(bp = foot[i]; bp != NULL; bp = valuePtr(bp))
		{ //让下一个空闲块在同一个列表
			if(asize <= GET_SIZE(HDRP(bp)))
				return bp;
		}
	}
	return NULL; //不适合则返回
}

static void *place(void *bp, size_t asize){
	size_t csize = GET_SIZE(HDRP(bp));
	counter++;
	void *ret;
	deletefreelist(bp);	//从空闲列表中删除当前块 
	if((csize - asize) >= (DSIZE + OVERHEAD)){		
		if(counter % 2 == 1){
			ret = bp;		
			PUT(HDRP(bp), PACK(asize, 1));
			PUT(FTRP(bp), PACK(asize, 1));
			bp = NEXT_BLKP(bp);
			PUT(HDRP(bp), PACK(csize-asize, 0));
			PUT(FTRP(bp), PACK(csize-asize, 0));
			addfreelist(bp);	// 加入到空闲块中 
		}
		else{
			PUT(HDRP(bp), PACK(csize-asize, 0));
			PUT(FTRP(bp), PACK(csize-asize, 0));
			addfreelist(bp);	// 加入到空闲块中 
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
	// 添加和删除条目 
	//前后都已分配不需要合并，没有操作
	if(prev_alloc && next_alloc) {                 
		addfreelist((char *)bp);
		return bp;
	}
	// 前面已分配，后面空闲，与后面相合并
	else if(prev_alloc && !next_alloc) {              	
		deletefreelist((char *)NEXT_BLKP(bp));
		deletefreelist((char *)bp);
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
		addfreelist((char *)bp);
		return (bp);
	}
	// 前面空闲，后面已分配，和前面想合并 
	else if(!prev_alloc && next_alloc) {            	
		deletefreelist((char *)PREV_BLKP(bp));
		deletefreelist((char *)bp);
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		addfreelist((char *)PREV_BLKP(bp));
		return (PREV_BLKP(bp));
	}
	// 前后都空闲，与前后合并
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
	if(ptr == NULL)	//指针为空
		return mm_malloc(size);
	else if(size == 0)   // 大小为0
	{ mm_free(ptr);
	return NULL;
	}
	//正常情况下
	size_t curr_size = GET_SIZE(HDRP(ptr));   // 当前块的大小
	size_t asize;
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
	size_t prev_size = GET_SIZE(FTRP(PREV_BLKP(ptr)));
	size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));

	void *newptr;     
	//调整当前快的大小使得满足空间和对齐 
	if(size <= DSIZE)
		asize = DSIZE + OVERHEAD;
	else
		asize = DSIZE * ((size + (OVERHEAD) + (DSIZE-1)) / DSIZE);
	// 三个都空闲 
	if(prev_alloc && next_alloc && asize <= curr_size)	
	{
		ptr = place(ptr, asize);
		return ptr;
	}
	//第一个空闲，后两个不是
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
		memcpy(newptr, ptr, curr_size-OVERHEAD);     // 把原先的变量复制到新的地址
		newptr = place(newptr, asize);
		return newptr;
	}
	//都非空闲状态 
	else if(!prev_alloc && !next_alloc && asize <= (curr_size + prev_size + next_size))
	{
		newptr = PREV_BLKP(ptr);
		deletefreelist(newptr);
		deletefreelist(NEXT_BLKP(ptr));
		PUT(HDRP(newptr), PACK(curr_size + prev_size + next_size, 1));
		PUT(FTRP(newptr), PACK(curr_size + prev_size + next_size, 1));
		memcpy(newptr, ptr, curr_size-OVERHEAD);   
		//把旧的值复制到新的地方 		
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
static char *valuePtr(char *ptr)   // 列表中返回下一个空闲块指针
{
	return *(char **)ptr;
}
/********************************************************/
static void putPtr(char *ptr, char *newPtr)  // 把指针设到洗呢指针上 
{
	*(char **)ptr = newPtr;
}
/******************************************************/

static void addfreelist(char *bp)   // 在列表中增加一个空闲块
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


static void deletefreelist(char *bp)      // 如果已经被分配了就删除一个块 
{
	int i = range(GET_SIZE(HDRP(bp)));
	//寻找要删除的元素
	char *temp = head[i];
	while(temp != NULL)
	{
		if(temp == bp)
			break;
		temp = valuePtr(temp+WSIZE);
	}
	if(temp == NULL)    //找不到
		return;
	char *prev_bp = valuePtr(bp);   // 找到了
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


static int range(size_t size)      //根据大小返回范围 
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
	//如果错误发生
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
	int errorNum = 0;	//数字错误
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
