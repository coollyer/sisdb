
#include "sis_disk.io.map.h"

////////////////////
// read 
////////////////////

void sis_disk_io_map_read_mindex(s_sis_map_fctrl *fctrl)
{
    // 仅仅读取块信息 其他信息不能读
    for (int si = 0; si < fctrl->mhead_r.sdbnums; si++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, si);
        s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_r.idxfbno[si]);
        int index = 0;
        while (1)
        {
            // sis_out_binary(":::", curblk, 32);
            // printf("si = %d %d  %d %d %zu %lld\n", si, fctrl->mhead_r.idxfbno[si], index, sdict->ksiblks->count, sizeof(s_sis_map_block), ((int64)curblk - (int64)fctrl->mapmem) / SIS_MAP_MIN_SIZE);
            // printf("si = %d\n", curblk->next);
            if (index == 0 && sdict->ksiblks->count == 0)
            {
                sis_int_list_push(sdict->ksiblks, fctrl->mhead_r.idxfbno[si]);
            }
            if (curblk->next == -1)
            {
                break;
            }
            if (index >= sdict->ksiblks->count)
            {
                sis_int_list_push(sdict->ksiblks, curblk->next);
            }
            index++;
            curblk = sis_map_block_head(fctrl, curblk->next);
        }
    }
}

void sis_disk_io_map_read_ksctrl(s_sis_map_fctrl *fctrl)
{
    // 先映射到文件
    for (int si = 0; si < fctrl->mhead_r.sdbnums; si++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(fctrl->map_sdbs, si);
        for (int ki = 0; ki < fctrl->mhead_r.keynums; ki++)
        {
            int64 ksidx = sis_disk_io_map_get_ksidx(ki, si);
            s_sis_map_kdict *kdict = sis_map_list_geti(fctrl->map_keys, ki);
            s_sis_map_ksctrl *ksctrl = sis_map_kints_get(fctrl->map_kscs, ksidx);
            if (!ksctrl)
            {
                ksctrl = sis_map_ksctrl_create(kdict, sdict);
                // ksctrl->mindex_r = 
                // ksctrl->varblks = 
                sis_map_kints_set(fctrl->map_kscs, ksidx, ksctrl); 
            }
            // 下面开始读数据
            ksctrl->mindex_p = sis_map_fctrl_get_mindex(fctrl, ksctrl->sdict, ksctrl->kdict->index);

            memmove(&ksctrl->mindex_r, ksctrl->mindex_p, sizeof(s_sis_map_index));   
            sis_int_list_clear(ksctrl->varblks);
            if (ksctrl->mindex_r.varfbno >= 0)
            {
                s_sis_map_block *curblk = sis_map_block_head(fctrl, ksctrl->mindex_r.varfbno);
                sis_int_list_push(ksctrl->varblks, ksctrl->mindex_r.varfbno);
                while (1)
                {
                    if (curblk->next == -1)
                    {
                        break;
                    }
                    sis_int_list_push(ksctrl->varblks, curblk->next);
                    curblk = sis_map_block_head(fctrl, curblk->next);
                }
            }             
        }
    } 
    printf("===1.2===\n");
}

