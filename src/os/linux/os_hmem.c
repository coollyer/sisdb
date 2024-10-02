
#include <os_hmem.h>
#include <os_file.h>

// 注意 这里的内存申请 不加 sis 前缀

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
    }
    else
    {
        return NULL;
    }
    return hmem;
}
void sis_hmem_destroy(s_sis_hmem *hmem)
{
    if (hmem->usbale)
    {
        s_hmem_node *next = hmem->usbale;
        while (next)
        {
            s_hmem_node *prev = next;
            next = next->next;
            free(prev);	
        }
    }
    if (hmem->occupy)
    {
        s_hmem_node *next = hmem->occupy;
        while (next)
        {
            s_hmem_node *prev = next;
            next = next->next;
            free(prev);	
        }
    }
    if (hmem->memmap)
    {
        sis_unmmap(hmem->memmap, hmem->msize);
    }
    sis_mutex_destroy(&hmem->mutex);
    free(hmem);
}
// 缩减后向前找位置
void sis_hmem_incr_usable_prev(s_sis_hmem *hmem, s_hmem_node *node)
{
    s_hmem_node *next = node->prev;
    while (next)
    {
        // if (next != node) // 不可能
        {
            if (node->tail - node->head >= next->tail - next->head)
            {
                if (node->prev != next)
                {
                    node->prev->next = node->next;
                    node->next = next->next;
                    node->prev = next;
                    next->next = node;
                }
                break;
            }
            if (next->prev == NULL)
            {
                node->prev->next = node->next;
                node->prev = NULL;
                node->next = hmem->usbale;
                hmem->usbale->prev = node;
                hmem->usbale = node;
                break;
            }
        }
        next = next->prev;
    }
}
// 扩容后向后找位置
void sis_hmem_incr_usable_next(s_sis_hmem *hmem, s_hmem_node *node)
{
    s_hmem_node *next = node->next;
    while (next)
    {
        if (node->tail - node->head <= next->tail - next->head)
        {
            if (node->next != next)
            {
                // 位置一定有变化 先删除
                sis_hmem_decr_usable(hmem, node);
                // 再插入
                node->next = next;
                node->prev = next->prev;
                if (next->prev)
                {
                    next->prev->next = node;
                }
                next->prev = node;
            }
            return ;
        }
        if (next->next == NULL)
        {
            if (node->next != NULL)
            {
                node->prev->next = node->next;
                next->next = node;
                node->next = NULL;
                node->prev = next;
            }
            break;
        }
        next = next->next;
    }
}
void sis_hmem_show_nodes(s_sis_hmem *hmem, const char *info)
{
    printf("==========\n");
    {
        s_hmem_node *next = hmem->usbale;
        while (next)
        {
            printf("%s usable: %5lld %5lld %20p %20p %20p\n", info, next->head, next->tail, next->prev, next, next->next);
            next = next->next;
        }
    }
    {
        s_hmem_node *next = hmem->occupy;
        while (next)
        {
            printf("%s occupy: %5lld %5lld %20p %20p %20p\n", info, next->head, next->tail, next->prev, next, next->next);
            next = next->next;
        }
    }
}

