
#include "sis_disk.io.map.h"


////////////////////
// read 
////////////////////


void sis_disk_io_map_read_ksctrl(s_sis_map_fctrl *fctrl)
{
    printf("===1.1===\n");
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
            // 这里定位索引数据
            s_sis_map_index *mindex = sis_map_bctrl_get_var(bctrl, k);
            // 这里加载数据节点信息
            ksctrl->mapbctrl_var->currecs = 0;
            ksctrl->mapbctrl_var->sumrecs = 0;
            while(1)
            {
                s_sis_map_block *curblk = sis_map_block_head(fctrl, mindex->vbinfo.fbno);
                sis_node_push(ksctrl->mapbctrl_var->nodes, curblk);
                printf("===1.1=== %d %d\n", curblk->next, curblk->size);
                ksctrl->mapbctrl_var->currecs = curblk->size / sdict->table->size;
                ksctrl->mapbctrl_var->sumrecs+= ksctrl->mapbctrl_var->currecs;
                if (curblk->next == -1)
                {
                    break;
                }
                curblk = sis_map_block_head(fctrl, curblk->next);
            }
        }
    }
    printf("===1.1===\n");
}

void sis_disk_io_map_read_mindex(s_sis_map_fctrl *fctrl)
{
    // 先映射到文件
    printf("===1.2===\n");
    sis_pointer_list_clear(fctrl->mapbctrl_idx);
    for (int i = 0; i < fctrl->maphead->sdbnums; i++)
    {
        s_sis_map_bctrl *bctrl = sis_map_bctrl_create(0, sizeof(s_sis_map_index));
        sis_pointer_list_push(fctrl->mapbctrl_idx, bctrl);
        bctrl->sumrecs = fctrl->maphead->keynums;
        bctrl->currecs = bctrl->sumrecs % bctrl->perrecs;
        
        sis_node_clear(bctrl->nodes);
        while(1)
        {
            s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->ibinfo[i].fbno);
            sis_node_push(bctrl->nodes, curblk);
            printf("===1.2=== %d %d\n", curblk->next, curblk->size);
            if (curblk->next == -1)
            {
                break;
            }
            curblk = sis_map_block_head(fctrl, curblk->next);
        }
    }
    printf("===1.2===\n");
}
void sis_disk_io_map_read_kdict(s_sis_map_fctrl *fctrl)
{
    sis_sdsfree(fctrl->wkeys); 
    fctrl->wkeys = sis_sdsempty();
    s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->kbinfo.fbno);
    char *curmem = sis_map_block_memory(fctrl, fctrl->maphead->kbinfo.fbno);
    while (1)
    {
        fctrl->wkeys = sis_sdscatlen(fctrl->wkeys, curmem, curblk->size);
        if (curblk->next == -1)
        {
            break;
        }
        curmem = sis_map_block_memory(fctrl, curblk->next);
        curblk = sis_map_block_head(fctrl, curblk->next);
    }
    sis_disk_io_map_set_kdict(fctrl, fctrl->wkeys, sis_sdslen(fctrl->wkeys));
}
void sis_disk_io_map_read_sdict(s_sis_map_fctrl *fctrl)
{
    printf("fbno: %d %d\n", fctrl->maphead->kbinfo.fbno, fctrl->maphead->sbinfo.fbno);
    sis_sdsfree(fctrl->wsdbs); 
    fctrl->wsdbs = sis_sdsempty();
    s_sis_map_block *curblk = sis_map_block_head(fctrl, fctrl->maphead->sbinfo.fbno);
    char *curmem = sis_map_block_memory(fctrl, fctrl->maphead->sbinfo.fbno);
    while (1)
    {
        fctrl->wsdbs = sis_sdscatlen(fctrl->wsdbs, curmem, curblk->size);
        if (curblk->next == -1)
        {
            break;
        }
        curmem = sis_map_block_memory(fctrl, curblk->next);
        curblk = sis_map_block_head(fctrl, curblk->next);
    }
    sis_disk_io_map_set_sdict(fctrl, fctrl->wsdbs, sis_sdslen(fctrl->wsdbs));
}

