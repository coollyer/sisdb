#include "sis_sem.h"
#include "sis_math.h"

static size_t sis_map_rwlock_get_fsize(int nums)
{
    return SIS_ZOOM_UP(2 * sizeof(SEM_MAP_TYPE), 4096);
}
s_sis_map_rwlock *sis_map_rwlock_create(const char *rwname)
{
#ifndef SEM_DEBUG
    char name[255];
    sis_sprintf(name, 255, ".%s.sem", rwname);
    s_sis_sem *slock = sis_sem_open(name);
    if (slock == NULL)
    {
        printf("sis_sem_open fail : %d\n", errno);
        return NULL;
    }
    sis_sprintf(name, 255, ".%s.sem", rwname);
    char *mmapm = sis_mmap_open_w(name, sis_map_rwlock_get_fsize(1));
    if (!mmapm)
    {
        sis_sem_close(slock);
        printf("sis_mmap_open_w fail : %d\n", errno);
        return NULL;
    }
    s_sis_map_rwlock *rwlock = SIS_MALLOC(s_sis_map_rwlock, rwlock);
    rwlock->slock = slock;
    // rwlock->mmapm = mmapm;
    rwlock->reads = (SEM_MAP_TYPE *)(mmapm);
    rwlock->write = (SEM_MAP_TYPE *)(mmapm + sizeof(SEM_MAP_TYPE));
#else
    s_sis_map_rwlock *rwlock = SIS_MALLOC(s_sis_map_rwlock, rwlock);
#endif
    return rwlock;
}

void sis_map_rwlock_destroy(s_sis_map_rwlock *rwlock)
{
#ifndef SEM_DEBUG
    sis_sem_close(rwlock->slock);
    sis_unmmap((char *)rwlock->reads, sis_map_rwlock_get_fsize(1));
#endif
    sis_free(rwlock);
}

void sis_map_rwlock_r_incr(s_sis_map_rwlock *rwlock)
{
#ifndef SEM_DEBUG
    int success = 0;
    while(!success)
    {
        // printf("-1- %d %d\n", *(rwlock->reads), *(rwlock->write)); 
        sis_sem_lock(rwlock->slock);
        if ((*(rwlock->write)) == 0 && (*(rwlock->reads)) < SEM_MAP_MAXI)
        {
            (*(rwlock->reads))++;
            success = 1;
        }
        // printf("-2- %d %d\n", *(rwlock->reads), *(rwlock->write)); 
        sis_sem_unlock(rwlock->slock); 
        // printf("-3- %d %d\n", *(rwlock->reads), *(rwlock->write)); 
        if (!success)
        {
            sis_usleep(100);
        }
    }
#endif
}

void sis_map_rwlock_r_decr(s_sis_map_rwlock *rwlock)
{
#ifndef SEM_DEBUG
    sis_sem_lock(rwlock->slock);
    if ((*(rwlock->reads)) > 0)
    {
        // printf("-- %d %d\n", *(rwlock->reads), *(rwlock->write));
        (*(rwlock->reads))--;
        // printf("--%d %d\n", *(rwlock->reads), *(rwlock->write));
    }
    sis_sem_unlock(rwlock->slock);  
#endif
}
void sis_map_rwlock_w_incr(s_sis_map_rwlock *rwlock)
{
#ifndef SEM_DEBUG
    int success = 0;
    while(!success)
    {
        sis_sem_lock(rwlock->slock);
        // printf("%d %d\n", *(rwlock->reads), *(rwlock->write));
        if ((*(rwlock->reads)) == 0 && (*(rwlock->write)) == 0)
        {
            (*(rwlock->write))++;
            success = 1;
        }
        // printf("%d %d\n", *(rwlock->reads), *(rwlock->write));
        sis_sem_unlock(rwlock->slock);  
        if (!success)
        {
            sis_usleep(100);
        }
    }
#endif
}
void sis_map_rwlock_w_decr(s_sis_map_rwlock *rwlock)
{
#ifndef SEM_DEBUG
    sis_sem_lock(rwlock->slock);
    if ((*(rwlock->write)) > 0)
    {
        (*(rwlock->write))--;
    }
    sis_sem_unlock(rwlock->slock);  
#endif
}


