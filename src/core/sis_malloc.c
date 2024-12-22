#include "sis_malloc.h"

void sis_free_call(void *p)
{
	sis_free(p);
}

void sis_sdsfree_call(void *p)
{
	sis_sdsfree((s_sis_sds)p);
}

// 内存池
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <limits.h>

// // 内存池结构体，用于整体管理内存池相关信息
// typedef struct MemoryPool {
//     void *memory;              // 指向整块预分配的内存区域
//     size_t total_size;         // 内存池总的字节大小
//     size_t block_size;         // 每个可分配内存块的固定大小
//     size_t num_blocks;         // 内存池中内存块的数量
//     char *bitmap;              // 位图，用于标记内存块是否被占用，每个比特对应一个内存块
//     struct MemoryPool *next;   // 指向下一个内存池节点（用于扩展内存池）
// } MemoryPool;

// // 初始化内存池
// MemoryPool *memory_pool_init(size_t block_size, size_t num_blocks) {
//     MemoryPool *pool = (MemoryPool *)malloc(sizeof(MemoryPool));
//     if (pool == NULL) {
//         perror("Failed to allocate memory pool structure");
//         return NULL;
//     }

//     // 计算内存池总的字节大小，需考虑对齐等因素，这里简单以block_size对齐
//     pool->total_size = (num_blocks * block_size + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
//     pool->block_size = block_size;
//     pool->num_blocks = num_blocks;

//     // 分配整块内存区域
//     pool->memory = malloc(pool->total_size);
//     if (pool->memory == NULL) {
//         free(pool);
//         perror("Failed to allocate memory for the pool");
//         return NULL;
//     }

//     // 初始化位图，每个比特初始化为0，表示对应内存块未被占用
//     pool->bitmap = (char *)malloc((num_blocks + CHAR_BIT - 1) / CHAR_BIT);
//     if (pool->bitmap == NULL) {
//         free(pool->memory);
//         free(pool);
//         perror("Failed to allocate bitmap");
//         return NULL;
//     }
//     memset(pool->bitmap, 0, (num_blocks + CHAR_BIT - 1) / CHAR_BIT);

//     pool->next = NULL;
//     return pool;
// }

// // 查找空闲内存块，通过遍历位图找到第一个为0的比特对应的内存块
// size_t find_free_block(MemoryPool *pool) {
//     for (size_t i = 0; i < pool->num_blocks; i++) {
//         size_t byte_index = i / CHAR_BIT;
//         size_t bit_index = i % CHAR_BIT;
//         if (!(pool->bitmap[byte_index] & (1 << bit_index))) {
//             return i;
//         }
//     }
//     return (size_t)-1;  // 表示没有找到空闲内存块
// }

// // 从内存池中分配一个内存块
// void *memory_pool_alloc(MemoryPool *pool) {
//     size_t block_index = find_free_block(pool);
//     if (block_index == (size_t)-1) {
//         // 当前内存池没有空闲内存块了，可考虑扩展内存池等策略，这里简单返回NULL
//         return NULL;
//     }

//     // 在位图中标记该内存块已被占用
//     size_t byte_index = block_index / CHAR_BIT;
//     size_t bit_index = block_index % CHAR_BIT;
//     pool->bitmap[byte_index] |= (1 << bit_index);

//     // 返回对应内存块的地址
//     return (char *)pool->memory + block_index * pool->block_size;
// }

// // 将内存块释放回内存池
// void memory_pool_free(MemoryPool *pool, void *ptr) {
//     if (ptr == NULL) {
//         return;
//     }

//     char *memory_start = (char *)pool->memory;
//     size_t offset = ptr - memory_start;
//     if (offset % pool->block_size!= 0 || offset >= pool->total_size) {
//         // 检查释放的指针是否合法，是否属于该内存池且是对齐的内存块地址
//         fprintf(stderr, "Invalid pointer to free\n");
//         return;
//     }

//     size_t block_index = offset / pool->block_size;
//     // 在位图中标记该内存块为空闲
//     size_t byte_index = block_index / CHAR_BIT;
//     size_t bit_index = block_index % CHAR_BIT;
//     pool->bitmap[byte_index] &= ~(1 << bit_index);
// }

// // 销毁内存池，释放所有相关内存
// void memory_pool_destroy(MemoryPool *pool) {
//     if (pool == NULL) {
//         return;
//     }

//     free(pool->memory);
//     free(pool->bitmap);
//     MemoryPool *next = pool->next;
//     free(pool);
//     if (next!= NULL) {
//         memory_pool_destroy(next);
//     }
// }

// // 示例结构体，代表要分配的固定大小小数据结构
// typedef struct SmallData {
//     int id;
//     char name[10];
// } SmallData;

