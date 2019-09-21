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
    "ateam",
    /* First member's full name */
    "Wang Yingming",
    /* First member's email address */
    "16307130015@fudan.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	int listnumber;
	char *heap;

	/* 初始化分离空闲链表 */
	for (listnumber = 0; listnumber < LISTMAX; listnumber++)
	{
		segregated_free_lists[listnumber] = NULL;
	}

	/* 初始化堆 */
	if ((long)(heap = mem_sbrk(4 * WSIZE)) == -1)
		return -1;

	/* 这里的结构参见本文上面的“堆的起始和结束结构” */
	PUT(heap, 0);
	PUT(heap + (1 * WSIZE), PACK(DSIZE, 1));
	PUT(heap + (2 * WSIZE), PACK(DSIZE, 1));
	PUT(heap + (3 * WSIZE), PACK(0, 1));

	/* 扩展堆 */
	if (extend_heap(INITCHUNKSIZE) == NULL)
		return -1;

	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	if (size == 0)
		return NULL;
	/* 内存对齐 */
	if (size <= DSIZE)
	{
		size = 2 * DSIZE;
	}
	else
	{
		size = ALIGN(size + DSIZE);
	}


	int listnumber = 0;
	size_t searchsize = size;
	void *ptr = NULL;

	while (listnumber < LISTMAX)
	{
		/* 寻找对应链 */
		if (((searchsize <= 1) && (segregated_free_lists[listnumber] != NULL)))
		{
			ptr = segregated_free_lists[listnumber];
			/* 在该链寻找大小合适的free块 */
			while ((ptr != NULL) && ((size > GET_SIZE(HDRP(ptr)))))
			{
				ptr = PRED(ptr);
			}
			/* 找到对应的free块 */
			if (ptr != NULL)
				break;
		}

		searchsize >>= 1;
		listnumber++;
	}

	/* 没有找到合适的free块，扩展堆 */
	if (ptr == NULL)
	{
		if ((ptr = extend_heap(MAX(size, CHUNKSIZE))) == NULL)
			return NULL;
	}

	/* 在free块中allocate size大小的块 */
	ptr = place(ptr, size);

	return ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	size_t size = GET_SIZE(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));

	/* 插入分离空闲链表 */
	insert_node(ptr, size);
	/* 注意合并 */
	coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	void *new_block = ptr;
	int remainder;

	if (size == 0)
		return NULL;

	/* 内存对齐 */
	if (size <= DSIZE)
	{
		size = 2 * DSIZE;
	}
	else
	{
		size = ALIGN(size + DSIZE);
	}

	/* 如果size小于原来块的大小，直接返回原来的块 */
	if ((remainder = GET_SIZE(HDRP(ptr)) - size) >= 0)
	{
		return ptr;
	}
	/* 否则先检查地址连续下一个块是否为free块或者该块是堆的结束块，因为我们要尽可能利用相邻的free块，以此减小“external fragmentation” */
	else if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr))))
	{
		/* 即使加上后面连续地址上的free块空间也不够，需要扩展块 */
		if ((remainder = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - size) < 0)
		{
			if (extend_heap(MAX(-remainder, CHUNKSIZE)) == NULL)
				return NULL;
			remainder += MAX(-remainder, CHUNKSIZE);
		}

		/* 删除刚刚利用的free块并设置新块的头尾 */
		delete_node(NEXT_BLKP(ptr));
		PUT(HDRP(ptr), PACK(size + remainder, 1));
		PUT(FTRP(ptr), PACK(size + remainder, 1));
	}
	/* 没有可以利用的连续free块，而且size大于原来的块，这时只能申请新的不连续的free块、复制原块内容、释放原块 */
	else
	{
		new_block = mm_malloc(size);
		memcpy(new_block, ptr, GET_SIZE(HDRP(ptr)));
		mm_free(ptr);
	}

	return new_block;
}
    













