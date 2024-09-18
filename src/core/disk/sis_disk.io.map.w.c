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
    int kblks = sis_sdslen(fctrl->wkeys) / SIS_MAP_MAYUSE_LEN + 1;
    int sblks = sis_sdslen(fctrl->wsdbs) / SIS_MAP_MAYUSE_LEN + 1;
    int knums = sis_map_list_getsize(fctrl->map_keys);
    int snums = sis_map_list_getsize(fctrl->map_sdbs);

    int iblks = (knums * sizeof(s_sis_map_index)) / SIS_MAP_MAYUSE_LEN + 1;
    iblks = iblks * snums;
    
    int vblks = knums * snums;  // 初始化时默认只有一个数据块索引

    // 还要加1个头块 并取 SIS_MAP_INCR_SIZE 模
    return SIS_ZOOM_UP(iblks + vblks + kblks + sblks + 1, SIS_MAP_INCR_SIZE);
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
    fctrl->mhead_p->recbfno = -1;
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
    int blks = fctrl->mhead_p->keynums / fctrl->ksirecs + 1;
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
            ksctrl->mindex_p = sis_map_fctrl_get_mindex(fctrl, ksctrl->sdict, ksctrl->kdict->index);
            sis_map_ksctrl_init_mindex(fctrl, ksctrl);
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

        LOG(5)("create mmap 1 file.[%s]\n", fctrl->fname);

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
        int maxblks = fctrl->mhead_p->maxblks + SIS_MAP_INCR_SIZE; 
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
    }
    return fctrl->mhead_p->useblks++;
}

void sis_disk_io_map_rw_kdict(s_sis_map_fctrl *fctrl)
{
    size_t curlen = sis_sdslen(fctrl->wkeys);    

    s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_p->keyfbno);
    size_t curpos = 0;
    while(1)
    {
        curblk->size = sis_min(SIS_MAP_MAYUSE_LEN, curlen - curpos);
        memmove(curblk->vmem, fctrl->wkeys + curpos, curblk->size);
        if (curblk->size < SIS_MAP_MAYUSE_LEN)
        {
            break;
        }
        if (curblk->next == -1)
        {
            int newblk = sis_disk_io_map_get_newblk(fctrl);
            curblk->next = newblk;
            curblk = sis_map_block_head(fctrl, curblk->next);
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_DICT_KEY;
            curblk->next = -1;
        }
        else
        {
            curblk = sis_map_block_head(fctrl, curblk->next);
        }
        curpos += curblk->size;
    }
}
void sis_disk_io_map_rw_sdict(s_sis_map_fctrl *fctrl)
{
    size_t curlen = sis_sdslen(fctrl->wsdbs);
    
    s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_p->sdbfbno);
    size_t curpos = 0;
    while(1)
    {
        curblk->size = sis_min(SIS_MAP_MAYUSE_LEN, curlen - curpos);
        memmove(curblk->vmem, fctrl->wsdbs + curpos, curblk->size);
        if (curblk->size < SIS_MAP_MAYUSE_LEN)
        {
            break;
        }
        if (curblk->next == -1)
        {
            int newblk = sis_disk_io_map_get_newblk(fctrl);
            curblk->next = newblk;
            curblk = sis_map_block_head(fctrl, curblk->next);
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_DICT_SDB;
            curblk->next = -1;
        }
        else
        {
            curblk = sis_map_block_head(fctrl, curblk->next);
        }
        curpos += curblk->size;
    }
}

// 
void sis_disk_io_map_incr_key(s_sis_map_fctrl *fctrl, int kincr)
{
    for (int si = 0; si < fctrl->mhead_p->sdbnums; si++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, si);
        
        int newrecs = fctrl->mhead_p->keynums + kincr;
        int curblks = newrecs / fctrl->ksirecs + 1;
        
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
void sis_disk_io_map_incr_sdb(s_sis_map_fctrl *fctrl, int sincr)
{
    for (int si = fctrl->mhead_p->sdbnums; si < sincr; si++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, si);
        
        int curblks = fctrl->mhead_p->keynums / fctrl->ksirecs + 1;
        
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
    // 先增加key
    if (kincr > 0)
    {
        // 一次性增加老表的索引数
        sis_disk_io_map_incr_key(fctrl, kincr);
        // 最后再修改数量
        fctrl->mhead_p->keynums = sis_map_list_getsize(fctrl->map_keys);
        sis_sdsfree(fctrl->wsdbs);
        fctrl->wkeys = sis_disk_io_map_as_keys(fctrl->map_keys);
        sis_disk_io_map_rw_kdict(fctrl);
    }
    // 再增加sdb
    if (sincr > 0)
    {
        // 一次性增加老表的索引数
        sis_disk_io_map_incr_sdb(fctrl, sincr);
        // 最后再修改数量
        fctrl->mhead_p->sdbnums = sis_map_list_getsize(fctrl->map_sdbs);
        sis_sdsfree(fctrl->wsdbs);
        fctrl->wsdbs = sis_disk_io_map_as_keys(fctrl->map_sdbs);
        sis_disk_io_map_rw_sdict(fctrl);
    }
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
        sis_sdsfree(fctrl->wsdbs);
        fctrl->wkeys = sis_disk_io_map_as_keys(fctrl->map_keys);
        sis_disk_io_map_rw_kdict(fctrl);
        return 1;
    }
    return 0;
}

// 得到最后一块的数据头
s_sis_map_block *sis_map_ksctrl_incr_bhead(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl)
{
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

int sis_map_ksctrl_incr_data(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, s_sis_map_block *curblk, char *inmem)
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
    }
    printf("::: ==1== %d %d %d\n", kdict->index, sdict->index, fctrl->map_kscs->list->count);
    int count = ilen_ / sdict->table->size; 
    // start write
    int64 ksidx = sis_disk_io_map_get_ksidx(kdict->index, sdict->index);
    s_sis_map_ksctrl *ksctrl = sis_map_kints_get(fctrl->map_kscs, ksidx);    
    char *ptr = in_;
    // 为即将写入的数据准备块数据

    sis_map_rwlock_w_incr(fctrl->rwlock);
    
    // 这里只保证文件大小足够 数据块并没准备好
    // sis_disk_io_map_incr_ready(fctrl, ksctrl, count);
    printf("::: ==2== %d %d\n", kdict->index, sdict->index);
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
            sis_map_ksctrl_incr_data(fctrl, ksctrl, curblk, ptr);
        }
        ptr += sdict->table->size;
    }
    sis_map_rwlock_w_decr(fctrl->rwlock); 

    return ilen_;
}
