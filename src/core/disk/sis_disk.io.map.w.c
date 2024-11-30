#include "sis_disk.io.map.h"

////////////////////
// write 
////////////////////

int sis_disk_io_map_w_open(s_sis_map_fctrl *fctrl, const char *fpath_, const char *fname_)
{
    fctrl->rwmode = 1;
    
    if (!sis_file_exists(fctrl->fname))
    {
        LOG(5)("no map file,wait create.[%s]\n", fctrl->fname);
        fctrl->status |= SIS_MAP_STATUS_NOFILE;
        return 1;
    }
    else
    {
        if (!sis_path_exists(fctrl->fname))
        {
            sis_path_mkdir(fctrl->fname);
        }
        fctrl->mapmem = sis_mmap_open_w(fctrl->fname, 0);
        if (fctrl->mapmem == NULL)
        {
            LOG(5)("mmap open fail.[%s] %d\n", fctrl->fname, errno);
            return 0;
        }
        int er = sis_disk_io_map_load(fctrl);
        if (er != SIS_MAP_OPEN_OK)
        {
            LOG(5)("map file load fail. [%s]\n", fctrl->fname);
            return 0;
        }
        fctrl->status |= SIS_MAP_STATUS_OPENED;
        return 1;
    }
    return 0;
}
int sis_disk_writer_map_calc_minblks(s_sis_map_fctrl *fctrl)
{
    int kblks = MAP_GET_BLKS(sis_sdslen(fctrl->wkeys), SIS_MAP_MAYUSE_LEN);
    int sblks = MAP_GET_BLKS(sis_sdslen(fctrl->wsdbs), SIS_MAP_MAYUSE_LEN);
    int knums = sis_map_list_getsize(fctrl->map_keys);
    int snums = sis_map_list_getsize(fctrl->map_sdbs);

    int iblks = MAP_GET_BLKS((knums * sizeof(s_sis_map_index)), SIS_MAP_MAYUSE_LEN);
    iblks = iblks * snums;
    
    int vblks = knums * snums;  // 初始化时默认只有一个数据块索引

    // 还要加1个头块 并取 SIS_MAP_INCR_BLKS 模
    return SIS_ZOOM_UP(iblks + vblks + kblks + sblks + 1, SIS_MAP_INCR_BLKS);
} 
void sis_disk_io_map_fw_mhead(s_sis_map_fctrl *fctrl)
{
    s_sis_disk_main_head *mhead = (s_sis_disk_main_head *)fctrl->mapmem;
    memset(mhead, 0, sizeof(s_sis_disk_main_head));
    mhead->fin = 1;
    mhead->zip = 0;
    mhead->hid = SIS_DISK_HID_HEAD;
    memmove(mhead->sign, "SIS", 3);
    mhead->version = SIS_DISK_SDB_VER;
    mhead->style = fctrl->style;
    mhead->wdate = sis_time_get_idate(0);
    // 写 SIS_DISK_HID_MAP_CTRL 
    fctrl->mhead_p = (s_sis_map_head *)(fctrl->mapmem + sizeof(s_sis_disk_main_head));
    fctrl->mhead_p->fin     = 1;
    fctrl->mhead_p->zip     = 0;
    fctrl->mhead_p->hid     = SIS_DISK_HID_MAP_CTRL;
    fctrl->mhead_p->bsize   = SIS_MAP_MIN_SIZE;
    fctrl->mhead_p->fsize   = fctrl->mhead_r.fsize;
    fctrl->mhead_p->useblks = 1;  
    fctrl->mhead_p->maxblks = fctrl->mhead_r.maxblks;  
    fctrl->mhead_p->lastsno = 0; 
    fctrl->mhead_p->lastseq = 0; 
    fctrl->mhead_p->recfbno = -1;
    fctrl->mhead_p->keynums = fctrl->mhead_r.keynums;
    fctrl->mhead_p->sdbnums = fctrl->mhead_r.sdbnums;
    fctrl->mhead_p->keyfbno = -1; // -1 表示从 useblks 获取块信息
    fctrl->mhead_p->sdbfbno = -1;
    for (int i = 0; i < fctrl->mhead_p->sdbnums; i++)
    {
        fctrl->mhead_p->idxfbno[i] = -1;
    }
}
void sis_map_ksctrl_init_mindex(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl)
{
    ksctrl->mindex_p->kidx    = ksctrl->kdict->index;
    ksctrl->mindex_p->sidx    = ksctrl->sdict->index;
    ksctrl->mindex_p->recsize = ksctrl->sdict->table->size;
    ksctrl->mindex_p->makeseq = fctrl->mhead_p->lastseq++; // 
    if (fctrl->style == SIS_DISK_TYPE_MSN)
    {
        ksctrl->mindex_p->perrecs = SIS_MAP_MAYUSE_LEN / (ksctrl->sdict->table->size + 8);
        ksctrl->mindex_p->doffset = sizeof(s_sis_map_block) + ksctrl->mindex_p->perrecs * 8;
    }
    else
    {
        ksctrl->mindex_p->perrecs = SIS_MAP_MAYUSE_LEN / (ksctrl->sdict->table->size);
        ksctrl->mindex_p->doffset = sizeof(s_sis_map_block);
    }
    ksctrl->mindex_p->sumrecs = 0;
    ksctrl->mindex_p->currecs = 0;
    ksctrl->mindex_p->varfbno = -1;
}

