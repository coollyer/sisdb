#include "sis_disk.io.map.h"

////////////////////
// write 
////////////////////

void sis_disk_io_map_ksctrl_incr_key(s_sis_map_fctrl *fctrl, s_sis_map_kdict *kdict)
{
    // 增加索引
    for (int i = 0; i < fctrl->maphead->sdbnums; i++)
    {
        s_sis_map_bctrl *bctrl = sis_pointer_list_get(fctrl->mapbctrl_idx, i);
        bctrl->sumrecs = sis_map_list_getsize(fctrl->map_keys);
        bctrl->currecs = bctrl->sumrecs % bctrl->perrecs;
        int newblks = bctrl->sumrecs / bctrl->perrecs + 1;
        s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->ibinfo[i].fbno + fctrl->maphead->ibinfo[i].blks - 1);
        if (newblks > fctrl->maphead->ibinfo[i].blks)
        {
            curblk->next = fctrl->maphead->useblks;
            fctrl->maphead->useblks++; // 这里需要统一判断
            curblk = sis_map_block_head(fctrl, curblk->next);
            sis_node_push(bctrl->nodes, curblk);
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_MAP_INDEX;
            curblk->next = -1;
            curblk->size = bctrl->needsno ? bctrl->currecs * (bctrl->recsize + 8) : bctrl->currecs * bctrl->recsize;
            fctrl->maphead->ibinfo[i].blks = newblks;
        }
        else
        {
            curblk->size = bctrl->needsno ? bctrl->currecs * (bctrl->recsize + 8) : bctrl->currecs * bctrl->recsize;
        }
    }
    // 增加数据块
    for (int i = 0; i < fctrl->maphead->sdbnums; i++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, i);
        s_sis_map_bctrl *bctrl = sis_pointer_list_get(fctrl->mapbctrl_idx, i);
        {
            s_sis_map_ksctrl *ksctrl = sis_map_ksctrl_create(kdict, sdict);
            sis_map_kint_set(fctrl->map_kscs, ksctrl->ksidx, ksctrl);                
            // 这里初始化索引数据
            s_sis_map_index *mindex = sis_map_bctrl_get_var(bctrl, kdict->index);
            mindex->kidx        = kdict->index;
            mindex->sidx        = sdict->index;
            mindex->perrecs     = ksctrl->mapbctrl_var->perrecs;
            mindex->sumrecs     = 0;
            mindex->currecs     = 0;
            mindex->vbinfo.blks = 1;
            mindex->vbinfo.fbno = fctrl->maphead->useblks;
            // 这里初始化数据节点
            s_sis_map_block *curblk = sis_map_block_head(fctrl, mindex->vbinfo.fbno);
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_MAP_DATA;
            curblk->size = 0;
            curblk->next = -1;
            sis_node_push(ksctrl->mapbctrl_var->nodes, curblk);
            fctrl->maphead->useblks++;  // 这里需要统一判断
        }
    }
    fctrl->maphead->keynums = sis_map_list_getsize(fctrl->map_keys);
}
void sis_disk_io_map_ksctrl_incr_sdb(s_sis_map_fctrl *fctrl, s_sis_map_sdict *sdict)
{
    // 先映射到文件
    s_sis_map_bctrl *bctrl = sis_map_bctrl_create(0, sizeof(s_sis_map_index));
    sis_pointer_list_push(fctrl->mapbctrl_idx, bctrl);
    bctrl->sumrecs = fctrl->maphead->keynums;
    bctrl->currecs = bctrl->sumrecs % bctrl->perrecs;
    sis_node_clear(bctrl->nodes);
    int i = sdict->index;
    fctrl->maphead->ibinfo[i].fbno = fctrl->maphead->useblks;
    fctrl->maphead->ibinfo[i].blks = bctrl->sumrecs / bctrl->perrecs + 1;
    s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->ibinfo[i].fbno);
    for (int k = 0; k < fctrl->maphead->ibinfo[i].blks; k++)
    {
        curblk->fin  = 1;
        curblk->zip  = 0;
        curblk->hid  = SIS_DISK_HID_MAP_INDEX;
        sis_node_push(bctrl->nodes, curblk);
        fctrl->maphead->useblks++;  // 这里需要统一判断
        if (k == fctrl->maphead->ibinfo[i].blks - 1)
        {
            curblk->size = bctrl->needsno ? bctrl->currecs * (bctrl->recsize + 8) : bctrl->currecs * bctrl->recsize;
            curblk->next = -1;
            break;
        }
        else
        {
            curblk->size = bctrl->needsno ? bctrl->perrecs * (bctrl->recsize + 8) : bctrl->perrecs * bctrl->recsize;
            curblk->next = fctrl->maphead->useblks;
            curblk = sis_map_block_head(fctrl, curblk->next);
        }
    }
    // init_ksctrl
    for (int k = 0; k < fctrl->maphead->keynums; k++)
    {
        s_sis_map_kdict *kdict = sis_map_list_geti(fctrl->map_keys, k);
        s_sis_map_ksctrl *ksctrl = sis_map_ksctrl_create(kdict, sdict);
        sis_map_kint_set(fctrl->map_kscs, ksctrl->ksidx, ksctrl);                
        // 这里初始化索引数据
        s_sis_map_index *mindex = sis_map_bctrl_get_var(bctrl, k);
        mindex->kidx        = kdict->index;
        mindex->sidx        = sdict->index;
        mindex->perrecs     = ksctrl->mapbctrl_var->perrecs;
        mindex->sumrecs     = 0;
        mindex->currecs     = 0;
        mindex->vbinfo.blks = 0;
        mindex->vbinfo.fbno = fctrl->maphead->useblks;
        // 这里初始化数据节点
        s_sis_map_block *curblk = sis_map_block_head(fctrl, mindex->vbinfo.fbno);
        curblk->fin  = 1;
        curblk->zip  = 0;
        curblk->hid  = SIS_DISK_HID_MAP_DATA;
        sis_node_push(ksctrl->mapbctrl_var->nodes, curblk);
        mindex->vbinfo.blks = 1;
        fctrl->maphead->useblks++;  // 这里需要统一判断
    }
    fctrl->maphead->sdbnums = sis_map_list_getsize(fctrl->map_sdbs);
}
void sis_disk_io_map_write_ksctrl(s_sis_map_fctrl *fctrl)
{
    // 设置序列号为0
    // fctrl->maphead->lastsno = 0;
    sis_map_kint_clear(fctrl->map_kscs);
    
    for (int i = 0; i < fctrl->maphead->sdbnums; i++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, i);
        s_sis_map_bctrl *bctrl = sis_pointer_list_get(fctrl->mapbctrl_idx, i);
        for (int k = 0; k < fctrl->maphead->keynums; k++)
        {
            s_sis_map_kdict *kdict = sis_map_list_geti(fctrl->map_keys, k);
            s_sis_map_ksctrl *ksctrl = sis_map_ksctrl_create(kdict, sdict);
            sis_map_kint_set(fctrl->map_kscs, ksctrl->ksidx, ksctrl);                
            // 这里初始化索引数据
            s_sis_map_index *mindex = sis_map_bctrl_get_var(bctrl, k);
            mindex->kidx        = kdict->index;
            mindex->sidx        = sdict->index;
            mindex->perrecs     = ksctrl->mapbctrl_var->perrecs;
            mindex->sumrecs     = 0;
            mindex->currecs     = 0;
            mindex->vbinfo.blks = 0;
            mindex->vbinfo.fbno = fctrl->maphead->useblks;
            // 这里初始化数据节点
            s_sis_map_block *curblk = sis_map_block_head(fctrl, mindex->vbinfo.fbno);
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_MAP_DATA;
            curblk->next = -1;
            curblk->size = 0;
            sis_node_push(ksctrl->mapbctrl_var->nodes, curblk);
            mindex->vbinfo.blks = 1;
            fctrl->maphead->useblks++;  // 这里需要统一判断
        }
    }
}

