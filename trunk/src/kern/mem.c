/*
 * mem.c
 *
 *  Created on: 2012-2-23
 *      Author: Administrator
 */

#include "config.h"
#include "types.h"
#include "kern.h"
#include "mem.h"

/*
 * 内存块状态
 */
#define MEM_BLK_STAT_FREE           0
#define MEM_BLK_STAT_USED           (1 << 4)

#define MEM_BLK_MAGIC0              0x12345678

struct _mem_block {
    uint32_t        magic0;
    mem_block      *prev;
    mem_block      *next;
    mem_block      *prev_free;
    mem_block      *next_free;
    uint32_t        size;
    uint8_t         status;
};

#define MEM_ALIGNMENT               4
#define MEM_ALIGN_SIZE(size)        (((size) + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT - 1))
#define MEM_ALIGN_SIZE_LESS(size)   (((size) & ~(MEM_ALIGNMENT - 1)))
#define MEM_ALIGN(addr)             ((void *)(((uint32_t)(addr) + MEM_ALIGNMENT - 1) & ~(uint32_t)(MEM_ALIGNMENT - 1)))



/*
 * 初始化内存堆
 */
int mem_heap_init(mem_heap *heap, uint8_t *base, uint32_t size)
{
    heap->base = MEM_ALIGN(base);                                       /*  对齐基址                    */

    heap->size = MEM_ALIGN_SIZE_LESS(size - (heap->base - base));       /*  对齐大小                    */

    heap->block_list            = (mem_block *)heap->base;              /*  内存块链表                  */
    heap->block_list->magic0    = MEM_BLK_MAGIC0;
    heap->block_list->prev      = NULL;
    heap->block_list->next      = NULL;
    heap->block_list->prev_free = NULL;
    heap->block_list->next_free = NULL;
    heap->block_list->size      = heap->size - MEM_ALIGN_SIZE(sizeof(mem_block));
    heap->block_list->status    = MEM_BLK_STAT_FREE;

    heap->free_list             = heap->block_list;                     /*  空闲内存块链表              */

    return 0;
}

/*
 * 分配内存
 */
void *mem_heap_alloc(mem_heap *heap, uint32_t size)
{
    mem_block *blk, *new_blk;

    if (heap->free_list != NULL) {                                      /*  有空闲内存块                */

        size = MEM_ALIGN_SIZE(size);                                    /*  对齐大小                    */

        blk = heap->free_list;                                          /*  从空闲内存块链表里找出一个  */
        while (blk != NULL && blk->size < size) {                       /*  首次满足大小的空闲内存块    */
            blk = blk->next_free;
        }

        if (blk == NULL) {                                              /*  没找到                      */
            return NULL;
        }

        if (blk->size <= MEM_ALIGN_SIZE(sizeof(mem_block)) + size) {    /*  如果内存块切割后, 剩余的大小*/
                                                                        /*  不足或只够建立一个内存块节点*/
                                                                        /*  则整块分配出去              */
            if (blk == heap->free_list) {                               /*  处理空闲内存块链表链头      */
                heap->free_list = blk->next_free;
            }

            if (blk->prev_free != NULL) {                               /*  从空闲内存块链表中删除内存块*/
                blk->prev_free->next_free = blk->next_free;
            }
            if (blk->next_free != NULL) {
                blk->next_free->prev_free = blk->prev_free;
            }

            blk->status = MEM_BLK_STAT_USED;                            /*  改变内存块状态为已用        */

            new_blk = blk;

        } else {

            new_blk = (mem_block *)((char *)blk + blk->size - size);    /*  建立一个新的内存块          */

            if (blk->next != NULL) {                                    /*  新内存块加入内存块链表      */
                blk->next->prev = new_blk;
            }
            new_blk->next = blk->next;
            blk->next     = new_blk;
            new_blk->prev = blk;

            new_blk->size   = size;
            new_blk->status = MEM_BLK_STAT_USED;                        /*  新内存块状态为已用          */
                                                                        /*  所以不在空闲内存块链表      */

            blk->size -= MEM_ALIGN_SIZE(sizeof(mem_block)) + size;      /*  原内存块变小                */
        }

        new_blk->magic0 = MEM_BLK_MAGIC0;                               /*  魔数                        */

        return (char *)new_blk + MEM_ALIGN_SIZE(sizeof(mem_block));
    }

    return NULL;
}

/*
 * 释放内存
 */
void *mem_heap_free(mem_heap *heap, void *ptr)
{
    mem_block *blk;
    mem_block *prev;
    mem_block *next;

    if (ptr == NULL) {
        return ptr;
    }

    if (ptr != MEM_ALIGN(ptr)) {
        return ptr;
    }
                                                                        /*  指针所在的内存块节点        */
    blk  = (mem_block *)((char *)ptr - MEM_ALIGN_SIZE(sizeof(mem_block)));
    if (blk == NULL) {
        return ptr;
    }

    if (blk->magic0 != MEM_BLK_MAGIC0 || blk->status != MEM_BLK_STAT_USED) {
        return ptr;
    }

    prev = blk->prev;
    next = blk->next;

    if (next != NULL && next->magic0 != MEM_BLK_MAGIC0) {               /*  写缓冲区溢出                */
        return ptr;
    }

    if (prev != NULL && prev->status == MEM_BLK_STAT_FREE) {            /*  前一个内存块空闲, 合并之    */
        prev->size += MEM_ALIGN_SIZE(sizeof(mem_block)) + blk->size;    /*  前一个内存块变大            */

        prev->next = blk->next;                                         /*  从内存块链表中删除内存块    */
        if (next != NULL) {
            next->prev = prev;
        }

    } else if (next != NULL && next->status == MEM_BLK_STAT_FREE) {     /*  后一个内存块空闲, 合并之    */

        blk->size  += MEM_ALIGN_SIZE(sizeof(mem_block)) + next->size;   /*  内存块变大                  */
        blk->status = MEM_BLK_STAT_FREE;                                /*  改变内存块状态为空闲        */

        if (next->next != NULL) {                                       /*  从内存块链表中删除后内存块  */
            next->next->prev = blk;
        }
        blk->next = next->next;
        if (heap->block_list == next) {
            heap->block_list = blk;
        }

        if (next->next_free != NULL) {                                  /*  从空闲块链表中删除后内存块  */
            next->next_free->prev_free = blk;
        }

        if (next->prev_free != NULL) {
            next->prev_free->next_free = blk;
        }

        blk->prev_free = next->prev_free;                               /*  把内存块加入空闲内存块链表  */
        blk->next_free = next->next_free;
        if (next == heap->free_list) {
            heap->free_list = blk;
        }
    } else {
        blk->prev_free = NULL;                                          /*  把内存块加入空闲内存块链表  */
        blk->next_free = heap->free_list;
        if (heap->free_list != NULL) {
            heap->free_list->prev_free = blk;
        } else {
            heap->free_list = blk;
        }
        blk->status = MEM_BLK_STAT_FREE;                                /*  改变内存块状态为空闲        */
    }

    return NULL;
}
