int sis_disk_io_map_load(s_sis_map_fctrl *fctrl)
{
    // 同步
    // sis_mmap_sync(fctrl->mapmem, fctrl->maphead->fsize);
    // 
    s_sis_disk_main_head *mhead = (s_sis_disk_main_head *)fctrl->mapmem;
    if (mhead->style != SIS_DISK_TYPE_MAP)
    {
        return SIS_MAP_STYLE_ERR;
    }
    fctrl->maphead = (s_sis_map_ctrl *)(fctrl->mapmem + sizeof(s_sis_disk_main_head));
    printf("%p %d | %d %d\n", fctrl->mapmem, fctrl->maphead->fsize, fctrl->maphead->keynums, fctrl->maphead->sdbnums);
    if (fctrl->maphead->fsize != (fctrl->maphead->maxblks + 1) * fctrl->maphead->bsize)
    {
        return SIS_MAP_SIZE_ERR;
    }
    if ((fctrl->maphead->kbinfo.fbno < 0) || (fctrl->maphead->sbinfo.fbno < 0))
    {
        return SIS_MAP_DATA_ERR;
    }
    // 开始读取
    // SIS_DISK_HID_DICT_KEY
    sis_disk_io_map_read_kdict(fctrl);
    // SIS_DISK_HID_DICT_SDB
    sis_disk_io_map_read_sdict(fctrl);
    // SIS_DISK_HID_MAP_INDEX
    sis_disk_io_map_read_mindex(fctrl);
    // SIS_DISK_HID_MAP_DATA
    sis_disk_io_map_read_ksctrl(fctrl);
    return SIS_MAP_LOAD_OK;
}

int sis_disk_io_map_r_open(s_sis_map_fctrl *fctrl, const char *fpath_, const char *fname_)
{
    sis_sdsfree(fctrl->fname);
    fctrl->fname = sis_disk_io_map_get_fname(fpath_, fname_);

    fctrl->rwmode = 0;

    if (!sis_file_exists(fctrl->fname))
    {
        LOG(5)("map file no exists.[%s]\n", fctrl->fname);
        return -1;
    }
    else
    {
        if (sis_disk_io_map_open(fctrl) != SIS_DISK_CMD_OK)
        {
            LOG(5)("map file is no valid.[%s]\n", fctrl->fname);
            return -2;
        }
        if (sis_disk_io_map_load(fctrl) != SIS_MAP_LOAD_OK)
        {
            LOG(5)("map file is no load.[%s]\n", fctrl->fname);
            return -3;
        }
        fctrl->status |= SIS_DISK_STATUS_OPENED;
    }
    return 0;
}

void sis_disk_io_map_r_unsub(s_sis_map_fctrl *fctrl)
{
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
    if (fctrl->sub_mode == SIS_MAP_SUB_TSNO)
    {
        sis_map_subctrl_sub_sno(msubctrl, fctrl);
    }
    else // if (fctrl->sub_mode == SIS_MAP_SUB_TSNO)
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
    s_sis_map_ksctrl *ksctrl = sis_map_kint_get(fctrl->map_kscs, sis_disk_io_map_get_ksidx(kdict->index, sdict->index));
    if (!ksctrl || ksctrl->mapbctrl_var->sumrecs < 1)
    {
        return NULL;
    }
    
    s_sis_memory *memory = sis_memory_create();
    s_sis_map_bctrl *bctrl = ksctrl->mapbctrl_var;

    s_sis_node *next = sis_node_next(bctrl->nodes);
    while (next)
    {
        char *ptr = next->value;
        s_sis_map_block *curblk = (s_sis_map_block *)ptr;
        ptr += sizeof(s_sis_map_block);
        if (bctrl->needsno)
        {
            ptr += ksctrl->mapbctrl_var->perrecs * 8;
        }
        if ((smsec_->start == 0 && smsec_->stop == 0) ||
            (!sdict->table->field_time || sdict->table->field_time->len != 8))
        {
            sis_memory_cat(memory, ptr, curblk->size);
            sis_memory_move(memory, curblk->size);
        }
        else
        {
            int recs = curblk->size / ksctrl->mapbctrl_var->recsize;
            // 这是最简单的方法 效率不高 以后有需要再优化
            for (int i = 0; i < recs; i++)
            {
                msec_t *curmsec = (msec_t *)(ptr + sdict->table->field_time->offset);
                if ((*curmsec >= smsec_->start || smsec_->start == 0) &&
                    (*curmsec <= smsec_->stop || smsec_->stop == 0)) 
                {
                    sis_memory_cat(memory, ptr, sdict->table->size);
                    sis_memory_move(memory, sdict->table->size);
                }
                ptr += ksctrl->mapbctrl_var->recsize;
            }
        }
        next = sis_node_next(next);
    }
    if (sis_memory_get_size(memory) > 0)
    {
        return sis_object_create(SIS_OBJECT_MEMORY, memory);
    }
    return NULL;
}