void sis_disk_io_map_fw_mindex(s_sis_map_fctrl *fctrl)
{
    // 先映射到文件
    int blks = MAP_GET_BLKS(fctrl->mhead_p->keynums, fctrl->ksirecs);
    for (int si = 0; si < fctrl->mhead_p->sdbnums; si++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, si);
        // 这里初始化索引的地址
        // if (fctrl->mhead_p->idxfbno[si] < 0)
        {
            fctrl->mhead_p->idxfbno[si] = fctrl->mhead_p->useblks;
            s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_p->idxfbno[si]);
            for (int k = 0; k < blks; k++)
            {
                curblk->fin  = 1;
                curblk->zip  = 0;
                curblk->hid  = SIS_DISK_HID_MAP_INDEX;
                sis_int_list_push(sdict->ksiblks, fctrl->mhead_p->useblks);
                fctrl->mhead_p->useblks++;  // 这里需要统一判断
                if (k == blks - 1)
                {
                    curblk->size = (fctrl->mhead_p->keynums % fctrl->ksirecs) * sizeof(s_sis_map_index);
                    curblk->next = -1;
                    break;
                }
                else
                {
                    curblk->size = fctrl->ksirecs * sizeof(s_sis_map_index);
                    curblk->next = fctrl->mhead_p->useblks;
                    curblk = sis_map_block_head(fctrl, curblk->next);
                }
            }
        }
        // 这里对索引数据赋值
        for (int ki = 0; ki < fctrl->mhead_p->keynums; ki++)
        {
            int64 ksidx = sis_disk_io_map_get_ksidx(ki, si);
            s_sis_map_kdict *kdict = sis_map_list_geti(fctrl->map_keys, ki);
            s_sis_map_ksctrl *ksctrl = sis_map_kints_get(fctrl->map_kscs, ksidx);
            if (!ksctrl)
            {
                ksctrl = sis_map_ksctrl_create(kdict, sdict);
                sis_map_kints_set(fctrl->map_kscs, ksidx, ksctrl); 
            }
            // 下面开始读数据
            // 0x71a
            ksctrl->mindex_p = sis_map_fctrl_get_mindex(fctrl, ksctrl->sdict, ksctrl->kdict->index);
            sis_map_ksctrl_init_mindex(fctrl, ksctrl);
            // if (ki >= 4205 || ki == 4204)
            // {
            //     sis_out_binary("windex:", ksctrl->mindex_p, 2 * sizeof(s_sis_map_index));
            // }
        }
    }
    // 索引区的块准备好了
}
void sis_disk_io_map_fw_kdict(s_sis_map_fctrl *fctrl)
{
    size_t curlen = sis_sdslen(fctrl->wkeys);    
    // if (fctrl->mhead_p->keyfbno.fbno < 0)
    {
        fctrl->mhead_p->keyfbno = fctrl->mhead_p->useblks;
        s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_p->keyfbno);
        size_t curpos = 0;
        while(1)
        {
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_DICT_KEY;
            curblk->size = sis_min(SIS_MAP_MAYUSE_LEN, curlen - curpos);
            memmove(curblk->vmem, fctrl->wkeys + curpos, curblk->size);
            fctrl->mhead_p->useblks++;  // 这里需要统一判断
            if (curblk->size < SIS_MAP_MAYUSE_LEN)
            {
                curblk->next = -1;
                break;
            }
            curblk->next = fctrl->mhead_p->useblks;
            curpos += curblk->size;
            curblk = sis_map_block_head(fctrl, curblk->next);
        }
    }
}
void sis_disk_io_map_fw_sdict(s_sis_map_fctrl *fctrl)
{
    size_t curlen = sis_sdslen(fctrl->wsdbs);
    // if (fctrl->mhead_p->sdbfbno < 0)
    {
        fctrl->mhead_p->sdbfbno = fctrl->mhead_p->useblks;
        s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_p->sdbfbno);
        size_t curpos = 0;
        while(1)
        {
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_DICT_SDB;
            curblk->size = sis_min(SIS_MAP_MAYUSE_LEN, curlen - curpos);
            memmove(curblk->vmem, fctrl->wsdbs + curpos, curblk->size);
            fctrl->mhead_p->useblks++;  // 这里需要统一判断
            if (curblk->size < SIS_MAP_MAYUSE_LEN)
            {
                curblk->next = -1;
                break;
            }
            curblk->next = fctrl->mhead_p->useblks;
            curpos += curblk->size;
            curblk = sis_map_block_head(fctrl, curblk->next);
        }
    }
}

