
#ifndef _sis_mrwlock_h
#define _sis_mrwlock_h

#include "os_file.h"
#include "os_thread.h"
#include "sis_list.h"
#include <sis_malloc.h>

///////////////////////
// 多进程读写锁
//////////////////////

#define READER_COUNT_OFFSET 0
#define WRITER_WAITING_OFFSET sizeof(int)
#define LOCK_STATE_OFFSET (WRITER_WAITING_OFFSET + sizeof(int))
#define PID_LIST_OFFSET (LOCK_STATE_OFFSET + sizeof(int))
#define MAX_READERS 100

// 共享内存结构
typedef struct s_sis_mshare_info
{
    int   reader_count;     // 当前读进程数量
    int   writer_waiting;   // 是否有写进程在等待
    int   rwlock_status;    // 锁状态：0-未锁定，1-读锁定，2-写锁定
    pid_t reader_pids[MAX_READERS]; // 记录所有读进程的PID
} s_sis_mshare_info;

// 多进程读写锁结构类
typedef struct s_sis_mrwlock {
    char               rwname[255];           // 锁的名字
    char               shm_name[255];         // 数据区的名字
    char               sem_metex_name[255];   // 信号互斥的名字
    char               sem_write_name[255];   // 写的名字
    s_sis_mshare_info *share;           // 共享资源
} s_sis_mrwlock;

// 初始化共享资源
s_sis_mrwlock *sis_mrwlock_create(const char *rwname);

// 清理共享资源
void sis_mrwlock_destroy(s_sis_mrwlock *);

s_sis_mshare_info *sis_mrwlock_share_info(s_sis_mrwlock *);

// 读锁加锁
int sis_mrwlock_r_incr(s_sis_mrwlock *rwlock);
// 读锁解锁
void sis_mrwlock_r_decr(s_sis_mrwlock *rwlock);
// 写锁加锁
int sis_mrwlock_w_incr(s_sis_mrwlock *rwlock);
// 写锁解锁
void sis_mrwlock_w_decr(s_sis_mrwlock *rwlock);

// 检查并清理僵尸读进程
void sis_mrwlock_clear_reader(s_sis_mrwlock *);


#endif /* _sis_sem_h */
