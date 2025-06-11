#include "sis_mrwlock.h"
#include "sis_math.h"


// 初始化共享资源
s_sis_mrwlock *sis_mrwlock_create(const char *rwname)
{
    s_sis_mrwlock *rwlock = SIS_MALLOC(s_sis_mrwlock, rwlock);
    sis_strcpy(rwlock->rwname, 255, rwname);
    sis_sprintf(rwlock->shm_name, 255, "%s_%s.lock", rwname, "shm");
    sis_sprintf(rwlock->sem_metex_name, 255, "%s_%s.lock", rwname, "mutex");
    sis_sprintf(rwlock->sem_write_name, 255, "%s_%s.lock", rwname, "write");
    
    int fd = shm_open(rwlock->shm_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) 
    {
        sis_free(rwlock);
        LOG(3)("mrwlock open fail. %s\n", rwname);
        return NULL;
    }
    ftruncate(fd, sizeof(s_sis_mshare_info));
    close(fd);
    // 初始化信号量
    sem_unlink(rwlock->sem_metex_name);
    sem_unlink(rwlock->sem_write_name);
    sem_t *mutex = sem_open(rwlock->sem_metex_name, O_CREAT, 0666, 1);
    sem_t *write = sem_open(rwlock->sem_write_name, O_CREAT, 0666, 1);
    sem_close(mutex);
    sem_close(write);

    rwlock->share = sis_mrwlock_share_info(rwlock);

    return rwlock;
}

// 清理共享资源
void sis_mrwlock_destroy(s_sis_mrwlock *rwlock) 
{
    munmap(rwlock->share, sizeof(s_sis_mshare_info));
    shm_unlink(rwlock->shm_name);
    sem_unlink(rwlock->sem_metex_name);
    sem_unlink(rwlock->sem_write_name);
    sis_free(rwlock);
}

