#include "sis_sem.h"

s_sis_map_rwlock *sis_map_rwlock_create(const char *rwname)
{
    s_sis_map_rwlock *rwlock = SIS_MALLOC(s_sis_map_rwlock, rwlock);
    char name[255];
    sis_sprintf(name, 255, ".%s.rd", rwname);
    rwlock->rlock = sis_sem_open(name);
    if (rwlock->rlock == NULL)
    {
        goto rwlock_fail;
    }
    sis_sprintf(name, 255, ".%s.wd", rwname);
    rwlock->wlock = sis_sem_open(name);
    if (rwlock->wlock == NULL)
    {
        goto rwlock_fail;
    }

    sis_sprintf(name, 255, ".%s.rs", rwname);
    s_sis_handle fd = sis_open(name, SIS_FILE_IO_RDWR | SIS_FILE_IO_CREATE, 0666);
    if (fd == -1)
    {
        goto rwlock_fail;
    }
    if (sis_seek(fd, sizeof(int) - 1, SEEK_SET) == -1)
    {
        LOG(5) ("error stretching the file.\n");
        sis_close(fd);
        goto rwlock_fail;
    }
    // 在文件末尾写入一个空字节，扩大文件
    if (sis_write(fd, "", 1) != 1)
    {
        LOG(5)("error writing last byte of the file.\n");
        sis_close(fd);
        goto rwlock_fail;
    }
    char *map = sis_mmap_w(fd, sizeof(int));
    if (map == MAP_FAILED) 
    {
        LOG(5)("mapp file fail. %s\n", name);
        sis_close(fd);
        goto rwlock_fail;
    }
    rwlock->reads = (int *)map;
    sis_close(fd);
    return rwlock;

rwlock_fail:
    if (rwlock->rlock)
    {
        sis_sem_close(rwlock->rlock);
    }
    if (rwlock->wlock)
    {
        sis_sem_close(rwlock->wlock);
    }
    if (rwlock->reads)
    {
        munmap(rwlock->reads, sizeof(int));
    }
    sis_free(rwlock);
    return NULL;
}

void sis_map_rwlock_destroy(s_sis_map_rwlock *rwlock)
{
    sis_sem_close(rwlock->rlock);
    sis_sem_close(rwlock->wlock);
    munmap(rwlock->reads, sizeof(int));
    sis_free(rwlock);
}

void sis_map_rwlock_r_incr(s_sis_map_rwlock *rwlock)
{
    sis_sem_lock(rwlock->rlock);
    (*(rwlock->reads))++;
    if (*(rwlock->reads) == 1) 
    {
        sis_sem_lock(rwlock->wlock);  // 第一个读者进程锁住写锁
    }
    sis_sem_unlock(rwlock->rlock);
}
void sis_map_rwlock_r_decr(s_sis_map_rwlock *rwlock)
{
    sis_sem_lock(rwlock->rlock);
    (*(rwlock->reads))--;
    if (*(rwlock->reads) == 0) 
    {
        sis_sem_unlock(rwlock->wlock);   // 最后一个读者进程释放写锁
    }
    sis_sem_unlock(rwlock->rlock);
}
void sis_map_rwlock_w_incr(s_sis_map_rwlock *rwlock)
{
    sis_sem_lock(rwlock->wlock); 
}
void sis_map_rwlock_w_decr(s_sis_map_rwlock *rwlock)
{
    sis_sem_unlock(rwlock->wlock);
}


////////////////////
// s_sis_map_rwlocks 
////////////////////

s_sis_map_rwlocks *sis_map_rwlocks_create(const char *rwname, int count)
{
    if (count < 1)
    {
        count = 1;
    } 
    const char *sname = rwname ? rwname : "sismap";
    char name[255];
    sis_sprintf(name, 255, ".%s.rs", sname);
    s_sis_handle fd = sis_open(name, SIS_FILE_IO_RDWR | SIS_FILE_IO_CREATE, 0666);
    if (fd == -1)
    {
        return NULL;
    }
    if (sis_seek(fd, count * sizeof(int), SEEK_SET) == -1)
    {
        LOG(5) ("error stretching the file.\n");
        sis_close(fd);
        return NULL;
    }
    // 在文件末尾写入一个空字节，扩大文件
    if (sis_write(fd, "", 1) != 1)
    {
        LOG(5)("error writing last byte of the file.\n");
        sis_close(fd);
        return NULL;
    }
    char *map = sis_mmap_w(fd, count * sizeof(int));
    if (map == MAP_FAILED) 
    {
        LOG(5)("mapp file fail. %s\n", name);
        sis_close(fd);
        return NULL;
    }
    s_sis_map_rwlocks *rwlocks = SIS_MALLOC(s_sis_map_rwlocks, rwlocks);
    rwlocks->mmap = map;
    sis_close(fd);
    
    int slen = sis_strlen(sname);
    rwlocks->rwname = sis_malloc(slen + 1);
    memmove(rwlocks->rwname, sname, slen);
    rwlocks->rwname[slen] = 0;
    
    rwlocks->locks = sis_pointer_list_create();
    for (int i = 0; i < count; i++)
    {
        s_sis_map_rwlock *rwlock = SIS_MALLOC(s_sis_map_rwlock, rwlock);
        sis_sprintf(name, 255, ".%s.%d.rd", rwlocks->rwname, i);
        rwlock->rlock = sis_sem_open(name);
        sis_sprintf(name, 255, ".%s.%d.wd", rwlocks->rwname, i);
        rwlock->wlock = sis_sem_open(name);
        rwlock->reads = (int *)(rwlocks->mmap + i * sizeof(int));
        sis_pointer_list_push(rwlocks->locks, rwlock);
    }
    return rwlocks;
}