////////////////////
// s_sis_map_rwlocks 
////////////////////

s_sis_map_rwlocks *sis_map_rwlocks_create(const char *rwname, int maxnum)
{
    if (maxnum < 1)
    {
        maxnum = 1;
    } 
    int fsize = sis_map_rwlock_get_fsize(maxnum);
    const char *sname = rwname ? rwname : "sismap";
    
    char name[255];
    sis_sprintf(name, 255, ".%s.msem", rwname);
    s_sis_sem *slock = sis_sem_open(name);
    if (slock == NULL)
    {
        printf("sis_sem_open fail : %d\n", errno);
        return NULL;
    }
    sis_sprintf(name, 255, ".%s.msem", sname);
    char *mmapm = sis_mmap_open_w(name, fsize);
    if (!mmapm)
    {
        sis_sem_close(slock);
        printf("sis_mmap_open_w fail : %d\n", errno);
        return NULL;
    }
    s_sis_map_rwlocks *rwlocks = SIS_MALLOC(s_sis_map_rwlocks, rwlocks);
    rwlocks->slock  = slock;
    rwlocks->mmapm  = mmapm;
    rwlocks->maxnum = maxnum;
    
    int slen = sis_strlen(sname);
    rwlocks->rwname = sis_malloc(slen + 1);
    memmove(rwlocks->rwname, sname, slen);
    rwlocks->rwname[slen] = 0;
    
    rwlocks->locks = (s_sis_map_rwlock **)sis_malloc(rwlocks->maxnum * sizeof(void *));
    memset(rwlocks->locks, 0, rwlocks->maxnum * sizeof(void *));
    return rwlocks;
}

void sis_map_rwlocks_destroy(s_sis_map_rwlocks *rwlocks)
{
    sis_sem_close(rwlocks->slock);
    int fsize = sis_map_rwlock_get_fsize(rwlocks->maxnum);
    sis_unmmap(rwlocks->mmapm, fsize);
    sis_free(rwlocks->locks);
    sis_free(rwlocks->rwname);
    sis_free(rwlocks);
}
int sis_map_rwlocks_check(s_sis_map_rwlocks *rwlocks, int index)
{
    if (index < 0 || index >= rwlocks->maxnum)
    {
        return 0;
    } 
    if (rwlocks->locks[index])
    {
        return 1;
    }
    // 这里要增加数据
    rwlocks->locks[index] = SIS_MALLOC(s_sis_map_rwlock, rwlocks->locks[index]);
    rwlocks->locks[index]->slock = rwlocks->slock;
    int offset = index * 2 * sizeof(SEM_MAP_TYPE);
    rwlocks->locks[index]->reads = (SEM_MAP_TYPE *)(rwlocks->mmapm + offset);
    rwlocks->locks[index]->write = (SEM_MAP_TYPE *)(rwlocks->mmapm + offset + sizeof(SEM_MAP_TYPE));
    return 1;
}
int sis_map_rwlocks_r_incr(s_sis_map_rwlocks *rwlocks, int index)
{
    if (sis_map_rwlocks_check(rwlocks, index))
    {
        sis_map_rwlock_r_incr(rwlocks->locks[index]);
        return 0;
    }
    return -1;
}
int sis_map_rwlocks_r_decr(s_sis_map_rwlocks *rwlocks, int index)
{
    if (sis_map_rwlocks_check(rwlocks, index))
    {
        sis_map_rwlock_r_decr(rwlocks->locks[index]);
        return 0;
    }
    return -1;
}
int sis_map_rwlocks_w_incr(s_sis_map_rwlocks *rwlocks, int index)
{
    if (sis_map_rwlocks_check(rwlocks, index))
    {
        sis_map_rwlock_w_incr(rwlocks->locks[index]);
        return 0;     
    }
    return -1;
}
int sis_map_rwlocks_w_decr(s_sis_map_rwlocks *rwlocks, int index)
{
    if (sis_map_rwlocks_check(rwlocks, index))
    {
        sis_map_rwlock_w_decr(rwlocks->locks[index]);
        return 0;
    }
    return -1;
}