// int main() {
//     // 初始化一个内存池，每个内存块大小为SmallData结构体大小，共10个内存块
//     MemoryPool *pool = memory_pool_init(sizeof(SmallData), 10);
//     if (pool == NULL) {
//         return 1;
//     }

//     // 分配内存块并使用
//     SmallData *data1 = (SmallData *)memory_pool_alloc(pool);
//     if (data1!= NULL) {
//         data1->id = 1;
//         strcpy(data1->name, "data1");
//         printf("Allocated memory for data1: id = %d, name = %s\n", data1->id, data1->name);
//     }

//     SmallData *data2 = (SmallData *)memory_pool_alloc(pool);
//     if (data2!= NULL) {
//         data2->id = 2;
//         strcpy(data2->name, "data2");
//         printf("Allocated memory for data2: id = %d, name = %s\n", data2->id, data2->name);
//     }

//     // 释放内存块
//     memory_pool_free(pool, data1);
//     memory_pool_free(pool, data2);

//     // 再次分配并使用内存块
//     SmallData *data3 = (SmallData *)memory_pool_alloc(pool);
//     if (data3!= NULL) {
//         data3->id = 3;
//         strcpy(data3->name, "data3");
//         printf("Allocated memory for data3: id = %d, name = %s\n", data3->id, data3->name);
//     }

//     // 销毁内存池
//     memory_pool_destroy(pool);

//     return 0;
// }
#if 0

// sort old
// hmem memory begin.
// === malloc 10000 cost = 3543 
// === free 5000 cost = 170176 
// === malloc 5000 cost = 356 
// === free 10000 cost = 100523 

// new no sort
// hmem memory begin.
// === malloc 10000 cost = 21213 
// === free 5000 cost = 3111 
// === malloc 5000 cost = 645 
// === free 10000 cost = 4344 
// === 1 ========================== 
// === malloc 10000 cost = 1134 
// === free 5000 cost = 1377 
// === malloc 5000 cost = 351 
// === free 10000 cost = 2172 
// === 2 ========================== 
// === malloc 10000 cost = 742 
// === free 5000 cost = 1138 
// === malloc 5000 cost = 359 
// === free 10000 cost = 3043 
// === 3 ========================== 
// === malloc 10000 cost = 901 
// === free 5000 cost = 1249 
// === malloc 5000 cost = 354 
// === free 10000 cost = 2327 
// === 4 ========================== 
// === malloc 10000 cost = 707 
// === free 5000 cost = 1189 
// === malloc 5000 cost = 412 
// === free 10000 cost = 2454 
// === 5 ========================== 
// === malloc 10000 cost = 672 
// === free 5000 cost = 1096 
// === malloc 5000 cost = 340 
// === free 10000 cost = 2453 
// === 6 ========================== 
// === malloc 10000 cost = 723 
// === free 5000 cost = 1468 
// === malloc 5000 cost = 389 
// === free 10000 cost = 2469 
// === 7 ========================== 
// === malloc 10000 cost = 688 
// === free 5000 cost = 1235 
// === malloc 5000 cost = 400 
// === free 10000 cost = 2579 
// === 8 ========================== 
// === malloc 10000 cost = 688 
// === free 5000 cost = 1341 
// === malloc 5000 cost = 351 
// === free 10000 cost = 2305 
// === 9 ========================== 
// === malloc 10000 cost = 668 
// === free 5000 cost = 1262 
// === malloc 5000 cost = 515 
// === free 10000 cost = 2465 


// test debug mem
// === 0 ========================== 
// === malloc 10000 cost = 15592 
// === free 5000 cost = 802 
// === malloc 5000 cost = 3915 
// === free 10000 cost = 7490 
// === 1 ========================== 
// === malloc 10000 cost = 10770 
// === free 5000 cost = 1700 
// === malloc 5000 cost = 5020 
// === free 10000 cost = 6567 
// === 2 ========================== 
// === malloc 10000 cost = 7744 
// === free 5000 cost = 1164 
// === malloc 5000 cost = 1977 
// === free 10000 cost = 2041 
// === 3 ========================== 
// === malloc 10000 cost = 5684 
// === free 5000 cost = 2146 
// === malloc 5000 cost = 1960 
// === free 10000 cost = 1626 
// === 4 ========================== 
// === malloc 10000 cost = 4694 
// === free 5000 cost = 1739 
// === malloc 5000 cost = 3386 
// === free 10000 cost = 1737 
// === 5 ========================== 
// === malloc 10000 cost = 2015 
// === free 5000 cost = 900 
// === malloc 5000 cost = 1011 
// === free 10000 cost = 2927 
// === 6 ========================== 
// === malloc 10000 cost = 4751 
// === free 5000 cost = 4004 
// === malloc 5000 cost = 1665 
// === free 10000 cost = 5612 
// === 7 ========================== 
// === malloc 10000 cost = 2902 
// === free 5000 cost = 2335 
// === malloc 5000 cost = 1757 
// === free 10000 cost = 4294 
// === 8 ========================== 
// === malloc 10000 cost = 4542 
// === free 5000 cost = 2658 
// === malloc 5000 cost = 1260 
// === free 10000 cost = 5562 
// === 9 ========================== 
// === malloc 10000 cost = 4349 
// === free 5000 cost = 712 
// === malloc 5000 cost = 2751 
// === free 10000 cost = 1421 