int sis_disk_writer_map_inited(s_sis_map_fctrl *fctrl, 
    const char *keys_, size_t klen_, 
    const char *sdbs_, size_t slen_)
{
    int kincr = sis_disk_io_map_set_kdict(fctrl, keys_, klen_);
    int sincr = sis_disk_io_map_set_sdict(fctrl, sdbs_, slen_);
    // printf("---- %s %d %d\n", __func__, kincr, sincr);
    if (fctrl->status & SIS_MAP_STATUS_OPENED)
    {
        if ((kincr + sincr) > 0)
        {
            sis_map_rwlock_w_incr(fctrl->rwlock);
            sis_disk_io_map_ks_change(fctrl, kincr, sincr);
            sis_map_rwlock_w_decr(fctrl->rwlock);
        }
    }
    if (fctrl->status & SIS_MAP_STATUS_NOFILE)
    {
        int knums = sis_map_list_getsize(fctrl->map_keys);
        int snums = sis_map_list_getsize(fctrl->map_sdbs);
        if (knums < 1 || snums < 1)
        {
            return -1;
        }
        fctrl->wkeys = sis_disk_io_map_as_keys(fctrl->map_keys);
        fctrl->wsdbs = sis_disk_io_map_as_sdbs(fctrl->map_sdbs);
        int64 maxblks = sis_disk_writer_map_calc_minblks(fctrl);
        int64 fsize = maxblks * SIS_MAP_MIN_SIZE;

        LOG(5)("create mmap file.[%s]\n", fctrl->fname);
        if (!sis_path_exists(fctrl->fname))
        {
            sis_path_mkdir(fctrl->fname);
        }
        char *mmap = sis_mmap_open_w(fctrl->fname, fsize);
        if (mmap == MAP_FAILED) 
        {
            LOG(5)("mmap open fail.[%s] %d\n", fctrl->fname, errno);
            return SIS_MAP_MMAP_ERR;
        }
        fctrl->mapmem = mmap;
        fctrl->mhead_r.fsize   = fsize;
        fctrl->mhead_r.maxblks = maxblks;
        fctrl->mhead_r.keynums = knums;
        fctrl->mhead_r.sdbnums = snums;

        LOG(5)("create mmap 1 file.[%s] %p\n", fctrl->fname, fctrl->mapmem);

        sis_map_rwlock_w_incr(fctrl->rwlock);
        
        LOG(5)("create mmap 2 file.[%s]\n", fctrl->fname);

        // 先写头
        sis_disk_io_map_fw_mhead(fctrl);
        // SIS_DISK_HID_DICT_KEY
        sis_disk_io_map_fw_kdict(fctrl);
        // SIS_DISK_HID_DICT_SDB
        sis_disk_io_map_fw_sdict(fctrl);
        // SIS_DISK_HID_MAP_INDEX
        sis_disk_io_map_fw_mindex(fctrl);
        // SIS_DISK_HID_MAP_DATA
        // 同步        
        LOG(5)("create mmap 3 file.[%s]\n", fctrl->fname);

        sis_map_rwlock_w_decr(fctrl->rwlock);

        LOG(5)("create mmap file. ok [%s]\n", fctrl->fname);

        sis_mmap_sync(fctrl->mapmem, fctrl->mhead_p->fsize);

        LOG(5)("create mmap file. ok [%s]\n", fctrl->fname);

        fctrl->status &= ~SIS_MAP_STATUS_NOFILE;
        fctrl->status |= SIS_MAP_STATUS_OPENED;
    }
    return 0;
}

int sis_disk_io_map_close(s_sis_map_fctrl *fctrl)
{
    if (fctrl->status & SIS_MAP_STATUS_OPENED)
    {
        if (fctrl->rwmode == 1)
        {
            sis_mmap_sync(fctrl->mapmem, fctrl->mhead_p->fsize);
        }
        sis_unmmap(fctrl->mapmem, fctrl->mhead_r.fsize);
        fctrl->mapmem = NULL;
    }
    fctrl->status = SIS_MAP_STATUS_CLOSED;
    return 0;
}
// 已加锁
void sis_disk_io_map_mmap_change(s_sis_map_fctrl *fctrl)
{
    // 只需修改以下内容就可以重新定位数据了
    // s_sis_map_ksctrl -> mindex_p 其他都不用修改
    // 需要保存 以便unmmap时使用
    fctrl->mhead_r.fsize = fctrl->mhead_p->fsize;

    for (int si = 0; si < fctrl->mhead_p->sdbnums; si++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, si);
        for (int ki = 0; ki < fctrl->mhead_p->keynums; ki++)
        {
            int64 ksidx = sis_disk_io_map_get_ksidx(ki, si);
            s_sis_map_kdict *kdict = sis_map_list_geti(fctrl->map_keys, ki);
            s_sis_map_ksctrl *ksctrl = sis_map_kints_get(fctrl->map_kscs, ksidx);
            if (!ksctrl)
            {
                ksctrl = sis_map_ksctrl_create(kdict, sdict);
                sis_map_kints_set(fctrl->map_kscs, ksidx, ksctrl); 
            }
            // 下面开始读数据
            ksctrl->mindex_p = sis_map_fctrl_get_mindex(fctrl, ksctrl->sdict, ksctrl->kdict->index);          
        }
    }   
}