void sis_disk_io_map_read_kdict(s_sis_map_fctrl *fctrl)
{
    printf("rkeys: %d\n", fctrl->mhead_r.keyfbno);
    sis_sdsfree(fctrl->wkeys); 
    fctrl->wkeys = sis_sdsempty();
    s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_r.keyfbno);
    while (1)
    {
        fctrl->wkeys = sis_sdscatlen(fctrl->wkeys, curblk->vmem, curblk->size);
        if (curblk->next == -1)
        {
            break;
        }
        curblk = sis_map_block_head(fctrl, curblk->next);
    }
    sis_disk_io_map_set_kdict(fctrl, fctrl->wkeys, sis_sdslen(fctrl->wkeys));
    fctrl->mhead_r.keynums = sis_map_list_getsize(fctrl->map_keys);
}
void sis_disk_io_map_read_sdict(s_sis_map_fctrl *fctrl)
{
    printf("rsdbs: %d\n", fctrl->mhead_r.sdbfbno);
    
    sis_sdsfree(fctrl->wsdbs); 
    fctrl->wsdbs = sis_sdsempty();
    s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->mhead_r.sdbfbno);
    while (1)
    {
        fctrl->wsdbs = sis_sdscatlen(fctrl->wsdbs, curblk->vmem, curblk->size);
        if (curblk->next == -1)
        {
            break;
        }
        curblk = sis_map_block_head(fctrl, curblk->next);
    }
    sis_disk_io_map_set_sdict(fctrl, fctrl->wsdbs, sis_sdslen(fctrl->wsdbs));
    fctrl->mhead_r.sdbnums = sis_map_list_getsize(fctrl->map_sdbs);
}
int sis_disk_io_map_check_mhead(s_sis_map_fctrl *fctrl, char *mapmem)
{
    int style = ((s_sis_disk_main_head *)mapmem)->style;
    if (style != SIS_DISK_TYPE_MDB && style != SIS_DISK_TYPE_MSN)
    {
        LOG(5)("map file style error.[%s]\n", fctrl->fname);
        return SIS_MAP_STYLE_ERR;
    }
    fctrl->style = style;
    s_sis_map_head *mhead_p = (s_sis_map_head *)(mapmem + sizeof(s_sis_disk_main_head));
    if (mhead_p->fsize != mhead_p->maxblks * mhead_p->bsize)
    {
        LOG(5)("map file is error.[%s]\n", fctrl->fname);
        return SIS_MAP_SIZE_ERR;
    }
    if ((mhead_p->keyfbno < 0) || (mhead_p->sdbfbno < 0))
    {
        LOG(5)("map file no ks.[%s]\n", fctrl->fname);
        return SIS_MAP_DATA_ERR;
    }

    return SIS_MAP_OPEN_OK;
}
void sis_disk_io_map_read_mhead(s_sis_map_fctrl *fctrl)
{
    fctrl->mhead_p = (s_sis_map_head *)(fctrl->mapmem + sizeof(s_sis_disk_main_head));
    int headsize = sizeof(s_sis_map_head) - SIS_MAP_OTHER_SIZE - (SIS_MAP_MAX_SDBNUM - fctrl->mhead_p->sdbnums) * sizeof(int);
    memmove(&fctrl->mhead_r, fctrl->mhead_p, headsize);    
    LOG(5)("%p %d | %d %d\n", fctrl->mapmem, fctrl->mhead_r.fsize, fctrl->mhead_r.keynums, fctrl->mhead_r.sdbnums);
}
int sis_disk_io_map_load(s_sis_map_fctrl *fctrl)
{
    // s_sis_disk_main_head *mhead = (s_sis_disk_main_head *)fctrl->mapmem;
    // if (mhead->style != SIS_DISK_TYPE_MDB)
    // {
    //     return SIS_MAP_STYLE_ERR;
    // }
    sis_map_rwlock_r_incr(fctrl->rwlock);
    if (sis_disk_io_map_check_mhead(fctrl, fctrl->mapmem) == SIS_MAP_OPEN_OK)
    {
        // 开始读取
        sis_disk_io_map_read_mhead(fctrl);
        // SIS_DISK_HID_DICT_KEY
        sis_disk_io_map_read_kdict(fctrl);
        // SIS_DISK_HID_DICT_SDB
        sis_disk_io_map_read_sdict(fctrl);
        // SIS_DISK_HID_MAP_INDEX
        sis_disk_io_map_read_mindex(fctrl);
        // SIS_DISK_HID_MAP_DATA
        sis_disk_io_map_read_ksctrl(fctrl);
    }
    sis_map_rwlock_r_decr(fctrl->rwlock);

    return SIS_MAP_OPEN_OK;
}

int sis_disk_io_map_r_open(s_sis_map_fctrl *fctrl, const char *fpath_, const char *fname_)
{
    fctrl->rwmode = 0;

    if (fctrl->status != SIS_MAP_STATUS_CLOSED)
    {
        // 已经打开并加载过了
        return 0;
    }
    
    if (!sis_file_exists(fctrl->fname))
    {
        LOG(5)("map file no exists.[%s]\n", fctrl->fname);
        fctrl->status |= SIS_MAP_STATUS_NOFILE;
        return -1;
    }
    else
    {
        size_t fsize = 0;
        fctrl->mapmem = sis_mmap_open_r(fctrl->fname, SIS_MAP_MIN_SIZE, &fsize);
        if (fctrl->mapmem == NULL)
        {
            LOG(5)("mmap open fail.[%s] %d\n", fctrl->fname, errno);
            return -2;
        }
        if (sis_disk_io_map_load(fctrl) != SIS_MAP_OPEN_OK)
        {
            sis_unmmap(fctrl->mapmem, fsize);
            LOG(5)("map file is no load.[%s]\n", fctrl->fname);
            return -3;
        }
        fctrl->status |= SIS_MAP_STATUS_OPENED;
    }
    return 0;
}

void sis_disk_io_map_r_unsub(s_sis_map_fctrl *fctrl)
{
    // 原子操作
    fctrl->sub_stop = 1;
}