void sis_disk_io_map_write_mindex(s_sis_map_fctrl *fctrl)
{
    // 先映射到文件
    sis_pointer_list_clear(fctrl->mapbctrl_idx);
    for (int i = 0; i < fctrl->maphead->sdbnums; i++)
    {
        s_sis_map_bctrl *bctrl = sis_map_bctrl_create(0, sizeof(s_sis_map_index));
        sis_pointer_list_push(fctrl->mapbctrl_idx, bctrl);
        bctrl->sumrecs = fctrl->maphead->keynums;
        bctrl->currecs = bctrl->sumrecs % bctrl->perrecs;
        fctrl->maphead->ibinfo[i].blks = bctrl->sumrecs / bctrl->perrecs + 1;
        sis_node_clear(bctrl->nodes);
        if (fctrl->maphead->ibinfo[i].fbno < 0)
        {
            fctrl->maphead->ibinfo[i].fbno = fctrl->maphead->useblks;
            s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->ibinfo[i].fbno);
            for (int k = 0; k < fctrl->maphead->ibinfo[i].blks; k++)
            {
                curblk->fin  = 1;
                curblk->zip  = 0;
                curblk->hid  = SIS_DISK_HID_MAP_INDEX;
                sis_node_push(bctrl->nodes, curblk);
                fctrl->maphead->useblks++;  // 这里需要统一判断
                if (k == fctrl->maphead->ibinfo[i].blks - 1)
                {
                    curblk->size = bctrl->needsno ? bctrl->currecs * (bctrl->recsize + 8) : bctrl->currecs * bctrl->recsize;
                    curblk->next = -1;
                    break;
                }
                else
                {
                    curblk->size = bctrl->needsno ? bctrl->perrecs * (bctrl->recsize + 8) : bctrl->perrecs * bctrl->recsize;
                    curblk->next = fctrl->maphead->useblks;
                    curblk = sis_map_block_head(fctrl, curblk->next);
                }
            }
        }
        else
        {
            s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->ibinfo[i].fbno);
            for (int k = 0; k < fctrl->maphead->ibinfo[i].blks; k++)
            {
                sis_node_push(bctrl->nodes, curblk);
                if (k == fctrl->maphead->ibinfo[i].blks - 1)
                {
                    curblk->size = bctrl->needsno ? bctrl->currecs * (bctrl->recsize + 8) : bctrl->currecs * bctrl->recsize;
                    curblk->next = -1;
                    break;
                }
                if (curblk->next == -1)
                {
                    fctrl->maphead->useblks++;  // 这里需要统一判断
                    curblk->size = bctrl->needsno ? bctrl->perrecs * (bctrl->recsize + 8) : bctrl->perrecs * bctrl->recsize;
                    curblk->next = fctrl->maphead->useblks;
                    curblk = sis_map_block_head(fctrl, curblk->next);
                    curblk->fin  = 1;
                    curblk->zip  = 0;
                    curblk->hid  = SIS_DISK_HID_MAP_INDEX;
                    curblk->next = -1;
                }
                else
                {
                    curblk = sis_map_block_head(fctrl, curblk->next);
                }
            }      
        }
    }
}
void sis_disk_io_map_write_kdict(s_sis_map_fctrl *fctrl)
{
    size_t curlen = sis_sdslen(fctrl->wkeys);    
    if (fctrl->maphead->kbinfo.fbno < 0)
    {
        fctrl->maphead->kbinfo.fbno = fctrl->maphead->useblks;
        fctrl->maphead->kbinfo.blks = 0;
        char *curmem = sis_map_block_memory(fctrl, fctrl->maphead->kbinfo.fbno);
        s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->kbinfo.fbno);
        size_t curpos = 0;
        while(1)
        {
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_DICT_KEY;
            curblk->size = sis_min(SIS_MAP_MAYUSE_LEN, curlen - curpos);
            memmove(curmem, fctrl->wkeys + curpos, curblk->size);
            fctrl->maphead->useblks++;  // 这里需要统一判断
            fctrl->maphead->kbinfo.blks++;
            if (curblk->size < SIS_MAP_MAYUSE_LEN)
            {
                curblk->next = -1;
                break;
            }
            curblk->next = fctrl->maphead->useblks;
            curpos += curblk->size;
            curmem = sis_map_block_memory(fctrl, curblk->next);
            curblk = sis_map_block_head(fctrl, curblk->next);
        }
    }
    else
    {
        char *curmem = sis_map_block_memory(fctrl, fctrl->maphead->kbinfo.fbno);
        s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->kbinfo.fbno);
        size_t curpos = 0;
        fctrl->maphead->kbinfo.blks = 0;
        while(1)
        {
            curblk->size = sis_min(SIS_MAP_MAYUSE_LEN, curlen - curpos);
            memmove(curmem, fctrl->wkeys + curpos, curblk->size);
            fctrl->maphead->kbinfo.blks++;
            if (curblk->size < SIS_MAP_MAYUSE_LEN)
            {
                break;
            }
            curmem = sis_map_block_memory(fctrl, curblk->next);
            if (curblk->next == -1)
            {
                fctrl->maphead->useblks++;  // 这里需要统一判断
                curblk->next = fctrl->maphead->useblks;
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
}
void sis_disk_io_map_write_sdict(s_sis_map_fctrl *fctrl)
{
    size_t curlen = sis_sdslen(fctrl->wsdbs);
    if (fctrl->maphead->sbinfo.fbno < 0)
    {
        fctrl->maphead->sbinfo.fbno = fctrl->maphead->useblks;
        char *curmem = sis_map_block_memory(fctrl, fctrl->maphead->sbinfo.fbno);
        s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->sbinfo.fbno);
        size_t curpos = 0;
        while(1)
        {
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_DICT_SDB;
            curblk->size = sis_min(SIS_MAP_MAYUSE_LEN, curlen - curpos);
            memmove(curmem, fctrl->wsdbs + curpos, curblk->size);
            fctrl->maphead->useblks++;  // 这里需要统一判断
            fctrl->maphead->sbinfo.blks++;
            if (curblk->size < SIS_MAP_MAYUSE_LEN)
            {
                curblk->next = -1;
                break;
            }
            curblk->next = fctrl->maphead->useblks;
            curpos += curblk->size;
            curmem = sis_map_block_memory(fctrl, curblk->next);
            curblk = sis_map_block_head(fctrl, curblk->next);
        }
    }
    else
    {
        char *curmem = sis_map_block_memory(fctrl, fctrl->maphead->sbinfo.fbno);
        s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->sbinfo.fbno);
        size_t curpos = 0;
        fctrl->maphead->sbinfo.blks = 0;
        while(1)
        {
            curblk->size = sis_min(SIS_MAP_MAYUSE_LEN, curlen - curpos);
            memmove(curmem, fctrl->wsdbs + curpos, curblk->size);
            fctrl->maphead->sbinfo.blks++;
            if (curblk->size < SIS_MAP_MAYUSE_LEN)
            {
                break;
            }
            curmem = sis_map_block_memory(fctrl, curblk->next);
            if (curblk->next == -1)
            {
                fctrl->maphead->useblks++;  // 这里需要统一判断
                curblk->next = fctrl->maphead->useblks;
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
}

int sis_disk_io_map_w_open(s_sis_map_fctrl *fctrl, const char *fpath_, const char *fname_)
{
    sis_sdsfree(fctrl->fname);
    fctrl->fname = sis_disk_io_map_get_fname(fpath_, fname_);

    fctrl->rwmode = 1;

    if (!sis_file_exists(fctrl->fname))
    {
        fctrl->status |= SIS_DISK_STATUS_CREATE;  // 新文件
    }
    else
    {
        if (sis_disk_io_map_open(fctrl) != SIS_DISK_CMD_OK)
        {
            LOG(5)("map file is no valid.[%s]\n", fctrl->fname);
            return 0;
        }
        if (sis_disk_io_map_load(fctrl) != SIS_MAP_LOAD_OK)
        {
            LOG(5)("map file is no load.[%s]\n", fctrl->fname);
            return 0;
        }
        
        fctrl->status |= SIS_DISK_STATUS_OPENED;
    }
    return 1;
}
int sis_disk_io_map_inited(s_sis_map_fctrl *fctrl)
{
    int keynums = sis_map_list_getsize(fctrl->map_keys);
    int sdbnums = sis_map_list_getsize(fctrl->map_sdbs);
    // 增加锁信息
    sis_map_rwlocks_incr(fctrl->rwlocks, keynums * sdbnums);
    // 计算文件大小
    int idxs = (keynums * sizeof(s_sis_map_index)) / SIS_MAP_MAYUSE_LEN + 1;
    idxs = idxs * sdbnums;
    int blks = keynums * sdbnums; // 初始化时默认只有一个块索引 后期

    fctrl->wkeys = sis_disk_io_map_as_keys(fctrl->map_keys);
    int keys = sis_sdslen(fctrl->wkeys) / SIS_MAP_MAYUSE_LEN + 1;
    fctrl->wsdbs = sis_disk_io_map_as_sdbs(fctrl->map_sdbs);
    int sdbs = sis_sdslen(fctrl->wkeys) / SIS_MAP_MAYUSE_LEN + 1;

    int64 fsize = sizeof(s_sis_disk_main_head) + sizeof(s_sis_map_ctrl);
    int64 maxblks = SIS_ZOOM_UP(idxs + blks + keys + sdbs, SIS_MAP_INCR_SIZE);
    fsize += maxblks * SIS_DISK_MAXLEN_MAPPAGE;
    // 创建文件
    {
        int fd;
        fd = sis_open(fctrl->fname, SIS_FILE_IO_RDWR | SIS_FILE_IO_CREATE, 0666);
        if (fd == -1)
        {
            LOG(5)("error opening file. %s\n", fctrl->fname);
            return SIS_DISK_CMD_NO_OPEN;
        }
        if (sis_seek(fd, fsize - 1, SEEK_SET) == -1)
        {
            LOG(5) ("error stretching the file.\n");
            sis_close(fd);
            return SIS_DISK_CMD_SIZENO;
        }
        // 在文件末尾写入一个空字节，扩大文件
        if (sis_write(fd, "", 1) != 1)
        {
            LOG(5)("error writing last byte of the file.\n");
            sis_close(fd);
            return SIS_DISK_CMD_RWFAIL;
        }
        char *map = // sis_mmap_w(fd, fsize);
        mmap(0, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) 
        {
            LOG(5)("error mapping the file. %d %d %d\n", fsize, fd, errno);
            sis_close(fd);
            return SIS_DISK_CMD_MAPERR;
        }
        fctrl->mapmem = map;
        sis_close(fd);
    }
    // 下面开始写文件
    s_sis_disk_main_head *mhead = (s_sis_disk_main_head *)fctrl->mapmem;
    memset(mhead, 0, sizeof(s_sis_disk_main_head));
    mhead->fin = 1;
    mhead->zip = 0;
    mhead->hid = SIS_DISK_HID_HEAD;
    memmove(mhead->sign, "SIS", 3);
    mhead->version = SIS_DISK_SDB_VER;
    mhead->style = SIS_DISK_TYPE_MAP;
    mhead->wdate = sis_time_get_idate(0);
    // 写 SIS_DISK_HID_MAP_CTRL 
    fctrl->maphead = (s_sis_map_ctrl *)(fctrl->mapmem + sizeof(s_sis_disk_main_head));
    fctrl->maphead->fin     = 1;
    fctrl->maphead->zip     = 0;
    fctrl->maphead->hid     = SIS_DISK_HID_MAP_CTRL;
    fctrl->maphead->bsize   = SIS_DISK_MAXLEN_MAPPAGE;
    fctrl->maphead->fsize   = fsize;
    fctrl->maphead->useblks = 0;  
    fctrl->maphead->maxblks = maxblks;  
    fctrl->maphead->lastsno = 0; 
    fctrl->maphead->boffset = sizeof(s_sis_disk_main_head) + sizeof(s_sis_map_ctrl);
    fctrl->maphead->keynums = keynums;
    fctrl->maphead->sdbnums = sdbnums;
    fctrl->maphead->kbinfo.fbno = -1; // -1 表示从 useblks 获取块信息
    fctrl->maphead->sbinfo.fbno = -1;
    fctrl->maphead->kbinfo.blks = 0; 
    fctrl->maphead->sbinfo.blks = 0; 
    for (int i = 0; i < fctrl->maphead->sdbnums; i++)
    {
        fctrl->maphead->ibinfo[i].fbno = -1;
        fctrl->maphead->ibinfo[i].blks = 0;         
    }
    // SIS_DISK_HID_DICT_KEY
    sis_disk_io_map_write_kdict(fctrl);
    // SIS_DISK_HID_DICT_SDB
    sis_disk_io_map_write_sdict(fctrl);
    // SIS_DISK_HID_MAP_INDEX
    sis_disk_io_map_write_mindex(fctrl);
    // SIS_DISK_HID_MAP_DATA
    sis_disk_io_map_write_ksctrl(fctrl);
    // 建立数据索引
    // 同步
    sis_mmap_sync(fctrl->mapmem, fctrl->maphead->fsize);
    return 0;
}
// 开始 此时 key 和 sdb 已经有基础数据了
int sis_disk_io_map_w_start(s_sis_map_fctrl *fctrl)
{
    printf("inited... %d %d %zu %zu\n", fctrl->status, fctrl->rwmode, sis_map_list_getsize(fctrl->map_keys), sis_map_list_getsize(fctrl->map_sdbs));
    if (fctrl->rwmode != 1 || sis_map_list_getsize(fctrl->map_keys) < 1 || sis_map_list_getsize(fctrl->map_sdbs) < 1)
    {
        return -1;
    }
    printf("inited... %d\n", fctrl->status);
    if (fctrl->status & SIS_DISK_STATUS_CREATE)
    {
        // 
        sis_disk_io_map_inited(fctrl);
        printf("inited...\n");
        //
        fctrl->status &= ~SIS_DISK_STATUS_CREATE;
        fctrl->status |= SIS_DISK_STATUS_OPENED;
    }
    return 0;
}

int sis_disk_io_map_w_stop(s_sis_map_fctrl *fctrl)
{
    if (fctrl->status == SIS_DISK_STATUS_CLOSED)
    {
        return 0;
    }
    sis_mmap_sync(fctrl->mapmem, fctrl->maphead->fsize);
    return 0;
}

int sis_disk_io_map_close(s_sis_map_fctrl *fctrl)
{
    if (fctrl->status & SIS_DISK_STATUS_OPENED)
    {
        if (fctrl->rwmode == 1)
        {
            sis_mmap_sync(fctrl->mapmem, fctrl->maphead->fsize);
        }
        sis_unmmap(fctrl->mapmem, fctrl->maphead->fsize);
        fctrl->mapmem = NULL;
    }
    fctrl->status = SIS_DISK_STATUS_CLOSED;
    return 0;
}

int sis_disk_io_map_control_remove(const char *fpath_, const char *fname_)
{
    s_sis_sds fname = sis_disk_io_map_get_fname(fpath_, fname_);
    if (sis_file_exists(fname))
    {
        sis_file_delete(fname);
        return 1;
    }
    return 0;
}

int sis_disk_io_map_w_data(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, void *in_, size_t ilen_)
{
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
    }
    int count = ilen_ / sdict->table->size; 
    // start write
    // printf("::: %d %d\n", fctrl->mapbctrl_idx->count, sdict->index);
    s_sis_map_bctrl  *ibctrl = sis_pointer_list_get(fctrl->mapbctrl_idx, sdict->index);
    s_sis_map_index *mindex  = sis_map_bctrl_get_var(ibctrl, kdict->index);
    s_sis_map_ksctrl *ksctrl = sis_map_kint_get(fctrl->map_kscs, sis_disk_io_map_get_ksidx(kdict->index, sdict->index));
    
    char *ptr = in_;
    for (int i = 0; i < count; i++)
    {
        s_sis_map_block *curblk = sis_map_ksctrl_last_head(ksctrl);
        int incrrec = 0;
        int incrblk = 0;
        if (mindex->currecs == mindex->perrecs)
        {
            // 新增块
            curblk->next = fctrl->maphead->useblks;
            fctrl->maphead->useblks++;  // 这里需要统一判断
            curblk = sis_map_block_head(fctrl, curblk->next);
            curblk->fin  = 1;
            curblk->zip  = 0;
            curblk->hid  = SIS_DISK_HID_MAP_DATA;
            sis_node_push(ksctrl->mapbctrl_var->nodes, curblk);
            incrblk = 1;
            incrrec = 1;            
            ksctrl->mapbctrl_var->currecs = 1;
        }
        else
        {
            incrrec = mindex->currecs + 1;
            ksctrl->mapbctrl_var->currecs++;
        }
        // 先写数据
        char *vmem = sis_map_bctrl_get_var(ksctrl->mapbctrl_var, mindex->sumrecs);
        memmove(vmem, ptr, sdict->table->size);
        // 这里可以判断写入什么内容
        ksctrl->mapbctrl_var->sumrecs++;
        sis_map_bctrl_set_sno(ksctrl->mapbctrl_var, mindex->sumrecs, fctrl->maphead->lastsno);
        fctrl->maphead->lastsno++;

        // 再更新索引
        mindex->vbinfo.blks += incrblk;
        mindex->currecs = incrrec;
        mindex->sumrecs ++;

        curblk->size = ksctrl->mapbctrl_var->currecs * ksctrl->mapbctrl_var->recsize;

        ptr += sdict->table->size;
    }
    return 0;
}