int sis_disk_io_map_get_newblk(s_sis_map_fctrl *fctrl)
{
    if (fctrl->mhead_p->useblks >= fctrl->mhead_p->maxblks)
    {
        // 对文件扩容 然后重新加载所有和地址相关的信息
        int64 maxblks = (int64)fctrl->mhead_p->maxblks + SIS_MAP_INCR_BLKS; 
        int64 fsize = maxblks * SIS_MAP_MIN_SIZE;
        char *mmap = sis_mmap_open_w(fctrl->fname, fsize);
        if (mmap == MAP_FAILED) 
        {
            LOG(5)("mmap open fail.[%s] %d\n", fctrl->fname, errno);
            return SIS_MAP_MMAP_ERR;
        }
        
        sis_unmmap(fctrl->mapmem, fctrl->mhead_r.fsize);
        fctrl->mapmem = mmap;
        fctrl->mhead_p = (s_sis_map_head *)(fctrl->mapmem + sizeof(s_sis_disk_main_head));
        fctrl->mhead_p->fsize   = fsize;
        fctrl->mhead_p->maxblks = maxblks;
        // 这里只更新和地址有关的信息
        sis_disk_io_map_mmap_change(fctrl);
        LOG(5)("map file changed ok. %d --> %d : %lldM\n", fctrl->mhead_p->useblks, fctrl->mhead_p->maxblks, fctrl->mhead_p->fsize / (1024 * 1024));
    }
    return fctrl->mhead_p->useblks++;
}
// 回收块 curblk 的下一块 当前块还是有数据的
void sis_disk_io_map_recy_blk(s_sis_map_fctrl *fctrl, s_sis_map_block *curblk)
{
    if (curblk->next != -1)
    {  
        // 循环回收
    }
    // 设置当前块为结束块
    curblk->next = -1;
}

void sis_disk_io_map_rw_kdict(s_sis_map_fctrl *fctrl)
{
    size_t curlen = sis_sdslen(fctrl->wkeys);    
    int blks = MAP_GET_BLKS(curlen, SIS_MAP_MAYUSE_LEN);
    
    s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_p->keyfbno);
    size_t curpos = 0;
    for (int i = 0; i < blks; i++)
    {
        if (i < blks - 1)
        {
            curblk->size = SIS_MAP_MAYUSE_LEN;
        }
        else
        {
            curblk->size = curlen - curpos;
        }
        memmove(curblk->vmem, fctrl->wkeys + curpos, curblk->size);
        curpos += curblk->size;
        
        // printf("=====%d %d key %d %d %p %zu, %d\n",i, blks, curlen, curpos, curblk, curblk->size, curblk->next);
        if (curblk->next == -1)
        {
            if (i < blks - 1)
            {
                int newblk = sis_disk_io_map_get_newblk(fctrl);
                curblk->next = newblk;
                curblk = sis_map_block_head(fctrl, curblk->next);
                curblk->fin  = 1;
                curblk->zip  = 0;
                curblk->hid  = SIS_DISK_HID_DICT_KEY;
                curblk->next = -1;
            }
        }
        else
        {
            if (i < blks - 1)
            {
                curblk = sis_map_block_head(fctrl, curblk->next);
            }
            else
            {
                // 需要回收块
                sis_disk_io_map_recy_blk(fctrl, curblk);
            }
        }
    }
}
void sis_disk_io_map_rw_sdict(s_sis_map_fctrl *fctrl)
{
    size_t curlen = sis_sdslen(fctrl->wsdbs);    
    int blks = MAP_GET_BLKS(curlen, SIS_MAP_MAYUSE_LEN);
    
    s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_p->sdbfbno);
    size_t curpos = 0;
    for (int i = 0; i < blks; i++)
    {
        if (i < blks - 1)
        {
            curblk->size = SIS_MAP_MAYUSE_LEN;
        }
        else
        {
            curblk->size = curlen - curpos;
        }
        memmove(curblk->vmem, fctrl->wsdbs + curpos, curblk->size);
        curpos += curblk->size;
        
        // printf("=====%d %d sdb %d %d %p %zu, %d\n",i, blks, curlen, curpos, curblk, curblk->size, curblk->next);
        if (curblk->next == -1)
        {
            if (i < blks - 1)
            {
                int newblk = sis_disk_io_map_get_newblk(fctrl);
                curblk->next = newblk;
                curblk = sis_map_block_head(fctrl, curblk->next);
                curblk->fin  = 1;
                curblk->zip  = 0;
                curblk->hid  = SIS_DISK_HID_DICT_KEY;
                curblk->next = -1;
            }
        }
        else
        {
            if (i < blks - 1)
            {
                curblk = sis_map_block_head(fctrl, curblk->next);
            }
            else
            {
                // 需要回收块
                sis_disk_io_map_recy_blk(fctrl, curblk);
            }
        }
    }
}

