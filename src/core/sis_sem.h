
#ifndef _sis_sem_h
#define _sis_sem_h

#include "os_file.h"
#include "os_thread.h"
#include "sis_list.h"
#include <sis_malloc.h>

#define SEM_DEBUG     //  状态下不设置map读写锁

#define SEM_MAP_TYPE   uint8
#define SEM_MAP_MAXI   0xFF

// #define SEM_MAP_TYPE  (uint32)
// #define SEM_MAP_MAXI   0xFFFFFFFF

#define SIS_SEM_PATH   ".mlock"
// 多进程读写锁
// 采用信号量方式 映射文件共享读 若崩溃 手动删除对应文件
// 所有的信息都在map文件中 锁仅仅是互斥读写map文件而已

typedef struct s_sis_map_rwlock {
    // char        *mmapm;   // 读者数量指针 
    s_sis_sem    *slock;   // 信号量锁
    SEM_MAP_TYPE *reads;   // 读者数量
    SEM_MAP_TYPE *write;   // 写者数量 最大为 1
} s_sis_map_rwlock;

s_sis_map_rwlock *sis_map_rwlock_create(const char *rwname);

void sis_map_rwlock_destroy(s_sis_map_rwlock *rwlock);

void sis_map_rwlock_r_incr(s_sis_map_rwlock *rwlock);
void sis_map_rwlock_r_decr(s_sis_map_rwlock *rwlock);
void sis_map_rwlock_w_incr(s_sis_map_rwlock *rwlock);
void sis_map_rwlock_w_decr(s_sis_map_rwlock *rwlock);

// 通常不会只有一个锁
// 支持多个锁 总索引和每条记录可以单独锁定
// 总索引一般用 0 号记录
// 其他记录锁 用 1-N
typedef struct s_sis_map_rwlocks {
    int                  maxnum; // 设置最大数 仅仅是为了不改变mmap读计数
    char                *rwname; // 锁的头名字
    char                *mmapm;  // 读者数量指针  
    s_sis_sem           *slock;  // 信号量锁
    s_sis_map_rwlock   **locks;  // s_sis_map_rwlock
} s_sis_map_rwlocks;

s_sis_map_rwlocks *sis_map_rwlocks_create(const char *rwname, int maxnum);

void sis_map_rwlocks_destroy(s_sis_map_rwlocks *rwlocks);

int sis_map_rwlocks_r_incr(s_sis_map_rwlocks *rwlocks, int index);
int sis_map_rwlocks_r_decr(s_sis_map_rwlocks *rwlocks, int index);
int sis_map_rwlocks_w_incr(s_sis_map_rwlocks *rwlocks, int index);
int sis_map_rwlocks_w_decr(s_sis_map_rwlocks *rwlocks, int index);

s_sis_map_rwlock *sis_map_rwlocks_get(s_sis_map_rwlocks *rwlocks, int index);

void sis_map_rwlock_clear(const char *mname, int level);

#endif /* _sis_sem_h */