int sis_map_ksctrl_find_start_cursor(s_sis_map_ksctrl *ksctrl, s_sis_msec_pair *msec)
{
    if (msec->start == 0 && msec->stop == 0)
    {
        // 等最新的数据 和 get 完全不同
        return ksctrl->mapbctrl_var->sumrecs;
    }
    else
    {
        if (msec->start == 0 || (!ksctrl->sdict->table->field_time || ksctrl->sdict->table->field_time->len != 8))
        {
            return 0;
        }
        else
        {
            s_sis_map_bctrl *bctrl = ksctrl->mapbctrl_var;
            s_sis_node *next = sis_node_next(bctrl->nodes);
            int agos = 0;
            while (next)
            {
                char *ptr = next->value;
                s_sis_map_block *curblk = (s_sis_map_block *)ptr;
                ptr += sizeof(s_sis_map_block);
                if (bctrl->needsno)
                {
                    ptr += ksctrl->mapbctrl_var->perrecs * 8;
                }
                int recs = curblk->size / ksctrl->mapbctrl_var->recsize;
                // 这是最简单的方法 效率不高 以后有需要再优化
                for (int i = 0; i < recs; i++)
                {
                    msec_t *curmsec = (msec_t *)(ptr + ksctrl->sdict->table->field_time->offset);
                    if (*curmsec >= msec->start) 
                    {
                        return i + agos;
                    }
                    ptr += ksctrl->mapbctrl_var->recsize;
                }
                agos += recs;
                next = sis_node_next(next);
            }
        }
    }
    return 0;
}

// 得到最后一块的数据头
s_sis_map_block *sis_map_ksctrl_last_head(s_sis_map_ksctrl *ksctrl)
{
    s_sis_map_bctrl *bctrl = ksctrl->mapbctrl_var;
    s_sis_node *next = sis_node_next(bctrl->nodes);
    while (next)
    {
        if (next->next == NULL)
        {
            return (s_sis_map_block *)(next->value);
        }
        next = sis_node_next(next);
    }
    return NULL;
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
    return subctrl;
}

void sis_map_subctrl_destroy(s_sis_map_subctrl *subctrl)
{
    sis_sdsfree(subctrl->sub_keys);
    sis_sdsfree(subctrl->sub_sdbs);
    sis_pointer_list_destroy(subctrl->subvars);
    sis_free(subctrl);
}
void sis_map_subinfo_set_fd(s_sis_map_subinfo *subinfo, s_sis_map_fctrl *fctrl)
{
    if (subinfo->nottfd == 0)
    {
        subinfo->timefd = sis_map_ksctrl_get_timefd(subinfo->ksctrl, subinfo->cursor);
    }
    if (fctrl->sub_mode == SIS_MAP_SUB_DATA)
    {
        subinfo->sortfd = subinfo->timefd;
    }
    else
    {
        subinfo->sortfd = sis_map_bctrl_get_sno(subinfo->ksctrl->mapbctrl_var, subinfo->cursor);
        printf(" getsno == : %p %lld\n", subinfo->sortfd, subinfo->sortfd? *subinfo->sortfd :0);
    }
}
int sis_map_subctrl_init(s_sis_map_subctrl *subctrl, s_sis_map_fctrl *fctrl)
{
    sis_pointer_list_clear(subctrl->subvars);
    s_sis_dict_entry *de;
    s_sis_dict_iter *di = sis_dict_get_iter(fctrl->map_kscs);
    while ((de = sis_dict_next(di)) != NULL)
    {
        s_sis_map_ksctrl *ksctrl = sis_dict_getval(de);
        if (subctrl->mpair.stop > 0 && ksctrl->mapbctrl_var->sumrecs < 1)
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
                subinfo->cursor = sis_map_ksctrl_find_start_cursor(ksctrl, &subctrl->mpair);
                // 没有记录 会返回 0 
                // 有记录 时间对不上 返回未初始化的记录号 subinfo->cursor >= bctrl->sumrecs
            }
            else
            {
                subinfo->nottfd = 1;
                subinfo->timefd = NULL;
            }
            sis_map_subinfo_set_fd(subinfo, fctrl);
            printf("== adda : %d %d %lld %d %s %s\n",subinfo->cursor, ksctrl->mapbctrl_var->sumrecs, subinfo->sortfd ? *subinfo->sortfd: -1, subinfo->ksctrl->mapbctrl_var->sumrecs, ksctrl->kdict->kname, ksctrl->sdict->table->name);
        }     
    }
    sis_dict_iter_free(di);  
    return 0;
}

