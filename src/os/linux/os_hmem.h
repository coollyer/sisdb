#ifndef _os_hmem_h
#define _os_hmem_h

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <sys/mman.h>

#include <os_types.h>
#include <os_thread.h>

// 因为多数是指针的申请 所以最小设置为16个字节 节省map的空间
#define SIS_HEME_BLKSIZE  16 // 因为已经使用的块信息需要写入文件 因此头有16个字节 最小块 +8 = 24

typedef struct s_hmem_head
{	
    uint32   blks;
    uint32   size;
    char     data[0];  //  
} s_hmem_head;

// node的尺寸大小决定了实际物理内存的大小 单向表更好维护 只需要建立一个索引就可以了
typedef struct s_hmem_node
{	
	int64   head;
    int64   tail;
    struct s_hmem_node *next;
    struct s_hmem_node *prev;
} s_hmem_node;

typedef struct s_sis_hmem {	
    s_sis_mutex_t   mutex;
    int64           msize;       // 内存尺寸 默认不超过 SIS_HEME_MAXSIZE
    char           *memmap;      // 内存
	s_hmem_node    *usbale;      // 必需按尺寸排序 否则内存碎片太多
    s_hmem_head     indexs[32];  // 建立一个blks等分的序列表 从 2个块开始 到2^32次方
} s_sis_hmem;

// useidx[0] --> 2
// useidx[1] --> 4
// useidx[2] --> 8
// ...

#ifdef __cplusplus
extern "C" {
#endif

s_sis_hmem *sis_hmem_create(int64);
void sis_hmem_destroy(s_sis_hmem *);

void *sis_hmem_malloc(s_sis_hmem *, int64);
void *sis_hmem_calloc(s_sis_hmem *, int64);
void *sis_hmem_realloc(s_sis_hmem *, void *, int64);

void sis_hmem_free(s_sis_hmem *, void *);

/// @brief ///
/// @param  
/// @param  
/// @return 
void sis_hmem_recy_occupy(s_sis_hmem *, s_hmem_head *);

void sis_hmem_incr_usable(s_sis_hmem *, s_hmem_node *);

void sis_hmem_swap_node(s_sis_hmem *hmem, s_hmem_node *nnode, s_hmem_node *fnode);


#ifdef __cplusplus
}
#endif

#endif /* _SIS_THREAD_H */
