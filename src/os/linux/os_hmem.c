
#include <os_hmem.h>
#include <os_file.h>

// 注意 这里的内存申请 不加 sis 前缀

#define HMEM_SHOWINFO
#define HMEM_SUMMARY

#ifdef HMEM_SUMMARY

typedef struct s_hmem_summary
{	
	msec_t  start_usec;   // sis_time_get_now_usec();
	msec_t  ago_usec;   // sis_time_get_now_usec();

    int64   mall_nums;
    int64   mall_size;
    int64   free_nums;
    int64   free_size;

    int64   cur_mall_nums;
    int64   cur_mall_size;
    int64   cur_free_nums;
    int64   cur_free_size;
} s_hmem_summary;

s_hmem_summary _hmemsum;

#endif
/////////////////////////////////////
//
//////////////////////////////////////////
s_sis_hmem *sis_hmem_create(int64 msize)
{
    int64 SIS_HEME_DEFSIZE = (long long)32*1024*1024*1024;    // 32G
    int64 SIS_HEME_MAXSIZE = (long long)1024*1024*1024*1024;  // 1T
    int64 SIS_HEME_MINSIZE = (long long)1024*1024;            // 1M

    s_sis_hmem *hmem = malloc(sizeof(s_sis_hmem));

    sis_mutex_create(&hmem->mutex);

    if (msize <= 0)
    {
        msize = SIS_HEME_DEFSIZE;
    }
    if (msize < SIS_HEME_MINSIZE)
    {
        msize = SIS_HEME_MINSIZE;
    }
    if (msize > SIS_HEME_MAXSIZE)
    {
        msize = SIS_HEME_MAXSIZE;
    }
    hmem->msize = msize / 4096 * 4096; // 对齐
    
    pid_t pid = getpid();
    char mfname[64];
    sprintf(mfname, ".h%d.mem", pid);

    hmem->memmap = sis_mmap_open_w(mfname, hmem->msize);
    if (hmem->memmap)
    {
        hmem->usbale = malloc(sizeof(s_hmem_node));
        hmem->usbale->head =  0;
        hmem->usbale->tail =  hmem->msize / SIS_HEME_BLKSIZE - 1;
        hmem->usbale->next =  0;
        hmem->usbale->prev =  0;
        for (int i = 0; i < 32; i++)
        {
            hmem->indexs[i].blks = 0;
            hmem->indexs[i].size = 0;
        }
    }
    else
    {
        return NULL;
    }
#ifdef HMEM_SUMMARY
    memset(&_hmemsum, 0, sizeof(s_hmem_summary));
    _hmemsum.start_usec = sis_time_get_now_usec();
    _hmemsum.ago_usec = sis_time_get_now_usec();
#endif
    return hmem;
}
void sis_hmem_exit(s_sis_hmem *hmem)
{
    if (hmem->memmap)
    {
        sis_unmmap(hmem->memmap, hmem->msize);
        pid_t pid = getpid();
        char mfname[64];
        sprintf(mfname, ".h%d.mem", pid);
        sis_file_delete(mfname);
    }
    sis_mutex_destroy(&hmem->mutex);
    free(hmem);
}
void sis_hmem_destroy(s_sis_hmem *hmem)
{
    sis_mutex_lock(&hmem->mutex);
    if (hmem->usbale)
    {
        s_hmem_node *next = hmem->usbale;
        while (next)
        {
            s_hmem_node *prev = next;
            next = next->next;
            free(prev);	
        }
        hmem->usbale = NULL;
    }
    sis_mutex_unlock(&hmem->mutex);
    sis_hmem_exit(hmem);
}
#ifdef HMEM_SHOWINFO
void sis_hmem_show_nodes(s_sis_hmem *hmem, const char *info)
{
    // printf("==========\n");
    // {
    //     s_hmem_node *next = hmem->usbale;
    //     while (next)
    //     {
    //         printf("%s usable: %5lld %5lld %20p %20p %20p\n", info, next->head, next->tail, next->prev, next, next->next);
    //         next = next->next;
    //     }
    // }
#ifdef HMEM_SUMMARY
    if (_hmemsum.mall_nums % 10000 == 0)
    {
        msec_t curusec = sis_time_get_now_usec();
        printf("%6lld %10lld %10lld %10lld %10lld %10.2f| %10lld %10lld %10lld %10lld %10.2f\n", curusec - _hmemsum.ago_usec, 
            _hmemsum.mall_nums, _hmemsum.mall_size, 
            _hmemsum.free_nums, _hmemsum.free_size,
            _hmemsum.mall_nums > 0 ? (double)_hmemsum.mall_size / _hmemsum.mall_nums : 0.0,
            _hmemsum.cur_mall_nums, _hmemsum.cur_mall_size, 
            _hmemsum.cur_free_nums, _hmemsum.cur_free_size,
            _hmemsum.cur_mall_nums > 0 ? (double)_hmemsum.cur_mall_size / _hmemsum.cur_mall_nums : 0.0);
        _hmemsum.cur_mall_nums = 0; 
        _hmemsum.cur_mall_size = 0;
        _hmemsum.cur_free_nums = 0;
        _hmemsum.cur_free_size = 0;
        _hmemsum.ago_usec = curusec;
    }
#endif

}
#endif
void *_hmem_malloc(s_sis_hmem *hmem, int64 nsize)
{
    char *rmem = NULL;
    s_hmem_head mhead;
    mhead.size = nsize;
    mhead.blks = (nsize + sizeof(s_hmem_head)) / SIS_HEME_BLKSIZE;
    mhead.blks = mhead.blks * SIS_HEME_BLKSIZE < (nsize + sizeof(s_hmem_head)) ? mhead.blks + 1 : mhead.blks;
    
    s_hmem_node *next = hmem->usbale;
    while (next)
    {
        int64 size = (next->tail - next->head + 1) * SIS_HEME_BLKSIZE - sizeof(s_hmem_head);
        if (size >= nsize)
        {
            // 赋值
            rmem = hmem->memmap + next->head * SIS_HEME_BLKSIZE;
            memmove(rmem, &mhead, sizeof(s_hmem_head));
            // printf("==3== %lld %p\n", blks, hmem->usbale);
            if (size == nsize)
            {
                // 从 usable 中清理
                if (next->prev == NULL)
                {
                    hmem->usbale = next->next;
                    hmem->usbale->prev = NULL;
                }
                else
                {
                    next->prev->next = next->next;
                    if (next->next)
                    {
                        next->next->prev = next->prev;
                    }
                }
                free(next);
            }
            else // if (size > nsize)
            {
                // 修改头指针
                next->head = next->head + mhead.blks;  
                // printf("==3== %lld %p\n", blks, hmem->usbale);
            }
            break;
        }
        next = next->next;
    }
    if (rmem)
    {
#ifdef HMEM_SHOWINFO
#ifdef HMEM_SUMMARY
    _hmemsum.mall_nums++;
    _hmemsum.mall_size+=nsize;
    _hmemsum.cur_mall_nums++;
    _hmemsum.cur_mall_size+=nsize;
#endif

    sis_hmem_show_nodes(hmem, "malloc");
#endif
        return rmem + sizeof(s_hmem_head);
    }
    else
    {
        printf("no map malloc.\n");
    }
    return NULL;  
}
void *sis_hmem_malloc(s_sis_hmem *hmem, int64 nsize)
{
    sis_mutex_lock(&hmem->mutex);
    void *rmem = _hmem_malloc(hmem, nsize);
    sis_mutex_unlock(&hmem->mutex);
    return rmem;  
}
void *sis_hmem_calloc(s_sis_hmem *hmem, int64 nsize)
{
    sis_mutex_lock(&hmem->mutex);
    char *rmem = _hmem_malloc(hmem, nsize);
    if (rmem)
    {
        memset(rmem, 0, nsize);
    }
    sis_mutex_unlock(&hmem->mutex);
    return rmem;
}
void *sis_hmem_realloc(s_sis_hmem *hmem, void *ptr, int64 nsize)
{
    // 先发现
    char *rmem = NULL;
    sis_mutex_lock(&hmem->mutex);
    s_hmem_head *ohead = (s_hmem_head *)((char *)ptr - sizeof(s_hmem_head));
    if (ohead->size >= 0)
    {
        rmem = _hmem_malloc(hmem, nsize);
        // 这里可以做优化
        memmove(rmem, ohead->data, ohead->size);
        sis_hmem_recy_occupy(hmem, ohead);
    }
    else
    {
        rmem = _hmem_malloc(hmem, nsize);
    }
    sis_mutex_unlock(&hmem->mutex);
    return rmem;
}