int sis_map_subctrl_sub_sno(s_sis_map_subctrl *msubctrl, s_sis_map_fctrl *fctrl)
{
    s_sis_disk_reader_cb *callback = msubctrl->callback;
    int startdate = sis_msec_get_idate(msubctrl->mpair.start);

    printf("===3=== %d %d\n", startdate, msubctrl->subvars->count);
    // 设置 minpubsv 的值
    // 这里默认数据文件必须按时序写入 不处理增量非时序写入的数据
    int64 minpubsv = -1;
    int64 minpubtv = 0;
    for (int i = 0; i < msubctrl->subvars->count; i++)
    {
        s_sis_map_subinfo *subinfo = sis_pointer_list_get(msubctrl->subvars, i);
        if (subinfo->cursor >= subinfo->ksctrl->mapbctrl_var->sumrecs)
        {
            // 表示数据消耗完毕
            continue;
        }
        if (subinfo->sortfd == NULL)
        {
            // 无排序字段的去掉 不可能做到
            continue;
        }
        int64 cursno = *(subinfo->sortfd);
        if (minpubsv > cursno || minpubsv == -1)
        {
            minpubsv = cursno;
            if (subinfo->nottfd == 0 && subinfo->timefd)
            {
                minpubtv = *(subinfo->timefd);
            }
        }
    }
    // 开始循环处理时序数据
    int minsize = SIS_MAP_SUB_SIZE;
    minsize = SIS_MAXI(minsize, msubctrl->subvars->count * 3);
    s_sis_map_subsno *vars = sis_malloc(minsize * sizeof(s_sis_map_subsno));
    
    startdate = SIS_MAXI(startdate, sis_msec_get_idate(minpubtv));
    
    int workdate = 0;
    while (1)
    {
        memset((char *)vars, 0, minsize * sizeof(s_sis_map_subsno));
        int   sends = 0;
        int64 mindiffsv = 0;
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
            printf("===4.1=== %d %d %d\n", i, subinfo->cursor, subinfo->ksctrl->mapbctrl_var->sumrecs);
            if (subinfo->cursor >= subinfo->ksctrl->mapbctrl_var->sumrecs)
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
                printf("===4.2===%s.%s %d %lld %lld %lld\n", subinfo->ksctrl->kdict->kname, subinfo->ksctrl->sdict->table->name,
                    i, curmsec, *subinfo->sortfd, subinfo->timefd ? *subinfo->timefd : 0);
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
                }
                int64 cursno = *(subinfo->sortfd);
                if (cursno < minpubsv)
                {
                    // 序列号小的应该是程序出问题了
                    LOG(3)("sub error. %lld > %lld\n", minpubsv, cursno);
                    break;
                }
                int64 diffsv = cursno - minpubsv;
                if (diffsv >= minsize)
                {
                    // 跨度大 不处理
                    if (mindiffsv > diffsv || mindiffsv == -1)
                    {
                        mindiffsv = diffsv;
                    }
                    break;
                }
                printf("%d %d %d %d %p\n", diffsv, minsize, cursno, minpubsv);
                vars[diffsv].inmem = sis_map_bctrl_get_var(subinfo->ksctrl->mapbctrl_var, subinfo->cursor);
                vars[diffsv].idate = curmsec ? sis_msec_get_idate(curmsec) : startdate;
                vars[diffsv].ksctrl = subinfo->ksctrl;
                subinfo->cursor ++;
                sends ++;
                sis_map_subinfo_set_fd(subinfo, fctrl);
                printf("===4.3=== %d %d %d %lld %p\n", sends, diffsv, subinfo->cursor, subinfo->sortfd? *subinfo->sortfd :0, subinfo->sortfd);
                if (subinfo->cursor >= subinfo->ksctrl->mapbctrl_var->sumrecs)
                {
                    break;
                }
            }
        }
        printf("===5=== %d %d %d %d %d\n", sends, msubctrl->subvars->count, workdate, minpubsv, mindiffsv);
        if (sends == 0)
        {
            if (msubctrl->mpair.stop > 0)
            {
                if(callback->cb_stop)
                {
                    callback->cb_stop(callback->cb_source, workdate);
                }
                sis_free(vars);
                return 0;
            }
            else
            {
                sis_sleep(SIS_MAP_DELAY_MS);
            }
        }
        else
        {
            for (int i = 0; i < minsize && sends > 0; i++)
            {
                if (vars[i].ksctrl == 0)
                {
                    continue;
                }
                printf("========= sends = %d %d\n", i, sends);
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
        minpubsv += mindiffsv == -1 ? 0 : mindiffsv;
    }
    sis_free(vars);
    return 1;
}
int sis_map_subctrl_sub_data(s_sis_map_subctrl *msubctrl, s_sis_map_fctrl *fctrl)
{    
    s_sis_disk_reader_cb *callback = msubctrl->callback;
    int startdate = sis_msec_get_idate(msubctrl->mpair.start);

    // 设置 minpubsv 的值
    int64 minpubsv = 0;
    for (int i = 0; i < msubctrl->subvars->count; i++)
    {
        s_sis_map_subinfo *subinfo = sis_pointer_list_get(msubctrl->subvars, i);
        if (subinfo->cursor >= subinfo->ksctrl->mapbctrl_var->sumrecs)
        {
            // 表示数据消耗完毕
            continue;
        }
        if (subinfo->nottfd == 1 || subinfo->timefd == NULL)
        {
            // 无时间字段的去掉
            continue;
        }
        minpubsv = SIS_MINI(minpubsv, *(subinfo->timefd));
    }
    // 开始循环处理时序数据
    startdate = SIS_MAXI(startdate, sis_msec_get_idate(minpubsv));
    int workdate = 0;
    while (1)
    {
        int   sends = 0;
        int64 mindiffsv = 0;
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
            if (subinfo->cursor >= subinfo->ksctrl->mapbctrl_var->sumrecs)
            {
                // 表示数据消耗完毕
                continue;
            }
            if (subinfo->nottfd == 1)
            {
                // 先处理没有时间字段的
                while (subinfo->cursor < subinfo->ksctrl->mapbctrl_var->sumrecs)
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
                        char *var = sis_map_bctrl_get_var(subinfo->ksctrl->mapbctrl_var, subinfo->cursor);
                        callback->cb_chardata(callback->cb_source, subinfo->ksctrl->kdict->kname, subinfo->ksctrl->sdict->table->name, var, subinfo->ksctrl->sdict->table->size);
                        subinfo->cursor ++;
                        // sis_map_subinfo_set_fd(subinfo, fctrl);
                        sends ++;
                    }
                }
                continue;
            }
            if (subinfo->timefd == NULL)
            {
                // 表示有新数据来了
                sis_map_subinfo_set_fd(subinfo, fctrl);
            }
            while (1)
            {
                msec_t curmsec = *(subinfo->timefd);
                printf("===6.1=== %lld %lld %lld\n", subinfo->ksctrl->ksidx, curmsec, msubctrl->mpair.stop);
                if (msubctrl->mpair.stop > 0 && curmsec > msubctrl->mpair.stop)
                {
                    // 时间超过 不再处理 除非不限制日期
                    sis_pointer_list_delete(msubctrl->subvars, i, 1);
                    i--;
                    break;
                }
                int64 diffsv = curmsec - minpubsv;
                printf("===6.1=== %lld %lld \n", subinfo->ksctrl->ksidx, diffsv);
                if (diffsv > SIS_MAP_MIN_DIFF)
                {
                    // 时间跨度大 不处理
                    mindiffsv = SIS_MINI(mindiffsv, diffsv);
                    break;
                }
                if (diffsv < 0 && msubctrl->mpair.start > 0 && curmsec < msubctrl->mpair.start)
                {
                    // 数据中时序错误 不再处理 除非不限制日期
                    sis_pointer_list_delete(msubctrl->subvars, i, 1);
                    i--;
                    break;
                }
                printf("===6.2===%s.%s %d %lld  %d | %lld %lld\n", subinfo->ksctrl->kdict->kname, subinfo->ksctrl->sdict->table->name,
                    i, curmsec, diffsv, *subinfo->sortfd, subinfo->timefd ? *subinfo->timefd : 0);

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
                    char *var = sis_map_bctrl_get_var(subinfo->ksctrl->mapbctrl_var, subinfo->cursor);
                    callback->cb_chardata(callback->cb_source, subinfo->ksctrl->kdict->kname, subinfo->ksctrl->sdict->table->name, var, subinfo->ksctrl->sdict->table->size);
                    subinfo->cursor ++;
                    sends ++;
                    sis_map_subinfo_set_fd(subinfo, fctrl);
                }
                printf("===6.3=== %d %d\n", subinfo->cursor, subinfo->ksctrl->mapbctrl_var->sumrecs);
                if (subinfo->cursor >= subinfo->ksctrl->mapbctrl_var->sumrecs)
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
                    callback->cb_stop(callback->cb_source, workdate);
                }
                return 0;
            }
            else
            {
                sis_sleep(SIS_MAP_DELAY_MS);
            }
        }
        minpubsv += mindiffsv;
    }
    return 1;
}
