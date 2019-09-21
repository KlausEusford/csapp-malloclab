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

	/* ��ʼ������������� */
	for (listnumber = 0; listnumber < LISTMAX; listnumber++)
	{
		segregated_free_lists[listnumber] = NULL;
	}

	/* ��ʼ���� */
	if ((long)(heap = mem_sbrk(4 * WSIZE)) == -1)
		return -1;

	/* ����Ľṹ�μ���������ġ��ѵ���ʼ�ͽ����ṹ�� */
	PUT(heap, 0);
	PUT(heap + (1 * WSIZE), PACK(DSIZE, 1));
	PUT(heap + (2 * WSIZE), PACK(DSIZE, 1));
	PUT(heap + (3 * WSIZE), PACK(0, 1));

	/* ��չ�� */
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
	/* �ڴ���� */
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
		/* Ѱ�Ҷ�Ӧ�� */
		if (((searchsize <= 1) && (segregated_free_lists[listnumber] != NULL)))
		{
			ptr = segregated_free_lists[listnumber];
			/* �ڸ���Ѱ�Ҵ�С���ʵ�free�� */
			while ((ptr != NULL) && ((size > GET_SIZE(HDRP(ptr)))))
			{
				ptr = PRED(ptr);
			}
			/* �ҵ���Ӧ��free�� */
			if (ptr != NULL)
				break;
		}

		searchsize >>= 1;
		listnumber++;
	}

	/* û���ҵ����ʵ�free�飬��չ�� */
	if (ptr == NULL)
	{
		if ((ptr = extend_heap(MAX(size, CHUNKSIZE))) == NULL)
			return NULL;
	}

	/* ��free����allocate size��С�Ŀ� */
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

	/* �������������� */
	insert_node(ptr, size);
	/* ע��ϲ� */
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

	/* �ڴ���� */
	if (size <= DSIZE)
	{
		size = 2 * DSIZE;
	}
	else
	{
		size = ALIGN(size + DSIZE);
	}

	/* ���sizeС��ԭ����Ĵ�С��ֱ�ӷ���ԭ���Ŀ� */
	if ((remainder = GET_SIZE(HDRP(ptr)) - size) >= 0)
	{
		return ptr;
	}
	/* �����ȼ���ַ������һ�����Ƿ�Ϊfree����߸ÿ��ǶѵĽ����飬��Ϊ����Ҫ�������������ڵ�free�飬�Դ˼�С��external fragmentation�� */
	else if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr))))
	{
		/* ��ʹ���Ϻ���������ַ�ϵ�free��ռ�Ҳ��������Ҫ��չ�� */
		if ((remainder = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - size) < 0)
		{
			if (extend_heap(MAX(-remainder, CHUNKSIZE)) == NULL)
				return NULL;
			remainder += MAX(-remainder, CHUNKSIZE);
		}

		/* ɾ���ո����õ�free�鲢�����¿��ͷβ */
		delete_node(NEXT_BLKP(ptr));
		PUT(HDRP(ptr), PACK(size + remainder, 1));
		PUT(FTRP(ptr), PACK(size + remainder, 1));
	}
	/* û�п������õ�����free�飬����size����ԭ���Ŀ飬��ʱֻ�������µĲ�������free�顢����ԭ�����ݡ��ͷ�ԭ�� */
	else
	{
		new_block = mm_malloc(size);
		memcpy(new_block, ptr, GET_SIZE(HDRP(ptr)));
		mm_free(ptr);
	}

	return new_block;
}
    













