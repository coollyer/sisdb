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

#define SIS_HEME_BLKSIZE  32

typedef struct s_hmem_node
{	
	int64   head;
    int64   tail ;
    struct s_hmem_node *next ;
    struct s_hmem_node *prev ;
} s_hmem_node;

typedef struct s_sis_hmem {	
    s_sis_mutex_t  mutex;
    int64          msize;   // 内存尺寸 默认不超过 SIS_HEME_MAXSIZE
    char          *memmap;  // 内存
	s_hmem_node   *usbale;  // 必需按尺寸排序 否则内存碎片太多
    s_hmem_node   *occupy;  // 可不排序
} s_sis_hmem;

#ifdef __cplusplus
extern "C" {
#endif

s_sis_hmem *sis_hmem_create(int64);
void sis_hmem_destroy(s_sis_hmem *);

void *sis_hmem_malloc(s_sis_hmem *, int64);
void *sis_hmem_calloc(s_sis_hmem *, int64);
void *sis_hmem_realloc(s_sis_hmem *, void *, int64);

void sis_hmem_free(s_sis_hmem *, void *);

int  sis_hmem_isnil(s_sis_hmem *);

/// @brief ///
/// @param  
/// @param  
/// @return 
s_hmem_node *sis_hmem_find_occupy(s_sis_hmem *, void *);
void sis_hmem_incr_occupy(s_sis_hmem *, s_hmem_node *);
void sis_hmem_decr_occupy(s_sis_hmem *, s_hmem_node *);
void sis_hmem_incr_usable(s_sis_hmem *, s_hmem_node *);
void sis_hmem_decr_usable(s_sis_hmem *, s_hmem_node *);


#ifdef __cplusplus
}
#endif


#endif /* _SIS_THREAD_H */