int sis_disk_io_map_r_sub(s_sis_map_fctrl *fctrl, const char *keys_, const char *sdbs_, s_sis_msec_pair *mpair, void *cb_)
{
    s_sis_disk_reader_cb *callback = cb_;
    if (!callback)
    {
        return -1;
    }
    fctrl->sub_stop = 0;

    s_sis_map_subctrl *msubctrl = sis_map_subctrl_create(keys_, sdbs_, mpair, callback);

    // 初始化并定位到合适的记录
    sis_map_subctrl_init(msubctrl, fctrl);
    
    if(callback->cb_open)
    {
        callback->cb_open(callback->cb_source, sis_msec_get_idate(mpair->start));
    }
    if (callback->cb_dict_keys)
    {
        callback->cb_dict_keys(callback->cb_source, fctrl->wkeys, sis_sdslen(fctrl->wkeys));
    }
    if (callback->cb_dict_sdbs)
    {
        callback->cb_dict_sdbs(callback->cb_source, fctrl->wsdbs, sis_sdslen(fctrl->wsdbs));
    }
    if (fctrl->style == SIS_DISK_TYPE_MSN)
    {
        if (fctrl->sub_mode == SIS_MAP_SUB_TSNO)
        {
            sis_map_subctrl_sub_sno(msubctrl, fctrl);
        }
        else // if (fctrl->sub_mode == SIS_MAP_SUB_TSNO)
        {
            sis_map_subctrl_sub_data(msubctrl, fctrl);
        }
    }
    else
    {
        sis_map_subctrl_sub_data(msubctrl, fctrl);
    }

    if(callback->cb_close)
    {
        callback->cb_close(callback->cb_source, sis_msec_get_idate(mpair->stop));
    }
    sis_map_subctrl_destroy(msubctrl);
    return 0;
}

s_sis_object *sis_disk_io_map_r_get_obj(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, s_sis_msec_pair *smsec_)
{
    s_sis_map_sdict *sdict = sis_map_list_get(fctrl->map_sdbs, sname_);
    if (!sdict)
    {
        return NULL;
    }
    s_sis_map_kdict *kdict = sis_map_list_get(fctrl->map_keys, kname_);
    if (!kdict)
    {
        return NULL;
    }
    s_sis_map_ksctrl *ksctrl = sis_map_kints_get(fctrl->map_kscs, sis_disk_io_map_get_ksidx(kdict->index, sdict->index));
    if (!ksctrl || ksctrl->mindex_r.sumrecs < 1)
    {
        return NULL;
    }
    
    s_sis_memory *memory = sis_memory_create();

    for (int i = 0; i < ksctrl->varblks->count; i++)
    {
        int blkno = sis_int_list_get(ksctrl->varblks, i);
        char *var = (char *)sis_map_ksctrl_get_fbvar(fctrl, ksctrl, blkno);

        int recs = 0;
        if (i == ksctrl->varblks->count - 1)
        {   
            recs = ksctrl->mindex_r.currecs;
        }
        else
        {
            recs = ksctrl->mindex_r.perrecs;
        }
        size_t msize = recs * ksctrl->mindex_r.recsize;

        if ((smsec_->start == 0 && smsec_->stop == 0) ||
            (!sdict->table->field_time || sdict->table->field_time->len != 8))
        {
            sis_memory_cat(memory, var, msize);
            sis_memory_move(memory, msize);
        }
        else
        {
            // 这是最简单的方法 效率不高 以后有需要再优化
            for (int i = 0; i < recs; i++)
            {
                msec_t *curmsec = (msec_t *)(var + sdict->table->field_time->offset);
                if ((*curmsec >= smsec_->start || smsec_->start == 0) &&
                    (*curmsec <= smsec_->stop || smsec_->stop == 0)) 
                {
                    sis_memory_cat(memory, var, sdict->table->size);
                    sis_memory_move(memory, sdict->table->size);
                }
                var += ksctrl->mindex_r.recsize;
            }
        }
    }
    if (sis_memory_get_size(memory) > 0)
    {
        return sis_object_create(SIS_OBJECT_MEMORY, memory);
    }
    return NULL;
}

int sis_map_ksctrl_find_start_cursor(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, s_sis_msec_pair *msec)
{
    int agos = 0;
    if (msec->start == 0 && msec->stop == 0)
    {
        // 等最新的数据 和 get 完全不同
        return ksctrl->mindex_r.sumrecs;
    }
    else
    {
        if (msec->start == 0 || (!ksctrl->sdict->table->field_time || ksctrl->sdict->table->field_time->len != 8))
        {
            return 0;
        }
        else
        {
            for (int i = 0; i < ksctrl->varblks->count; i++)
            {
                int blkno = sis_int_list_get(ksctrl->varblks, i);
                char *var = (char *)sis_map_ksctrl_get_fbvar(fctrl, ksctrl, blkno);
                int recs = 0;
                if (i == ksctrl->varblks->count - 1)
                {   
                    recs = ksctrl->mindex_r.currecs;
                }
                else
                {
                    recs = ksctrl->mindex_r.perrecs;
                }
                // 这是最简单的方法 效率不高 以后有需要再优化
                for (int i = 0; i < recs; i++)
                {
                    msec_t *curmsec = (msec_t *)(var + ksctrl->sdict->table->field_time->offset);
                    if (*curmsec >= msec->start) 
                    {
                        return i + agos;
                    }
                    var += ksctrl->mindex_r.recsize;
                }
                agos += recs;
            }
        }
    }
    return agos;
}