// 映射共享内存
s_sis_mshare_info *sis_mrwlock_share_info(s_sis_mrwlock *rwlock)
{
    int fd = shm_open(rwlock->shm_name, O_RDWR, 0666);
    if (fd == -1) 
    {
        LOG(3)("mrwlock open shm fail. %s %d\n", rwlock->shm_name, errno);
        return NULL;
    }
    s_sis_mshare_info *share = mmap(NULL, sizeof(s_sis_mshare_info), 
                           PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (share == MAP_FAILED) 
    {
        LOG(3)("mrwlock mmap shm fail. %s\n", rwlock->shm_name);
        return NULL;
    }
    return share;
}

// 读锁加锁
int sis_mrwlock_r_incr(s_sis_mrwlock *rwlock) 
{
    sem_t *mutex = sem_open(rwlock->sem_metex_name, 0);
    sem_t *write = sem_open(rwlock->sem_write_name, 0);
    s_sis_mshare_info *share = rwlock->share;

    // 等待写锁释放
    sem_wait(write);
    sem_post(write);

    // 增加读计数并记录PID
    sem_wait(mutex);
    int index = -1;
    for (int i = 0; i < MAX_READERS; i++) 
    {
        if (share->reader_pids[i] == 0) 
        {
            index = i;
            break;
        }
    }
    if (index == -1) 
    {
        LOG(3)("rlock fail. too many readers. %s\n", rwlock->rwname);
        return 0;
    }
    share->reader_pids[index] = getpid();
    share->reader_count++;
    share->rwlock_status = 1; // 读锁定
    sem_post(mutex);

    sem_close(mutex);
    sem_close(write);
    return 1;
}

// 读锁解锁
void sis_mrwlock_r_decr(s_sis_mrwlock *rwlock) 
{
    sem_t *mutex = sem_open(rwlock->sem_metex_name, 0);
    s_sis_mshare_info *share = rwlock->share;

    sem_wait(mutex);
    pid_t my_pid = getpid();
    for (int i = 0; i < MAX_READERS; i++) 
    {
        if (share->reader_pids[i] == my_pid) 
        {
            share->reader_pids[i] = 0;
            break;
        }
    }
    share->reader_count--;
    if (share->reader_count == 0) 
    {
        share->rwlock_status = 0; // 无锁定
    }
    sem_post(mutex);

    sem_close(mutex);
}

// 写锁加锁
int sis_mrwlock_w_incr(s_sis_mrwlock *rwlock) 
{
    sem_t *mutex = sem_open(rwlock->sem_metex_name, 0);
    sem_t *write = sem_open(rwlock->sem_write_name, 0);
    s_sis_mshare_info *share = rwlock->share;

    // 标记写进程正在等待
    sem_wait(mutex);
    share->writer_waiting = 1;
    sem_post(mutex);

    // 等待所有读进程退出
    sem_wait(write);

    // 再次检查并清理已退出的读进程
    sem_wait(mutex);
    for (int i = 0; i < MAX_READERS; i++) 
    {
        if (share->reader_pids[i] != 0) 
        {
            if (kill(share->reader_pids[i], 0) == -1 && errno == ESRCH) 
            {
                // 进程不存在，清理
                share->reader_pids[i] = 0;
                share->reader_count--;
            }
        }
    }
    // 确保没有读进程
    while (share->reader_count > 0) 
    {
        sem_post(mutex);
        usleep(10000); // 等待10毫秒
        sem_wait(mutex);
        // 再次检查并清理
        for (int i = 0; i < MAX_READERS; i++) 
        {
            if (share->reader_pids[i] != 0) {
                if (kill(share->reader_pids[i], 0) == -1 && errno == ESRCH) 
                {
                    share->reader_pids[i] = 0;
                    share->reader_count--;
                }
            }
        }
    }
    share->rwlock_status = 2; // 写锁定
    share->writer_waiting = 0;
    sem_post(mutex);
    // 这里要关闭吗
    sem_close(mutex);
    sem_close(write);
    return 1;
}

// 写锁解锁
void sis_mrwlock_w_decr(s_sis_mrwlock *rwlock) 
{
    sem_t *write = sem_open(rwlock->sem_write_name, 0);
    s_sis_mshare_info *share = rwlock->share;

    share->rwlock_status = 0; // 无锁定
    sem_post(write);

    sem_close(write);
}

// 检查并清理僵尸读进程
void sis_mrwlock_clear_reader(s_sis_mrwlock *rwlock) 
{
    sem_t *mutex = sem_open(rwlock->sem_metex_name, 0);
    s_sis_mshare_info *share = rwlock->share;

    sem_wait(mutex);
    for (int i = 0; i < MAX_READERS; i++) 
    {
        if (share->reader_pids[i] != 0) 
        {
            if (kill(share->reader_pids[i], 0) == -1 && errno == ESRCH) 
            {
                // 进程不存在，清理
                share->reader_pids[i] = 0;
                share->reader_count--;
            }
        }
    }
    sem_post(mutex);

    sem_close(mutex);
}

#if 0


// 主函数示例
int main() {
    s_sis_mrwlock *rwlock = sis_mrwlock_create("./mydb");
    sleep(10);    
    // 示例：创建多个读写进程
    pid_t pid;
    
    // 创建写进程
    if ((pid = fork()) == 0) {
        // 子进程 - 写进程
        printf("Writer process started\n");
        sis_mrwlock_w_incr(rwlock);
        printf("Writer acquired lock\n");
        // 模拟写操作
        sleep(10);
        sis_mrwlock_w_decr(rwlock);
        printf("Writer released lock\n");
        exit(EXIT_SUCCESS);
    }
    
    // 创建读进程
    for (int i = 0; i < 3; i++) 
    {
        if ((pid = fork()) == 0) 
        {
            // 子进程 - 读进程
            printf("Reader %d started\n", i);
            sis_mrwlock_r_incr(rwlock);
            printf("Reader %d acquired lock\n", i);
            if (i == 1) {
                // 模拟第二个读进程异常退出
                printf("Reader 1 exiting unexpectedly\n");
                exit(EXIT_FAILURE);
            }
            // 模拟读操作
            sleep(10);
            sis_mrwlock_r_decr(rwlock);
            printf("Reader %d released lock\n", i);
            exit(EXIT_SUCCESS);
        }
    }
    
    // 父进程等待一段时间后清理
    sleep(30);
    sis_mrwlock_clear_reader(rwlock);
    sis_mrwlock_destroy(rwlock);
    printf("Parent process exiting\n");
    
    return 0;
}
#endif