// 这里增加一组key 同时增加*老数据表*的mindex信息
void sis_disk_io_map_incr_key(s_sis_map_fctrl *fctrl, int kincr)
{
    int newrecs = fctrl->mhead_p->keynums + kincr;
    for (int si = 0; si < fctrl->mhead_p->sdbnums; si++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, si);
        
        int curblks = MAP_GET_BLKS(newrecs, fctrl->ksirecs);
        
        if (curblks > sdict->ksiblks->count)
        {
            int start = sdict->ksiblks->count;
            
            int lastbno = sis_int_list_get(sdict->ksiblks, sdict->ksiblks->count - 1);
            s_sis_map_block *curblk = sis_map_block_head(fctrl, lastbno);
            
            int newblk = sis_disk_io_map_get_newblk(fctrl);
            curblk->next = newblk;
            curblk->size = fctrl->ksirecs * sizeof(s_sis_map_index);
            curblk = sis_map_block_head(fctrl, curblk->next);
            for (int k = start; k < curblks; k++)
            {
                curblk->fin  = 1;
                curblk->zip  = 0;
                curblk->hid  = SIS_DISK_HID_MAP_INDEX;
                sis_int_list_push(sdict->ksiblks, newblk);
                if (k == curblks - 1)
                {
                    curblk->size = (newrecs % fctrl->ksirecs) * sizeof(s_sis_map_index);
                    curblk->next = -1;
                    break;
                }
                else
                {
                    curblk->size = fctrl->ksirecs * sizeof(s_sis_map_index);
                    newblk = sis_disk_io_map_get_newblk(fctrl);
                    curblk->next = newblk;
                    curblk = sis_map_block_head(fctrl, curblk->next);
                }
            }
        }
        for (int ki = fctrl->mhead_p->keynums; ki < newrecs; ki++)
        {
            int64 ksidx = sis_disk_io_map_get_ksidx(ki, si);
            s_sis_map_kdict *kdict = sis_map_list_geti(fctrl->map_keys, ki);
            s_sis_map_ksctrl *ksctrl = sis_map_kints_get(fctrl->map_kscs, ksidx);
            if (!ksctrl)
            {
                ksctrl = sis_map_ksctrl_create(kdict, sdict);
                sis_map_kints_set(fctrl->map_kscs, ksidx, ksctrl); 
            }
            // 下面开始读数据
            ksctrl->mindex_p = sis_map_fctrl_get_mindex(fctrl, ksctrl->sdict, ksctrl->kdict->index);
            
            sis_map_ksctrl_init_mindex(fctrl, ksctrl);
        }
    }    
}
// 这里增加一组sdb 同时增加*所有key*的mindex信息 此时keynums已更新
void sis_disk_io_map_incr_sdb(s_sis_map_fctrl *fctrl, int sincr)
{
    int newsdbs = fctrl->mhead_p->sdbnums + sincr;
    for (int si = fctrl->mhead_p->sdbnums; si < newsdbs; si++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, si);
        
        int curblks = MAP_GET_BLKS(fctrl->mhead_p->keynums, fctrl->ksirecs);
        
        int newblk = sis_disk_io_map_get_newblk(fctrl);
        fctrl->mhead_p->idxfbno[si] = newblk;
        s_sis_map_block *curblk = sis_map_block_head(fctrl, newblk);
        for (int k = 0; k < curblks; k++)
        {
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_MAP_INDEX;
            sis_int_list_push(sdict->ksiblks, newblk);
            if (k == curblks - 1)
            {
                curblk->size = (fctrl->mhead_p->keynums % fctrl->ksirecs) * sizeof(s_sis_map_index);
                curblk->next = -1;
                break;
            }
            else
            {
                curblk->size = fctrl->ksirecs * sizeof(s_sis_map_index);
                newblk = sis_disk_io_map_get_newblk(fctrl);
                curblk->next = newblk;
                curblk = sis_map_block_head(fctrl, curblk->next);
            }
        }
        
        for (int ki = 0; ki < fctrl->mhead_p->keynums; ki++)
        {
            int64 ksidx = sis_disk_io_map_get_ksidx(ki, si);
            s_sis_map_kdict *kdict = sis_map_list_geti(fctrl->map_keys, ki);
            s_sis_map_ksctrl *ksctrl = sis_map_kints_get(fctrl->map_kscs, ksidx);
            if (!ksctrl)
            {
                ksctrl = sis_map_ksctrl_create(kdict, sdict);
                sis_map_kints_set(fctrl->map_kscs, ksidx, ksctrl); 
            }
            // 下面开始读数据
            ksctrl->mindex_p = sis_map_fctrl_get_mindex(fctrl, ksctrl->sdict, ksctrl->kdict->index);
            
            sis_map_ksctrl_init_mindex(fctrl, ksctrl);
        }
    }  
}
int sis_disk_io_map_ks_change(s_sis_map_fctrl *fctrl, int kincr, int sincr)
{
    printf("ks_change 0 : %d %d %d\n", kincr, sincr, sis_map_kints_getsize(fctrl->map_kscs));
    // 先增加key
    if (kincr > 0)
    {
        // 一次性增加老表的索引数
        sis_disk_io_map_incr_key(fctrl, kincr);
        // 最后再修改数量
        fctrl->mhead_p->keynums = sis_map_list_getsize(fctrl->map_keys);
        sis_sdsfree(fctrl->wkeys);
        fctrl->wkeys = sis_disk_io_map_as_keys(fctrl->map_keys);
        sis_disk_io_map_rw_kdict(fctrl);
    }
    printf("ks_change 1 : %d %d %d\n", fctrl->mhead_p->keynums, fctrl->mhead_p->sdbnums, sis_map_kints_getsize(fctrl->map_kscs));
    // 再增加sdb 顺序不能变
    if (sincr > 0)
    {
        // 一次性增加老表的索引数
        sis_disk_io_map_incr_sdb(fctrl, sincr);
        // 最后再修改数量
        fctrl->mhead_p->sdbnums = sis_map_list_getsize(fctrl->map_sdbs);
        sis_sdsfree(fctrl->wsdbs);
        fctrl->wsdbs = sis_disk_io_map_as_sdbs(fctrl->map_sdbs);
        sis_disk_io_map_rw_sdict(fctrl);
    }
    printf("ks_change 2 : %d %d %d\n", fctrl->mhead_p->keynums, fctrl->mhead_p->sdbnums, sis_map_kints_getsize(fctrl->map_kscs));
    return 0;
}