////////////////////
// s_sis_map_subctrl 
////////////////////

s_sis_map_subctrl *sis_map_subctrl_create(const char *keys, const char *sdbs, s_sis_msec_pair *mpair, s_sis_disk_reader_cb *callback)
{
    s_sis_map_subctrl *subctrl = SIS_MALLOC(s_sis_map_subctrl, subctrl);
    subctrl->sub_keys = sis_sdsnew(keys);
    subctrl->sub_sdbs = sis_sdsnew(sdbs);
    memmove(&subctrl->mpair, mpair, sizeof(s_sis_msec_pair));
    subctrl->callback = callback;
    subctrl->subvars  = sis_pointer_list_create();
    subctrl->subvars->vfree = sis_free_call;
    subctrl->aisleep = sis_aisleep_create(10, 1000000);
    return subctrl;
}

void sis_map_subctrl_destroy(s_sis_map_subctrl *subctrl)
{
    sis_sdsfree(subctrl->sub_keys);
    sis_sdsfree(subctrl->sub_sdbs);
    sis_pointer_list_destroy(subctrl->subvars);
    sis_aisleep_destroy(subctrl->aisleep);
    sis_free(subctrl);
}
void sis_map_subinfo_set_fd(s_sis_map_subinfo *subinfo, s_sis_map_fctrl *fctrl)
{
    if (subinfo->nottfd == 0)
    {
        subinfo->timefd = sis_map_ksctrl_get_timefd(fctrl, subinfo->ksctrl, subinfo->cursor);
    }
    if (fctrl->sub_mode == SIS_MAP_SUB_DATA)
    {
        subinfo->sortfd = subinfo->timefd;
    }
    else
    {
        subinfo->sortfd = sis_map_ksctrl_get_sno(fctrl, subinfo->ksctrl, subinfo->cursor);
        printf(" getsno == : %p %lld %d %d\n", subinfo->sortfd, subinfo->sortfd? *subinfo->sortfd :0, subinfo->cursor, subinfo->ksctrl->mindex_r.sumrecs);
    }
}
int sis_map_subctrl_init(s_sis_map_subctrl *subctrl, s_sis_map_fctrl *fctrl)
{
    sis_pointer_list_clear(subctrl->subvars);
    int count = sis_map_kints_getsize(fctrl->map_kscs);
    for (int i = 0; i < count; i++)
    {
        s_sis_map_ksctrl *ksctrl = sis_map_kints_geti(fctrl->map_kscs, i);
        if (subctrl->mpair.stop > 0 && ksctrl->mindex_r.sumrecs < 1)
        {
            // 如果是限定时间 并且没有数据就不往下面写了
            continue;
        }
        if ((!sis_strcasecmp(subctrl->sub_sdbs, "*") || sis_str_subcmp_strict(ksctrl->sdict->table->name, subctrl->sub_sdbs, ',') >= 0) &&
            (!sis_strcasecmp(subctrl->sub_keys, "*") || sis_str_subcmp(ksctrl->kdict->kname, subctrl->sub_keys, ',') >= 0))
        {
            s_sis_map_subinfo *subinfo = SIS_MALLOC(s_sis_map_subinfo, subinfo);
            subinfo->ksctrl = ksctrl;
            sis_pointer_list_push(subctrl->subvars, subinfo);
            if (ksctrl->sdict->table->field_time && ksctrl->sdict->table->field_time->len == 8)
            {
                // 取得合适的时间 
                subinfo->cursor = sis_map_ksctrl_find_start_cursor(fctrl, ksctrl, &subctrl->mpair);
                // 没有记录 会返回 0 
                // 有记录 时间对不上 返回未初始化的记录号 subinfo->cursor >= bctrl->sumrecs
            }
            else
            {
                subinfo->nottfd = 1;
                subinfo->timefd = NULL;
            }
            sis_map_subinfo_set_fd(subinfo, fctrl);
            printf("== adda : %d %d %lld %s %s\n",subinfo->cursor, ksctrl->mindex_r.sumrecs, subinfo->sortfd ? *subinfo->sortfd: -1, ksctrl->kdict->kname, ksctrl->sdict->table->name);
        }     

    } 
    return 0;
}