void sis_hmem_free(s_sis_hmem *hmem, void *ptr)
{
    sis_mutex_lock(&hmem->mutex);
    s_hmem_head *ohead = (s_hmem_head *)((char *)ptr - sizeof(s_hmem_head));
    size_t size = ohead->size;
    if (size >= 0)
    {
        // 回收
        sis_hmem_recy_occupy(hmem, ohead);
    }
    sis_mutex_unlock(&hmem->mutex);
#ifdef HMEM_SHOWINFO
#ifdef HMEM_SUMMARY
    _hmemsum.free_nums++;
    _hmemsum.free_size+=size;
    _hmemsum.cur_free_nums++;
    _hmemsum.cur_free_size+=size;
#endif
    sis_hmem_show_nodes(hmem, " free ");
#endif
}

void sis_hmem_recy_occupy(s_sis_hmem *hmem, s_hmem_head *ohead)
{
    s_hmem_node *node = malloc(sizeof(s_hmem_node));
    
    node->head = ((char *)ohead - hmem->memmap) / SIS_HEME_BLKSIZE;
    node->tail = node->head + ohead->blks - 1;
    node->next = NULL;
    node->prev = NULL;
    // 遍历 usbale 得到可插入的位置
    
    sis_hmem_incr_usable(hmem, node); 

    // ====== ok ==========
    // 设置删除标记
    ohead->blks =  0;
    ohead->size =  0;
}
void sis_hmem_incr_usable(s_sis_hmem *hmem, s_hmem_node *node)
{
    // 这里要判断是否有连续块 有连续块就合并
    s_hmem_node *head_node = NULL; // 和现有头部相连
    s_hmem_node *tail_node = NULL; // 和现有尾部相连
    s_hmem_node *incr_node = NULL; // 插入该节点之前
    s_hmem_node *next = hmem->usbale;    
    while (next)
    {
        if (node->head == next->tail + 1)
        {
            tail_node = next;
        }
        if (node->tail + 1 == next->head)
        {
            head_node = next;
        }
        // printf("== %lld %lld\n", node->head, next->head);
        if (node->head < next->head)
        {
            incr_node = next;
            break;
        }
        next = next->next;
    }
    if (head_node || tail_node)
    {
        node->tail = head_node ? head_node->tail : node->tail;
        node->head = tail_node ? tail_node->head : node->head;
        if (tail_node && !head_node) 
        {
            // 替代插入之后
            sis_hmem_swap_node(hmem, node, tail_node);
        }
        if (head_node && !tail_node) 
        {
            // 替代插入之前
            sis_hmem_swap_node(hmem, node, head_node);
        }
        if (head_node && tail_node) 
        {
            node->next = head_node->next;
            node->prev = tail_node->prev;
            if (node->prev == NULL)
            {
                hmem->usbale = node;
            }
            else
            {
                node->prev->next = node;
            }
            if (node->next)
            {
                node->next->prev = node;
            }
            free(head_node);
            free(tail_node);
        }
    }
    else
    {
        // 插入 incr_node 之前
        if (incr_node)
        {
            // 从排好的位置写
            node->next = incr_node;
            node->prev = incr_node->prev;
            if (!incr_node->prev)
            {
                hmem->usbale = node;
            }
            else
            {
                incr_node->prev->next = node;
            }
            incr_node->prev = node;
        }
        else
        {
            printf("no exec there.\n");
            // 最大从尾写
        }
    }
}
void sis_hmem_swap_node(s_sis_hmem *hmem, s_hmem_node *nnode, s_hmem_node *fnode)
{
    nnode->next = fnode->next;
    nnode->prev = fnode->prev;
    if (nnode->prev == NULL)
    {
        hmem->usbale = nnode;
    }
    else
    {
        nnode->prev->next = nnode;
    }
    if (nnode->next)
    {
        nnode->next->prev = nnode;
    }
    free(fnode);
}


#if 0

int main()
{
	s_sis_hmem *hm = sis_hmem_create(1024*1024);
	
    char *m1 = sis_hmem_malloc(hm, 32*2);
    memmove(m1, "my is m1", 9);

    char *m2 = sis_hmem_malloc(hm, 32*4);
    memmove(m2, "my is m2", 9);

    char *m3 = sis_hmem_malloc(hm, 32*8);
    memmove(m3, "my is m3", 9);

    sis_hmem_free(hm, m2);

    char *m4 = sis_hmem_malloc(hm, 32);
    memmove(m4, "my is m4", 9);

    char *m5 = sis_hmem_malloc(hm, 64);
    memmove(m5, "my is m5", 9);

    sis_hmem_free(hm, m4);
    sis_hmem_free(hm, m3);
    
    sis_hmem_free(hm, m1);
    sis_hmem_free(hm, m5);

	printf("end . \n");

    // sis_hmem_destroy(hm);
	return 1;
}
#endif