// test standed mem
// === 0 ========================== 
// === malloc 10000 cost = 1764 
// === free 5000 cost = 1117 
// === malloc 5000 cost = 3457 
// === free 10000 cost = 4093 
// === 1 ========================== 
// === malloc 10000 cost = 2823 
// === free 5000 cost = 1235 
// === malloc 5000 cost = 1001 
// === free 10000 cost = 2413 
// === 2 ========================== 
// === malloc 10000 cost = 4517 
// === free 5000 cost = 2330 
// === malloc 5000 cost = 1499 
// === free 10000 cost = 3181 
// === 3 ========================== 
// === malloc 10000 cost = 7033 
// === free 5000 cost = 595 
// === malloc 5000 cost = 3346 
// === free 10000 cost = 11306 
// === 4 ========================== 
// === malloc 10000 cost = 3694 
// === free 5000 cost = 537 
// === malloc 5000 cost = 2785 
// === free 10000 cost = 3919 
// === 5 ========================== 
// === malloc 10000 cost = 4734 
// === free 5000 cost = 1212 
// === malloc 5000 cost = 1165 
// === free 10000 cost = 2833 
// === 6 ========================== 
// === malloc 10000 cost = 3572 
// === free 5000 cost = 540 
// === malloc 5000 cost = 1013 
// === free 10000 cost = 5066 
// === 7 ========================== 
// === malloc 10000 cost = 4327 
// === free 5000 cost = 2180 
// === malloc 5000 cost = 2363 
// === free 10000 cost = 5302 
// === 8 ========================== 
// === malloc 10000 cost = 4292 
// === free 5000 cost = 1817 
// === malloc 5000 cost = 3913 
// === free 10000 cost = 1532 
// === 9 ========================== 
// === malloc 10000 cost = 2911 
// === free 5000 cost = 548 
// === malloc 5000 cost = 1941 
// === free 10000 cost = 1913 
// 这里测试hmem

#define TEST_NUMS 10000
char *testptr[TEST_NUMS];
#include "sis_math.h"
int main()
{
    safe_memory_start();
#ifdef HUGE_MEM
    printf("test hmem\n");
#else 
#ifdef __RELEASE__
    printf("test standed mem\n");
#else
    printf("test debug mem\n");
#endif
#endif
    sis_init_random();
    
    for (int i = 0; i < 10; i++)
    {
        printf("=== %d ========================== \n", i);
        msec_t start = sis_time_get_now_usec();
        for (int i = 0; i < TEST_NUMS; i++)
        {
            int64 len = sis_int_random(1, 4096);
            testptr[i] = sis_malloc(len);
        }
        msec_t costusec = sis_time_get_now_usec() - start;
        start = sis_time_get_now_usec();
        printf("=== malloc %d cost = %lld \n", TEST_NUMS, costusec);

        int freenum = TEST_NUMS / 2;
        for (int i = 0; i < freenum; i++)
        {
            sis_free(testptr[i]);
        }
        costusec = sis_time_get_now_usec() - start;
        start = sis_time_get_now_usec();
        printf("=== free %d cost = %lld \n", freenum, costusec);

        for (int i = 0; i < freenum; i++)
        {
            int64 len = sis_int_random(1, 4096);
            testptr[i] = sis_malloc(len);
        }
        costusec = sis_time_get_now_usec() - start;
        start = sis_time_get_now_usec();
        printf("=== malloc %d cost = %lld \n", freenum, costusec);

        for (int i = 0; i < TEST_NUMS; i++)
        {
            sis_free(testptr[i]);
        }
        costusec = sis_time_get_now_usec() - start;
        start = sis_time_get_now_usec();
        printf("=== free %d cost = %lld \n", TEST_NUMS, costusec);
    }
    sis_sleep(3000);
    printf("=== 1 ========================== \n");
    safe_memory_stop();
    printf("=== 2 ========================== \n");

return 0;

}
#endif