int sis_map_subctrl_sub_sno(s_sis_map_subctrl *msubctrl, s_sis_map_fctrl *fctrl)
{
    s_sis_disk_reader_cb *callback = msubctrl->callback;
    int startdate = sis_msec_get_idate(msubctrl->mpair.start);

    printf("===3=== %d %d\n", startdate, msubctrl->subvars->count);
    // 设置 minpubsv 的值
    // 这里默认数据文件必须按时序写入 不处理增量非时序写入的数据
    int64 minpub_snoi = -1;
    int64 minpub_msec = 0;
    for (int i = 0; i < msubctrl->subvars->count; i++)
    {
        s_sis_map_subinfo *subinfo = sis_pointer_list_get(msubctrl->subvars, i);
        if (subinfo->cursor >= subinfo->ksctrl->mindex_r.sumrecs)
        {
            // 表示数据消耗完毕
            continue;
        }
        if (subinfo->sortfd == NULL)
        {
            // 无排序字段的去掉 不可能存在
            continue;
        }
        int64 cursno = *(subinfo->sortfd);
        if (minpub_snoi > cursno || minpub_snoi == -1)
        {
            minpub_snoi = cursno;
            if (subinfo->nottfd == 0 && subinfo->timefd)
            {
                minpub_msec = *(subinfo->timefd);
            }
        }
    }    
    if (minpub_msec > 0)
    {
        startdate = sis_msec_get_idate(minpub_msec);
    }
    // 取最小时间
    // msubctrl->curpub_msec = minpub_msec;
    // 开始循环处理时序数据
    int minsize = SIS_MAXI(SIS_MAP_SUB_SIZE * 32, msubctrl->subvars->count * 3);
    s_sis_map_subsno *vars = sis_malloc(minsize * sizeof(s_sis_map_subsno));
    int workdate = 0;
    int sendnums = 0;
    while (1)
    {
        memset((char *)vars, 0, minsize * sizeof(s_sis_map_subsno));
        printf("============= %d || %d %d %lld\n", sendnums, msubctrl->subvars->count, workdate, minpub_snoi);
        
        int   sends = 0;
        // int64 minvar_msec = 0;
        int64 mingap_snoi = -1;
        for (int i = 0; i < msubctrl->subvars->count; i++)
        {
            s_sis_map_subinfo *subinfo = sis_pointer_list_get(msubctrl->subvars, i);
            if (fctrl->sub_stop == 1)
            {
                // 通知订阅者文件读取已中断
                if(callback->cb_break)
                {
                    callback->cb_break(callback->cb_source, 0);
                }
                fctrl->sub_stop = 0;
                sis_free(vars);
                return -1;
            }
            // printf("===4.1=== %d %d %d\n", i, subinfo->cursor, subinfo->ksctrl->mindex_r.sumrecs);
            if (subinfo->cursor >= subinfo->ksctrl->mindex_r.sumrecs)
            {
                // 表示数据消耗完毕
                continue;
            }
            if (subinfo->sortfd == NULL)
            {
                // 表示有新数据来了
                sis_map_subinfo_set_fd(subinfo, fctrl);
            }
            while (1)
            {
                msec_t curmsec = 0;
                printf("===4.2===%s.%s %d %lld %lld %lld | %d \n", subinfo->ksctrl->kdict->kname, subinfo->ksctrl->sdict->table->name,
                    i, curmsec, subinfo->sortfd ? *subinfo->sortfd : -1, subinfo->timefd ? *subinfo->timefd : 0, subinfo->ksctrl->mindex_p->perrecs);
                if (subinfo->timefd)
                {
                    curmsec = *(subinfo->timefd);
                    if (msubctrl->mpair.stop > 0 && curmsec > msubctrl->mpair.stop)
                    {
                        // 时间超过
                        sis_pointer_list_delete(msubctrl->subvars, i, 1);
                        i--;
                        break;
                    }
                    // if ((curmsec > 0 && curmsec < minvar_msec) || minvar_msec == 0)
                    // {
                    //     minvar_msec = curmsec;
                    // }
                }
                int64 cursno = *(subinfo->sortfd);
                if (cursno < minpub_snoi)
                {
                    LOG(3)("sub error. %lld > %lld\n", minpub_snoi, cursno);
                    // 序列号小的应该是程序出问题了
                    sis_pointer_list_delete(msubctrl->subvars, i, 1);
                    i--;
                    break;
                }
                int64 subv = cursno - minpub_snoi;
                if (subv >= minsize)
                {
                    // 跨度大 不处理
                    if (mingap_snoi > subv || mingap_snoi == -1)
                    {
                        mingap_snoi = subv;
                    }
                    break;
                }
                printf("%lld %d %lld %lld\n", subv, minsize, cursno, minpub_snoi);
                vars[subv].inmem = sis_map_ksctrl_get_var(fctrl, subinfo->ksctrl, subinfo->cursor);
                vars[subv].idate = curmsec ? sis_msec_get_idate(curmsec) : startdate;
                vars[subv].ksctrl = subinfo->ksctrl;
                subinfo->cursor ++;
                sends ++;
                sis_map_subinfo_set_fd(subinfo, fctrl);
                printf("===4.3=== %d %lld %d %lld %p %d\n", sends, subv, subinfo->cursor, subinfo->sortfd ? *subinfo->sortfd : 0, subinfo->sortfd, 
                    subinfo->ksctrl->mindex_r.sumrecs);
                if (subinfo->cursor >= subinfo->ksctrl->mindex_r.sumrecs)
                {
                    break;
                }
            }
        }
        // printf("===5=== %d %d %d %lld %lld\n", sends, msubctrl->subvars->count, workdate, minpub_snoi, mingap_snoi);
        if (sends == 0)
        {
            if (msubctrl->mpair.stop > 0)
            {
                if(callback->cb_stop)
                {
                    callback->cb_stop(callback->cb_source, workdate ? workdate : sis_msec_get_idate(msubctrl->mpair.stop));
                }
                sis_free(vars);
                return 0;
            }
            else
            {
                // 这里处理新增数据的检查 
                if (sis_map_subctrl_check(msubctrl, fctrl))
                {
                    // 重载出错
                    sis_free(vars);
                    return -1;
                }
                sis_aisleep(msubctrl->aisleep, 0);
            }
        }
        else
        {
            sis_aisleep(msubctrl->aisleep, 1);
            for (int i = 0; i < minsize && sends > 0; i++)
            {
                if (vars[i].ksctrl == NULL)
                {
                    continue;
                }
                printf("========= sends = %d %d [%d] %s %s\n", i, sends, ++sendnums, vars[i].ksctrl->kdict->kname, vars[i].ksctrl->sdict->table->name);
                {   // 这里处理开始结束回调
                    int currdate = vars[i].idate;
                    if (workdate == 0)
                    {
                        workdate = currdate;
                        if (callback->cb_start)
                        {
                            callback->cb_start(callback->cb_source, workdate);
                        }
                    }
                    else if (workdate > 0 && currdate > workdate)
                    {
                        // 通知订阅者文件读取已正常结束
                        if (callback->cb_stop)
                        {
                            callback->cb_stop(callback->cb_source, workdate);
                        }
                        workdate = currdate;
                        if (callback->cb_start)
                        {
                            callback->cb_start(callback->cb_source, workdate);
                        }
                    }
                }
                if (callback->cb_chardata)
                {
                    callback->cb_chardata(callback->cb_source, vars[i].ksctrl->kdict->kname, vars[i].ksctrl->sdict->table->name, vars[i].inmem, vars[i].ksctrl->sdict->table->size);
                }
                sends--;
            }
            
        }
        minpub_snoi += mingap_snoi == -1 ? 0 : mingap_snoi;
    }
    sis_free(vars);
    return 1;
}
int sis_map_subctrl_sub_data(s_sis_map_subctrl *msubctrl, s_sis_map_fctrl *fctrl)
{    
    s_sis_disk_reader_cb *callback = msubctrl->callback;
    int startdate = sis_msec_get_idate(msubctrl->mpair.start);

    // 设置 minpub_msec 的值
    int64 minpub_msec = 0;
    for (int i = 0; i < msubctrl->subvars->count; i++)
    {
        s_sis_map_subinfo *subinfo = sis_pointer_list_get(msubctrl->subvars, i);
        if (subinfo->cursor >= subinfo->ksctrl->mindex_r.sumrecs)
        {
            // 表示数据消耗完毕
            continue;
        }
        if (subinfo->nottfd == 1 || subinfo->timefd == NULL)
        {
            // 无时间字段的去掉
            continue;
        }
        minpub_msec = SIS_MINI(minpub_msec, *(subinfo->timefd));
    }
    // 开始循环处理时序数据
    if (minpub_msec > 0)
    {
        startdate = sis_msec_get_idate(minpub_msec);
    }
    msubctrl->curpub_msec = minpub_msec;
    int sendnums = 0;
    int workdate = 0;
    while (1)
    {
        printf("============= %d || %d %d %lld\n", sendnums, msubctrl->subvars->count, workdate, msubctrl->curpub_msec);
        int   sends = 0;
        int64 mingap_msec = 0;  // 记录最小的差额 避免隔天时快速定位数据
        for (int i = 0; i < msubctrl->subvars->count; i++)
        {
            s_sis_map_subinfo *subinfo = sis_pointer_list_get(msubctrl->subvars, i);
            if (fctrl->sub_stop == 1)
            {
                // 通知订阅者文件读取已中断
                if(callback->cb_break)
                {
                    callback->cb_break(callback->cb_source, 0);
                }
                fctrl->sub_stop = 0;
                return -1;
            }
            if (subinfo->cursor >= subinfo->ksctrl->mindex_r.sumrecs)
            {
                // 表示数据消耗完毕
                continue;
            }
            if (subinfo->nottfd == 1)
            {
                // 先处理没有时间字段的
                while (subinfo->cursor < subinfo->ksctrl->mindex_r.sumrecs)
                {
                    if (workdate == 0)
                    {
                        workdate = startdate;
                        if(callback->cb_start)
                        {
                            callback->cb_start(callback->cb_source, workdate);
                        }
                    }
                    if (callback->cb_chardata)
                    {
                        char *var = sis_map_ksctrl_get_var(fctrl, subinfo->ksctrl, subinfo->cursor);
                        callback->cb_chardata(callback->cb_source, subinfo->ksctrl->kdict->kname, subinfo->ksctrl->sdict->table->name, var, subinfo->ksctrl->sdict->table->size);
                        subinfo->cursor ++;
                        // sis_map_subinfo_set_fd(subinfo, fctrl);
                        sends ++;
                    }
                }
                // 无时间字段的发送万就剔除了
                sis_pointer_list_delete(msubctrl->subvars, i, 1);
                i--;
                continue;
            }
            if (subinfo->timefd == NULL)
            {
                // 表示有新数据来了 重新定位时间和排序字段
                sis_map_subinfo_set_fd(subinfo, fctrl);
            }
            while (1)
            {
                msec_t curmsec = *(subinfo->timefd);
                printf("===6.1===%d %lld %lld %lld\n", subinfo->cursor, subinfo->ksctrl->ksidx, curmsec, msubctrl->mpair.stop);
                if (msubctrl->mpair.stop > 0 && curmsec > msubctrl->mpair.stop)
                {
                    // 时间超过 不再处理 除非不限制日期
                    sis_pointer_list_delete(msubctrl->subvars, i, 1);
                    i--;
                    break;
                }
                int64 subv = curmsec - msubctrl->curpub_msec;
                // printf("===6.1=== %lld %lld \n", subinfo->ksctrl->ksidx, subv);
                if (subv > SIS_MAP_MIN_DIFF)
                {
                    // 时间跨度大 不处理
                    mingap_msec = SIS_MINI(mingap_msec, subv);
                    break;
                }
                if (subv < 0 && msubctrl->mpair.start > 0 && curmsec < msubctrl->mpair.start)
                {
                    // 数据中时序错误 不再处理 除非不限制日期
                    sis_pointer_list_delete(msubctrl->subvars, i, 1);
                    i--;
                    break;
                }
                printf("===6.2===%s.%s %d %lld  %lld | %lld %lld\n", subinfo->ksctrl->kdict->kname, subinfo->ksctrl->sdict->table->name,
                    i, curmsec, subv, *subinfo->sortfd, subinfo->timefd ? *subinfo->timefd : 0);

                {   // 这里处理开始结束回调
                    int currdate = sis_msec_get_idate(curmsec);
                    if (workdate == 0)
                    {
                        workdate = currdate;
                        if (callback->cb_start)
                        {
                            callback->cb_start(callback->cb_source, workdate);
                        }
                    }
                    else if (workdate > 0 && currdate > workdate)
                    {
                        // 通知订阅者文件读取已正常结束
                        if (callback->cb_stop)
                        {
                            callback->cb_stop(callback->cb_source, workdate);
                        }
                        workdate = currdate;
                        if (callback->cb_start)
                        {
                            callback->cb_start(callback->cb_source, workdate);
                        }
                    }
                }
                if (callback->cb_chardata)
                {
                    char *var = sis_map_ksctrl_get_var(fctrl, subinfo->ksctrl, subinfo->cursor);
                    callback->cb_chardata(callback->cb_source, subinfo->ksctrl->kdict->kname, subinfo->ksctrl->sdict->table->name, var, subinfo->ksctrl->sdict->table->size);
                    subinfo->cursor ++;
                    sends ++;
                    sis_map_subinfo_set_fd(subinfo, fctrl);
                }
                printf("===6.3=== %d %d %d\n", ++sendnums, subinfo->cursor, subinfo->ksctrl->mindex_r.sumrecs);
                if (subinfo->cursor >= subinfo->ksctrl->mindex_r.sumrecs)
                {
                    break;
                }
            }
        }
        if (sends == 0)
        {
            if (msubctrl->mpair.stop > 0)
            {
                if(callback->cb_stop)
                {
                    callback->cb_stop(callback->cb_source, workdate ? workdate : sis_msec_get_idate(msubctrl->mpair.stop));
                }
                return 0;
            }
            else
            {
                if (sis_map_subctrl_check(msubctrl, fctrl))
                {
                    // 重载出错
                    return -1;
                }
                sis_aisleep(msubctrl->aisleep, 0);
            }
        }
        else
        {
            sis_aisleep(msubctrl->aisleep, 1);
        }
        msubctrl->curpub_msec += mingap_msec;
    }
    return 1;
}