void *_hmem_malloc(s_sis_hmem *hmem, int64 nsize)
{
    void *rmem = NULL;
    s_hmem_node *newnode = malloc(sizeof(s_hmem_node));
    newnode->head = -1;
    newnode->tail = -1;
    newnode->next = NULL;
    newnode->prev = NULL;

    int64 blks = nsize / SIS_HEME_BLKSIZE;
    blks = blks * SIS_HEME_BLKSIZE < nsize ? blks + 1 : blks;
    // printf("==1== %lld %p\n", blks, hmem->usbale);
    
    s_hmem_node *next = hmem->usbale;
    while (next)
    {
        int64 size = (next->tail - next->head + 1) * SIS_HEME_BLKSIZE;
        if (size >= nsize)
        {
            // 可用
            newnode->head = next->head;
            newnode->tail = next->head + blks - 1;
            // 添加到 occupy
            sis_hmem_incr_occupy(hmem, newnode);
            // printf("==3== %lld %p\n", blks, hmem->usbale);
            if (size == nsize)
            {
                // 从 usable 中清理
                sis_hmem_decr_usable(hmem, next);
                free(next);
            }
            else // if (size > nsize)
            {
                // 修改头指针
                next->head = newnode->tail + 1;  // 需要重排序
                // 缩减后向前找位置
                sis_hmem_incr_usable_prev(hmem, next);
                // printf("==3== %lld %p\n", blks, hmem->usbale);
            }
            break;
        }
        next = next->next;
    }
    // sis_hmem_show_nodes(hmem, "malloc");
    if (newnode->head != -1)
    {
        rmem = hmem->memmap + newnode->head * SIS_HEME_BLKSIZE;
    }
    return rmem;  
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
    s_hmem_node *node = sis_hmem_find_occupy(hmem, ptr);
    if (node)
    {
        rmem = _hmem_malloc(hmem, nsize);
        // 这里可以做优化
        memmove(rmem, ptr, nsize);
        sis_hmem_decr_occupy(hmem, node);
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
    s_hmem_node *node = sis_hmem_find_occupy(hmem, ptr);
    if (node)
    {
        sis_hmem_decr_occupy(hmem, node);
    }
    sis_mutex_unlock(&hmem->mutex);
    // sis_hmem_show_nodes(hmem, "free");
}
int  sis_hmem_isnil(s_sis_hmem *hmem)
{
    return 0;
}

s_hmem_node *sis_hmem_find_occupy(s_sis_hmem *hmem, void *ptr)
{
    // 定位到块号
    uint64 head = (uint64)((char *)ptr - hmem->memmap) / SIS_HEME_BLKSIZE;
    // 寻址
    s_hmem_node *node = NULL;
    s_hmem_node *next = hmem->occupy;
    while (next)
    {
        if (next->head == head)
        {
            node = next;
            break;
        }
        next = next->next;
    }
    return node;
}
void sis_hmem_incr_occupy(s_sis_hmem *hmem, s_hmem_node *node)
{
    // 新增的默认放最前 多数操作需求
    // 加锁
    if (hmem->occupy != NULL)
    {
        hmem->occupy->prev = node;
        node->next = hmem->occupy;
    }
    hmem->occupy = node;
}
void sis_hmem_decr_occupy(s_sis_hmem *hmem, s_hmem_node *node)
{
    // 到这里 node 一定有值
    if (node->next)
    {
        node->next->prev = node->prev;
    }
    if (node->prev == NULL)
    {
        hmem->occupy = node->next;
        if (hmem->occupy)
        {
            hmem->occupy->prev = NULL;
        }
    }
    else
    {
        node->prev->next = node->next;
    }
    // 把释放的node返回 usable
    sis_hmem_incr_usable(hmem, node); 
}
void sis_hmem_incr_usable(s_sis_hmem *hmem, s_hmem_node *node)
{
    // 这里要判断是否有连续块 有连续块就合并
    int64 curblk = node->tail - node->head; 
    
    s_hmem_node *incr_node = NULL;  // 插入哪个节点的前面 NULL 是头部
    s_hmem_node *head_node = NULL; 
    s_hmem_node *tail_node = NULL; 

    s_hmem_node *next = hmem->usbale;    
    while (next)
    {
        if (node->head == next->tail + 1)
        {
            tail_node = next;
            // 和其他的尾部相连
            sis_hmem_decr_usable(hmem, next);
        }
        if (node->tail + 1 == next->head)
        {
            head_node = next;
            // 和其他的头部相连  
            sis_hmem_decr_usable(hmem, next);
        }
        if (curblk > next->tail - next->head)
        {
            incr_node = next;
        }
        next = next->next;
    }
    if (tail_node || head_node) 
    {
        node->tail = head_node ? head_node->tail : node->tail;
        node->head = tail_node ? tail_node->head : node->head;
        sis_hmem_incr_usable(hmem, node);
        if (tail_node) 
        {
            free(tail_node);
        }
        if (head_node) 
        {
            free(head_node);
        }
    } 
    else
    {
        if (incr_node == NULL)
        {
            // 最小从头写
            node->prev = NULL;
            node->next = hmem->usbale;
            if (hmem->usbale)
            {
                hmem->usbale->prev = node;
            }
            hmem->usbale = node;
        }
        else
        {
            // 从排好的位置写
            node->prev = incr_node;
            node->next = incr_node->next;
            incr_node->next = node;
        }
    }  
}
void sis_hmem_decr_usable(s_sis_hmem *hmem, s_hmem_node *node)
{
    if (node->prev == NULL)
    {
        hmem->usbale = node->next;
        if (hmem->usbale)
        {
            hmem->usbale->prev = NULL;
        }
    }
    else
    {
        node->prev->next = node->next;
    }
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

    sis_hmem_free(hm, m3);
    sis_hmem_free(hm, m4);
    
    sis_hmem_free(hm, m1);
    sis_hmem_free(hm, m5);

	printf("end . \n");

    // sis_hmem_destroy(hm);
	return 1;
}
#endif
