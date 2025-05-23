﻿

#include <os_malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <os_fork.h>
#ifndef __RELEASE__

#ifdef HUGE_MEM

s_sis_hmem  *__hmem_class = NULL;

void safe_memory_start()
{
    if (!__hmem_class)
    {
        // 得到实际大小
        int64 size = sis_str_2_int(HUGE_MEM);
        // sis_out_binary("::", HUGE_MEM, 16); 
        // printf("%lld\n", size);
        __hmem_class = sis_hmem_create(size == 0 ? (long long)32 * 1024 * 1024 * 1024 : size);
        printf("hmem memory begin.\n");
    }
}

void safe_memory_stop()
{
    if (__hmem_class)
    {
        sis_hmem_exit(__hmem_class);
        __hmem_class = NULL;
        printf("hmem memory end.\n");
    }
}

#else

s_memory_node *__memory_first, *__memory_last;
s_sis_mutex_t  __memory_mutex;
size_t __memory_size = 0;

void safe_memory_start()
{
    sis_mutex_create(&__memory_mutex);
    __memory_first = NULL;
    __memory_last = __memory_first;
    __memory_size = 0;
    printf("safe memory begin.\n");
}

int safe_memory_cmp(const char *s1_, const char *s2_)
{
    if (!s1_||!s2_)
    {
        return 1;
    }
    for (; (*s1_) == (*s2_); ++s1_, ++s2_)
    {
        if (*s1_ == 0 || *s2_ == 0)
        {
            return 0;
        }
    }
    return 1;
}

void safe_memory_stop()
{
    s_memory_node *node = __memory_first;
    if (node)
    {
        printf("no free memory:\n");
    }
    size_t size = 0;
    if (sis_get_signal() != SIS_SIGNAL_EXIT)
    {
        sis_mutex_lock(&__memory_mutex);
        int i = 0;
        while (node)
        {
            unsigned char *ptr = (unsigned char *)node + MEMORY_NODE_SIZE;
            size += node->size + MEMORY_NODE_SIZE;
            printf("[%4d] %p [%d] func:%s, lines:%d :: ", i++,
                ptr, node->size, node->info, node->line);
            if (!safe_memory_cmp(node->info, "sis_strdup")||
                !safe_memory_cmp(node->info, "sis_sds_addlen")||
                !safe_memory_cmp(node->info, "sis_sdsnewlen"))
            {
                printf("%s", ptr);
            }
            printf("\n");
            node = node->next;
        }
        sis_mutex_unlock(&__memory_mutex);
    }
    sis_mutex_destroy(&__memory_mutex);
    printf("safe memory end. %zu\n", size);
}

size_t safe_memory_getsize()
{
    return __memory_size;
}

#endif
#endif
// void check_memory_newnode(void *__p__,int line_,const char *func_)
// {
//     s_memory_node *__n = (s_memory_node *)__p__;
//     __n->next = NULL; __n->line = line_;
//     memmove(__n->info, func_, MEMORY_INFO_SIZE); __n->info[MEMORY_INFO_SIZE - 1] = 0;
//     if (!__memory_first) {
//         __n->prev = NULL;
//         __memory_first = __n; __memory_last = __memory_first;
//     } else {
//         __n->prev = __memory_last;
//         __memory_last->next = __n;
//         __memory_last = __n;
//     }
// }
// void check_memory_freenode(void *__p__)
// {
//     s_memory_node *__n = (s_memory_node *)__p__;
//     if(__n->prev) { __n->prev->next = __n->next; }
//     else { __memory_first = __n->next; }
//     if(__n->next)  { __n->next->prev = __n->prev; }
//     else { __memory_last = __n->prev; }
// }

// void sis_free(void *__p__)
// {
//     if (!__p__) {return;}
//     void *p = (char *)__p__ - MEMORY_NODE_SIZE;
//     check_memory_freenode(p);
//     free(p);
// }

#if 0
#include <sisdb_io.h>
int main()
{
    check_memory_start();
    sisdb_open("../conf/sisdb.conf");

    sisdb_close();
    check_memory_stop();
    return 0;
}
int main1()
{
    check_memory_start();

    void *aa = sis_malloc(100);
    memmove(aa,"my is aa\n",10);
    void *bb = sis_calloc(100);
    memmove(bb,"my is bb\n",10);
    void *dd = NULL;
    void *cc = sis_realloc(dd,1000);
    sprintf(cc,"my is cc %p-->%p\n",aa,cc);
    printf(aa);
    printf(bb);
    printf(cc);
    sis_free(cc);
    check_memory_stop();
    return 0;
}
#endif