int sis_map_subctrl_check(s_sis_map_subctrl *msubctrl, s_sis_map_fctrl *fctrl)
{
    // 检查是否新增sdb 且在订阅内 如果有定位时间到 msubctrl->curpub_msec
    // 检查是否新增key 且在订阅内 如果有定位时间到 msubctrl->curpub_msec
    // 检查正在订阅的数据 sumrecs 值是否有增加 有就加锁修改 

    int newsdb = 0;
    int newkey = 0;
    int reload = 0;
    sis_map_rwlock_r_incr(fctrl->rwlock);
    if (fctrl->mhead_p->sdbnums > fctrl->mhead_r.sdbnums)
    {
        newsdb = 1;
    }
    if (fctrl->mhead_p->keynums > fctrl->mhead_r.keynums)
    {
        newkey = 1;
    }
    if (fctrl->mhead_p->fsize > fctrl->mhead_r.fsize)
    {
        reload = 1;
    }
    if (reload)
    {
        size_t fsize = 0;
        char *mmap = sis_mmap_open_r(fctrl->fname, SIS_MAP_MIN_SIZE, &fsize);
        if (mmap == NULL)
        {
            LOG(5)("mmap open fail.[%s] %d\n", fctrl->fname, errno);
            sis_map_rwlock_r_decr(fctrl->rwlock);
            return SIS_MAP_MMAP_ERR;
        }
        int o = sis_disk_io_map_check_mhead(fctrl, mmap);
        if (o != SIS_MAP_OPEN_OK)
        {
            sis_unmmap(mmap, fsize);
            sis_map_rwlock_r_decr(fctrl->rwlock);
            return o;
        }
        sis_unmmap(fctrl->mapmem, fctrl->mhead_r.fsize);
        fctrl->mapmem = mmap;
    }
    if (reload || newsdb || newkey)
    {
        // 开始读取
        sis_disk_io_map_read_mhead(fctrl);
        // SIS_DISK_HID_DICT_KEY
        if (newkey)
        {
            sis_disk_io_map_read_kdict(fctrl);
        }
        // SIS_DISK_HID_DICT_SDB
        if (newsdb)
        {
            sis_disk_io_map_read_sdict(fctrl);
        }
    }
    // 每次都需要刷新索引数据
    // SIS_DISK_HID_MAP_INDEX
    sis_disk_io_map_read_mindex(fctrl);

    if (reload || newsdb || newkey)
    {
        // sis_disk_io_map_read_ksctrl(fctrl);
        if (newkey)
        {
            // 这里判断是否增加订阅 暂时不管
        }
    }

    // ksctrl 在这里更新加锁
    for (int i = 0; i < msubctrl->subvars->count; i++)
    {
        s_sis_map_subinfo *subinfo = sis_pointer_list_get(msubctrl->subvars, i);
        s_sis_map_ksctrl *ksctrl = subinfo->ksctrl;
        ksctrl->mindex_p = sis_map_fctrl_get_mindex(fctrl, ksctrl->sdict, ksctrl->kdict->index);
        if (ksctrl->mindex_p->sumrecs <= ksctrl->mindex_r.sumrecs)
        {
            continue;
        }
        if (ksctrl->mindex_p->varfbno == -1)
        {
            continue;
        }
        int newblks = ksctrl->mindex_p->sumrecs / ksctrl->mindex_r.perrecs + 1;
        if (newblks > ksctrl->varblks->count)
        {
            if (ksctrl->varblks->count == 0)
            {
                sis_int_list_push(ksctrl->varblks, ksctrl->mindex_p->varfbno);
            }
            while (newblks > ksctrl->varblks->count)
            {
                int blkno = sis_int_list_get(ksctrl->varblks, ksctrl->varblks->count - 1);
                s_sis_map_block *curblk = sis_map_block_head(fctrl, blkno);
                if (curblk->next == -1)
                {
                    break;
                }
                sis_int_list_push(ksctrl->varblks, curblk->next);               
            }
        }
        memmove(&ksctrl->mindex_r, ksctrl->mindex_p, sizeof(s_sis_map_index));                
    }
    sis_map_rwlock_r_decr(fctrl->rwlock);  

    return 0;
}