void sis_map_rwlocks_destroy(s_sis_map_rwlocks *rwlocks)
{
    for (int i = 0; i < rwlocks->locks->count; i++)
    {
        s_sis_map_rwlock *rwlock = sis_pointer_list_get(rwlocks->locks, i);
        sis_sem_close(rwlock->rlock);
        sis_sem_close(rwlock->wlock);
        sis_free(rwlock);
    }
    sis_pointer_list_destroy(rwlocks->locks);
    munmap(rwlocks->mmap, rwlocks->locks->count * sizeof(int));
    char name[255];
    sis_sprintf(name, 255, ".%s.rs", rwlocks->rwname);
    sis_file_delete(name);
    sis_free(rwlocks->rwname);
    sis_free(rwlocks);
}

int sis_map_rwlocks_r_incr(s_sis_map_rwlocks *rwlocks, int index)
{
    if (index >= 0 && index < rwlocks->locks->count)
    {
        s_sis_map_rwlock *rwlock = sis_pointer_list_get(rwlocks->locks, index);
        sis_map_rwlock_r_incr(rwlock);
        return 0;
    }
    return -1;
}
int sis_map_rwlocks_r_decr(s_sis_map_rwlocks *rwlocks, int index)
{
    if (index >= 0 && index < rwlocks->locks->count)
    {
        s_sis_map_rwlock *rwlock = sis_pointer_list_get(rwlocks->locks, index);
        sis_map_rwlock_r_decr(rwlock);  
        return 0;
    }
    return -1;
}
int sis_map_rwlocks_w_incr(s_sis_map_rwlocks *rwlocks, int index)
{
    if (index >= 0 && index < rwlocks->locks->count)
    {
        s_sis_map_rwlock *rwlock = sis_pointer_list_get(rwlocks->locks, index);
        sis_map_rwlock_w_incr(rwlock);   
        return 0;     
    }
    return -1;
}
int sis_map_rwlocks_w_decr(s_sis_map_rwlocks *rwlocks, int index)
{
    if (index >= 0 && index < rwlocks->locks->count)
    {
        s_sis_map_rwlock *rwlock = sis_pointer_list_get(rwlocks->locks, index);
        sis_map_rwlock_w_decr(rwlock);  
        return 0;
    }
    return -1;
}

s_sis_map_rwlock *sis_map_rwlocks_get(s_sis_map_rwlocks *rwlocks, int index)
{
    if (index >= 0 && index < rwlocks->locks->count)
    {
        return sis_pointer_list_get(rwlocks->locks, index);
    }
    return NULL;
}

int sis_map_rwlocks_incr(s_sis_map_rwlocks *rwlocks, int incrs)
{
    int agos = rwlocks->locks->count;
    LOG(5) ("start incr. %d %d \n", agos, incrs);
    for (int i = 0; i < agos; i++)
    {
        // 全锁
        sis_map_rwlocks_w_incr(rwlocks, i);
    }
    
    munmap(rwlocks->mmap, agos * sizeof(int));

    char name[255];
    sis_sprintf(name, 255, ".%s.rs", rwlocks->rwname);
    s_sis_handle fd = sis_open(name, SIS_FILE_IO_RDWR | SIS_FILE_IO_CREATE, 0666);
    if (fd == -1)
    {
        return -1;
    }
    size_t fsize = (agos + incrs) * sizeof(int);
    if (sis_seek(fd, fsize - 1, SEEK_SET) == -1)
    {
        LOG(5) ("error stretching the file.\n");
        sis_close(fd);
        return -2;
    }
    // 在文件末尾写入一个空字节，扩大文件
    if (sis_write(fd, "", 1) != 1)
    {
        LOG(5)("error writing last byte of the file.\n");
        sis_close(fd);
        return -3;
    }
    char *map = sis_mmap_w(fd, fsize);
    if (map == MAP_FAILED) 
    {
        LOG(5)("mapp file fail. %s\n", name);
        sis_close(fd);
        return -4;
    }
    rwlocks->mmap = map;
    sis_close(fd);

    for (int i = 0; i < agos; i++)
    {
        s_sis_map_rwlock *rwlock = sis_pointer_list_get(rwlocks->locks, i);
        rwlock->reads = (int *)(rwlocks->mmap + i * sizeof(int)); 
    }
    for (int i = 0; i < incrs; i++)
    {
        s_sis_map_rwlock *rwlock = SIS_MALLOC(s_sis_map_rwlock, rwlock);
        sis_sprintf(name, 255, ".%s.%d.rd", rwlocks->rwname, i + agos);
        rwlock->rlock = sis_sem_open(name);
        sis_sprintf(name, 255, ".%s.%d.wd", rwlocks->rwname, i + agos);
        rwlock->wlock = sis_sem_open(name);
        rwlock->reads = (int *)(rwlocks->mmap + (i + agos) * sizeof(int));
        sis_pointer_list_push(rwlocks->locks, rwlock);
    }
    
    for (int i = 0; i < agos; i++)
    {
        // 全解锁
        sis_map_rwlocks_w_decr(rwlocks, i);
    }
    return 0;
}