int sis_disk_io_map_kdict_change(s_sis_map_fctrl *fctrl, s_sis_map_kdict *kdict)
{
    if (fctrl->status & SIS_MAP_STATUS_OPENED)
    {
        // 如果文件已经开始写入 需要增加对应信息
        sis_disk_io_map_incr_key(fctrl, 1);
        // 最后再修改数量
        fctrl->mhead_p->keynums = sis_map_list_getsize(fctrl->map_keys);
        sis_sdsfree(fctrl->wkeys);
        fctrl->wkeys = sis_disk_io_map_as_keys(fctrl->map_keys);
        sis_disk_io_map_rw_kdict(fctrl);
        return 1;
    }
    return 0;
}

// 得到最后一块的数据头
s_sis_map_block *sis_map_ksctrl_incr_bhead(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl)
{
    // if (MAP_GET_BLKS(ksctrl->mindex_p->sumrecs, ksctrl->mindex_r.perrecs) > ksctrl->varblks->count)
    // {
    //     sis_disk_io_map_read_varblks(fctrl, ksctrl);
    // }
    if (ksctrl->varblks->count > 0 && ksctrl->mindex_p->currecs < ksctrl->mindex_p->perrecs)
    {
        int blkno = sis_int_list_get(ksctrl->varblks, ksctrl->varblks->count - 1);
        return sis_map_block_head(fctrl, blkno);
    }
    else
    {
        int newblk = sis_disk_io_map_get_newblk(fctrl);
        s_sis_map_block *curblk = sis_map_block_head(fctrl, newblk);
        curblk->fin  = 1;
        curblk->zip  = 0;
        curblk->hid  = SIS_DISK_HID_MAP_DATA;
        curblk->next = -1;
        curblk->size = 0;
        if (ksctrl->mindex_p->varfbno == -1)
        {
            ksctrl->mindex_p->varfbno = newblk;
        }
        else
        {
            int agobno = sis_int_list_get(ksctrl->varblks, ksctrl->varblks->count - 1);
            s_sis_map_block *agoblk = sis_map_block_head(fctrl, agobno);
            agoblk->next = newblk;
        }
        sis_int_list_push(ksctrl->varblks, newblk);
        return curblk;
    }
}

int sis_mdb_ksctrl_incr_data(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, s_sis_map_block *curblk, char *inmem)
{
    if (curblk)
    {
        int curno = ksctrl->mindex_p->sumrecs % ksctrl->mindex_p->perrecs;
        char *ptr = (char *)curblk + ksctrl->mindex_p->doffset + curno * ksctrl->mindex_p->recsize;
        memmove(ptr, inmem, ksctrl->sdict->table->size);

        ksctrl->mindex_p->currecs = ksctrl->mindex_p->currecs == ksctrl->mindex_p->perrecs ? 1 : ksctrl->mindex_p->currecs + 1;
        ksctrl->mindex_p->sumrecs++;

        curblk->size = ksctrl->mindex_p->currecs * ksctrl->mindex_p->recsize;

        return 1;
    }
    return 0;
}
int sis_msn_ksctrl_incr_data(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, s_sis_map_block *curblk, char *inmem)
{
    if (curblk)
    {
        int curno = ksctrl->mindex_p->sumrecs % ksctrl->mindex_p->perrecs;
        char *ptr = (char *)curblk + ksctrl->mindex_p->doffset + curno * ksctrl->mindex_p->recsize;
        memmove(ptr, inmem, ksctrl->sdict->table->size);
        int64 *var = (int64 *)((char *)curblk + sizeof(s_sis_map_block) + curno * 8);
        *var = fctrl->mhead_p->lastsno;

        fctrl->mhead_p->lastsno++;        
        ksctrl->mindex_p->currecs = ksctrl->mindex_p->currecs == ksctrl->mindex_p->perrecs ? 1 : ksctrl->mindex_p->currecs + 1;
        ksctrl->mindex_p->sumrecs++;

        curblk->size = ksctrl->mindex_p->currecs * (ksctrl->mindex_p->recsize + 8);

        return 1;
    }
    return 0;
}
int sis_disk_io_map_w_data(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, void *in_, size_t ilen_)
{
    if (!(fctrl->status & SIS_MAP_STATUS_OPENED))
    {
        return 0;
    }
    s_sis_map_sdict *sdict = sis_map_list_get(fctrl->map_sdbs, sname_);
    if (!sdict)
    {
        return 0;
    }
    s_sis_map_kdict *kdict = sis_map_list_get(fctrl->map_keys, kname_);
    if (!kdict)
    {
        LOG(8)("new key: %s\n", kname_);
        kdict = sis_disk_io_map_incr_kdict(fctrl, kname_);
        sis_disk_io_map_kdict_change(fctrl, kdict);
        LOG(8)("new key ok: %s\n", kname_);
    }
    // printf("::: ==1== %d %d %d\n", kdict->index, sdict->index, fctrl->map_kscs->list->count);
    int count = ilen_ / sdict->table->size; 
    // start write
    int64 ksidx = sis_disk_io_map_get_ksidx(kdict->index, sdict->index);
    s_sis_map_ksctrl *ksctrl = sis_map_kints_get(fctrl->map_kscs, ksidx);    
    char *ptr = in_;
    // 为即将写入的数据准备块数据

    sis_map_rwlock_w_incr(fctrl->rwlock);
    
    // 这里只保证文件大小足够 数据块并没准备好
    // sis_disk_io_map_incr_ready(fctrl, ksctrl, count);
    // printf("::: ==2== %d %d\n", kdict->index, sdict->index);
    for (int i = 0; i < count; i++)
    {
        s_sis_map_block *curblk = sis_map_ksctrl_incr_bhead(fctrl, ksctrl);
        // 先写数据
        if (fctrl->style == SIS_DISK_TYPE_MSN)
        {
            sis_msn_ksctrl_incr_data(fctrl, ksctrl, curblk, ptr);
        }
        else
        {
            sis_mdb_ksctrl_incr_data(fctrl, ksctrl, curblk, ptr);
        }
        ptr += sdict->table->size;
    }
    sis_map_rwlock_w_decr(fctrl->rwlock); 
    
    if (fctrl->wnums % 100000 == 0)
    {
        msec_t nowmsec = sis_time_get_now_msec();
        if (fctrl->wmsec > 0)
        {
            LOG(8)("wnums = %d cost = %lld\n", fctrl->wnums, nowmsec - fctrl->wmsec);
        }
        fctrl->wnums = 0;
        fctrl->wmsec = nowmsec;
    }
    fctrl->wnums++;

    return ilen_;
}