s_sis_map_rwlock *sis_map_rwlocks_get(s_sis_map_rwlocks *rwlocks, int index)
{
    if (sis_map_rwlocks_check(rwlocks, index))
    {
        return rwlocks->locks[index];
    }
    return NULL;
}

void sis_map_rwlock_clear(const char *mname, int level)
{
    if (level == 0)
    {
        s_sis_handle fd = sis_open(mname, SIS_FILE_IO_RDWR, 0666);
        if (fd == -1)
        {
            return ;
        }
		struct stat stat;
		if (fstat(fd, &stat) == -1) 
        {
			printf("error getting file status.\n");
			sis_close(fd);
			return ;
		}
		size_t fsize = stat.st_size;
        char *mmapm = sis_mmap_w(fd, fsize);
        if (!mmapm)
        {
            printf("sis_mmap_w fail : %d\n", errno);
            sis_close(fd);
            return ;
        }
        sis_close(fd);
        // printf("====1===%zu \n", fsize);
        // SEM_MAP_TYPE *ws = mmapm + 3;
        // *ws = 0;
        memset(mmapm , 0, fsize);
        // printf("====2===\n");
        // sis_mmap_sync(mmapm, fsize);
        sis_unmmap(mmapm, fsize);
        // printf("====3===\n");
	}
    if (level == 1)
    {
        sem_unlink(mname);
        sis_file_delete(mname);
    }
}

#if 0

int main1()
{
    s_sis_map_rwlock *rwl = sis_map_rwlock_create("mymap3");

    printf("r lock ...\n");
    sis_map_rwlock_r_incr(rwl);
    printf("r lock ok\n");
    sis_map_rwlock_r_decr(rwl);
    printf("r lock free\n");

    printf("w lock ...\n");
    sis_map_rwlock_w_incr(rwl);
    return 1;
    printf("w lock ok\n");
    sis_map_rwlock_w_decr(rwl);
    printf("w lock free\n");

    sis_map_rwlock_destroy(rwl);
}

int main(int argc, char **argv)
{
    if (argc > 1 && argv[1][0] == 'f')
    {
        sis_map_rwlock_clear(".mymap4.msem", 0);
        // sem_unlink(".mymap4.msem");
        // sis_file_delete(".mymap4.msem");
        return 0;
    }
    s_sis_map_rwlocks *rwls = sis_map_rwlocks_create("mymap4", 1000);

    s_sis_map_rwlock *rwl = sis_map_rwlocks_get(rwls, 1);
    int count = 1;
    if (argc > 1 && argv[1][0] == 'w')
    {
        count = 10000000;
    }
    for (int i = 0; i < count; i++)
    {
        printf("r lock ... %d %p %p %d\n", i, rwl, rwls, rwls->maxnum);
        sis_map_rwlock_r_incr(rwl);
        printf("r lock ok\n");
        sis_map_rwlock_r_decr(rwl);
        printf("r lock free\n");
        sleep(1);
    }
    

    printf("w lock ...\n");
    sis_map_rwlock_w_incr(rwl);
    if (argc > 1 && argv[1][0] == 'c')
    {
        // 中断
        return 1;
    }

    printf("w lock ok\n");
    sis_map_rwlock_w_decr(rwl);
    printf("w lock free\n");

    // int index = 4;
    // printf("r lock ...\n");
    // sis_map_rwlocks_r_incr(rwls, index);
    // printf("r lock ok\n");
    // sis_map_rwlocks_r_decr(rwls, index);
    // printf("r lock free\n");

    // printf("w lock ...\n");
    // sis_map_rwlocks_w_incr(rwls, index);
    // // return 1;
    // printf("w lock ok\n");
    // sis_map_rwlocks_w_decr(rwls, index);
    // printf("w lock free\n");

    sis_map_rwlocks_destroy(rwls);
}
#endif