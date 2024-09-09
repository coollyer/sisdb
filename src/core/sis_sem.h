
#ifndef _sis_sem_h
#define _sis_sem_h

#include "os_file.h"
#include "os_thread.h"
#include "sis_list.h"
#include <sis_malloc.h>

// 多进程读写锁
// 采用信号量方式 映射文件共享读 若崩溃 手动删除对应文件
typedef struct s_sis_map_rwlock {
    int       *reads;   // 读者数量指针   
    s_sis_sem *rlock;   // 读锁列
    s_sis_sem *wlock;   // 写锁列
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
    char                *rwname; // 锁的头名字
    char                *mmap;   // 读者数量指针  
    s_sis_pointer_list  *locks;  // s_sis_map_rwlock
} s_sis_map_rwlocks;

s_sis_map_rwlocks *sis_map_rwlocks_create(const char *rwname, int count);

void sis_map_rwlocks_destroy(s_sis_map_rwlocks *rwlocks);

int sis_map_rwlocks_r_incr(s_sis_map_rwlocks *rwlocks, int index);
int sis_map_rwlocks_r_decr(s_sis_map_rwlocks *rwlocks, int index);
int sis_map_rwlocks_w_incr(s_sis_map_rwlocks *rwlocks, int index);
int sis_map_rwlocks_w_decr(s_sis_map_rwlocks *rwlocks, int index);

s_sis_map_rwlock *sis_map_rwlocks_get(s_sis_map_rwlocks *rwlocks, int index);

// 临时增加锁
// 不建议在频繁读写操作时执行 切记 
int sis_map_rwlocks_incr(s_sis_map_rwlocks *rwlocks, int incrs);

#endif /* _sis_sem_h */