int sis_disk_io_map_sync_data(s_sis_map_fctrl *fctrl)
{
    // sis_mmap_sync(fctrl->mapmem, fctrl->mhead_p->fsize);
    return 0;
}
int sis_disk_io_map_del_all(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl)
{
    // 删除所有数据
    if (ksctrl->mindex_p->sumrecs > 0 && ksctrl->mindex_p->varfbno >= 0)
    {
        ksctrl->mindex_p->sumrecs =  0;
        ksctrl->mindex_p->currecs =  0;
        s_sis_map_block *curblk = sis_map_block_head(fctrl, ksctrl->mindex_p->varfbno);
        sis_disk_io_map_recy_blk(fctrl, curblk);
        ksctrl->mindex_p->varfbno = -1; 
    }
    return 0;
}
// 得到记录区间 区间的记录数 = 0 表示没有这个区间的数据 
int sis_map_ksctrl_get_recs(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, msec_t head_msec, msec_t tail_msec, int *headr)
{
    int recnums = 0;
    // 循环查找块
    // 如果块的 尾记录 小于 head_msec 直接找下一块
    //    否则一条条判断 直到找到 最后一条小于 head_msec 的记录时 对  headr 赋值
    // 从该块开始 如果 headr 后的记录小于 tail_msec 就累计 recnums + 1
    //    然后下一块  尾记录小于 tail_msec 直接找下一块 recnums + perrecs
    //    否则一条条判断 recnums + 1 直到找到 最后一条小于 tail_msec 的记录时 返回
    return recnums;
    // int agos = 0;
    // printf("%lld %lld %d %d\n", msec->start, msec->stop, sis_msec_get_idate(msec->start), sis_msec_get_idate(msec->stop));
    // if (msec->start == 0 && msec->stop == 0)
    // {
    //     // 等最新的数据 和 get 完全不同
    //     return ksctrl->mindex_r.sumrecs;
    // }
    // else
    // {
    //     if (msec->start == 0 || (!ksctrl->sdict->table->field_time || ksctrl->sdict->table->field_time->len != 8))
    //     {
    //         return 0;
    //     }
    //     else
    //     {
    //         for (int i = 0; i < ksctrl->varblks->count; i++)
    //         {
    //             int blkno = sis_int_list_get(ksctrl->varblks, i);
    //             char *var = (char *)sis_map_ksctrl_get_fbvar(fctrl, ksctrl, blkno);
    //             int recs = 0;
    //             if (i == ksctrl->varblks->count - 1)
    //             {   
    //                 recs = ksctrl->mindex_r.currecs;
    //             }
    //             else
    //             {
    //                 recs = ksctrl->mindex_r.perrecs;
    //             }
    //             // 这是最简单的方法 效率不高 以后有需要再优化
    //             for (int i = 0; i < recs; i++)
    //             {
    //                 msec_t *curmsec = (msec_t *)(var + ksctrl->sdict->table->field_time->offset);
    //                 printf("%lld,%d %d %d \n", *curmsec, sis_msec_get_idate(*curmsec), i, recs);
    //                 if (*curmsec >= msec->start) 
    //                 {
    //                     return i + agos;
    //                 }
    //                 var += ksctrl->mindex_r.recsize;
    //             }
    //             agos += recs;
    //         }
    //     }
    // }
    // return agos;
}
// 删除一个数据
int sis_disk_io_map_del_one(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, msec_t head_msec, msec_t tail_msec)
{
    int head_recno = 0;
    int recnums = sis_map_ksctrl_get_recs(fctrl, ksctrl, head_msec, tail_msec, &head_recno);
    if (recnums < 1)
    {
        return 0;
    }
    // 这里先定位数据块

    // 把剩余块数据拷贝过来

    // 以此类推 

    // 回收可能多出的块

    // int head_recno = sis_map_ksctrl_get_head_cursor(fctrl, ksctrl, head_msec);
    // int tail_recno = sis_map_ksctrl_get_head_cursor(fctrl, ksctrl, head_msec);
    // sis_map_ksctrl_get_timefd
    // for (int bi = 0; bi < ksctrl->varblks->count; bi++)
    // {
    //     int blkno = sis_int_list_get(ksctrl->varblks, bi);
    //     s_sis_map_block *curblk = sis_map_block_head(fctrl, blkno);

    // }
        
    //     if (ksctrl->varblks->count > 0 && ksctrl->mindex_p->currecs < ksctrl->mindex_p->perrecs)
    //     {
    //         int blkno = sis_int_list_get(ksctrl->varblks, ksctrl->varblks->count - 1);
    //         return sis_map_block_head(fctrl, blkno);
    //     }
    // s_sis_map_block *curblk = sis_map_ksctrl_incr_bhead(fctrl, ksctrl);
    // // 先写数据
    // if (fctrl->style == SIS_DISK_TYPE_MSN)
    // {
    //     sis_msn_ksctrl_incr_data(fctrl, ksctrl, curblk, ptr);
    // }
    // else
    // {
    //     sis_mdb_ksctrl_incr_data(fctrl, ksctrl, curblk, ptr);
    // }
    return recnums;
}
int sis_disk_io_map_del_data(s_sis_map_fctrl *fctrl, const char *keys_, const char *sdbs_, int idate_)
{
    if (!(fctrl->status & SIS_MAP_STATUS_OPENED))
    {
        return 0;
    }
    if (idate_ == 0)
    {
        return 0;
    }
    s_sis_string_list *klist = sis_string_list_create();
    s_sis_string_list *slist = sis_string_list_create();
    int knums = 0;
    if (!sis_strcasecmp(keys_, "*"))
    {
        knums = sis_map_list_getsize(fctrl->map_keys);
    }
    else
    {
        knums = sis_string_list_load(klist, keys_, sis_strlen(keys_), ",");
    }
    int snums = 0;
    if (!sis_strcasecmp(sdbs_, "*"))
    {
        snums = sis_map_list_getsize(fctrl->map_sdbs);
    }
    else
    {
        snums = sis_string_list_load(slist, sdbs_, sis_strlen(sdbs_), ",");
    }
    msec_t head_msec = sis_time_make_msec(idate_, 0, 0);
    msec_t tail_msec = sis_time_make_msec(idate_, 235959, 999);

    sis_map_rwlock_w_incr(fctrl->rwlock);
    int nums = 0;
    for (int si = 0; si < snums; si++)
    {
        s_sis_map_sdict *sdict = NULL;
        if (!sis_strcasecmp(sdbs_, "*"))
        {
            sdict = sis_map_list_geti(fctrl->map_sdbs, si);
        }
        else
        {
            const char *sdb = sis_string_list_get(slist, si);
            sdict = sis_map_list_get(fctrl->map_sdbs, sdb);
        }
        if (!sdict)
        {
            continue;
        }
        for (int ki = 0; ki < knums; ki++)
        {
            s_sis_map_kdict *kdict = NULL;
            if (!sis_strcasecmp(keys_, "*"))
            {
                kdict = sis_map_list_geti(fctrl->map_keys, ki);
            }
            else
            {
                const char *key = sis_string_list_get(slist, ki);
                kdict = sis_map_list_get(fctrl->map_keys, key);
            }
            if (!kdict)
            {
                continue;
            }
            int64 ksidx = sis_disk_io_map_get_ksidx(kdict->index, sdict->index);
            s_sis_map_ksctrl *ksctrl = sis_map_kints_get(fctrl->map_kscs, ksidx); 
            if (ksctrl)
            {
                if (idate_ == -1)
                {
                    sis_disk_io_map_del_all(fctrl, ksctrl);
                }
                else
                {
                    if (sdict->table->field_time && sdict->table->field_time->len == 8)
                    {
                        sis_disk_io_map_del_one(fctrl, ksctrl, head_msec, tail_msec);
                    }
                    else
                    {
                        // 非时序数据直接删除
                        sis_disk_io_map_del_all(fctrl, ksctrl);
                    }
                }
                nums++;
            }
            else
            {
                LOG(5)("no ks: %s %s\n", kdict->kname, sdict->table->name);
            }
        }
        
    }
    sis_map_rwlock_w_decr(fctrl->rwlock); 
    sis_string_list_destroy(klist);
    sis_string_list_destroy(slist);
    return nums;
}

int sis_disk_io_map_del_keys(s_sis_map_fctrl *fctrl, const char *keys_)
{

    return 0;
}

int sis_disk_io_map_del_sdbs(s_sis_map_fctrl *fctrl, const char *sdbs_)
{

    return 0;
}


