
#include "sis_disk.io.map.h"

// // 这里是关于sno的读写函数
// ///////////////////////////////////////////////////////
// // s_sis_disk_sno_rctrl
// ///////////////////////////////////////////////////////

s_sis_map_fctrl *sis_map_fctrl_create()
{
    return NULL;
}
void sis_map_fctrl_destroy(void *fctrl_)
{

}

int sis_disk_io_map_w_open(s_sis_map_fctrl *fctrl, const char *fpath_, const char *fname_, int wdate_)
{
    // s_sis_disk_ctrl *o = SIS_MALLOC(s_sis_disk_ctrl, o);
    //     o->fpath = sis_sdsnew(fpath_);
    // o->fname = sis_sdsnew(fname_);
    // o->style = style_;
    // o->open_date = wdate_ == 0 ? sis_time_get_idate(0) : wdate_;
    // o->stop_date = o->open_date;
    // o->status = SIS_DISK_STATUS_CLOSED;    
    // o->isstop  = false;

    // o->map_kdicts = sis_map_list_create(sis_disk_kdict_destroy);
    // o->map_sdicts = sis_map_list_create(sis_disk_sdict_destroy);
    
    // o->new_kinfos = sis_pointer_list_create();
    // o->new_kinfos->vfree = sis_object_destroy;
    // o->new_sinfos = sis_pointer_list_create();
    // o->new_sinfos->vfree = sis_dynamic_db_destroy;

    // o->wcatch = sis_disk_wcatch_create(NULL, NULL);
    // o->rcatch = sis_disk_rcatch_create(NULL);

    // o->work_fps = sis_disk_files_create();

    // o->widx_fps = sis_disk_files_create();
    // o->map_idxs = sis_map_list_create(sis_disk_idx_destroy);

    // o->sdb_incrzip = sis_incrzip_class_create();

    // o->map_maps = sis_map_list_create(sis_disk_map_destroy);
    
    // if (o->open_date > 0)
    // {
    //     sis_sprintf(work_fn, 1024, "%s/%s.%d.%s",
    //         o->fpath, o->fname, o->open_date,,SIS_DISK_MAP_CHAR);
    // }
    // else
    // {
    //     sis_sprintf(work_fn, 1024, "%s/%s.%s",
    //         o->fpath, o->fname, SIS_DISK_MAP_CHAR);
    // }
    // o->work_fps->max_page_size = SIS_DISK_MAXLEN_MAPPAGE;
    // o->work_fps->max_file_size = 0;
    // o->work_fps->main_head.zip = SIS_DISK_ZIP_NOZIP;
    // o->work_fps->main_head.index = 0;

}

// s_sis_disk_kdict *sis_disk_io_map_incr_kdict(s_sis_disk_ctrl *cls_, const char *kname_);

// 开始 此时 key 和 sdb 已经有基础数据了
int sis_disk_io_map_w_start(s_sis_map_fctrl *fctrl)
{
    return 0;
}

// 这两个填补key和sdb可以增补 并可能扩展大小
int sis_disk_io_map_set_kinfo(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_)
{
    // s_sis_string_list *klist = sis_string_list_create();
    // sis_string_list_load(klist, in_, ilen_, ",");
    // int count = sis_string_list_getsize(klist);
    // for (int i = 0; i < count; i++)
    // {
    //     const char *key = sis_string_list_get(klist, i);
    //     sis_disk_ctrl_set_kdict(writer_->munit, key);
    //     sis_disk_writer_kdict_changed(writer_, key);
    // }
    // sis_string_list_destroy(klist);
    // // 写盘
    // {
    //     sis_disk_ctrl_write_kdict(writer_->munit);
    //     int count = sis_map_list_getsize(writer_->units);
    //     for (int i = 0; i < count; i++)
    //     {
    //         s_sis_disk_ctrl *unit = (s_sis_disk_ctrl *)sis_map_list_geti(writer_->units, i);
    //         sis_disk_ctrl_write_kdict(unit);
    //     }
    // }
    // return  sis_map_list_getsize(writer_->munit->map_kdicts);  
    return 0;
}
int sis_disk_io_map_set_sinfo(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_)
{
    //     s_sis_json_handle *injson = sis_json_load(in_, ilen_);
    // if (!injson)
    // {
    //     return 0;
    // }
    // s_sis_json_node *innode = sis_json_first_node(injson->node); 
    // while (innode)
    // {
    //     s_sis_dynamic_db *newdb = sis_dynamic_db_create(innode);
    //     if (newdb)
    //     {
    //         sis_disk_ctrl_set_sdict(writer_->munit, newdb);
    //         sis_disk_writer_sdict_changed(writer_, newdb);
    //         sis_dynamic_db_decr(newdb);
    //     }  
    //     innode = sis_json_next_node(innode);
    // }
    // sis_json_close(injson);
    // {
    //     sis_disk_ctrl_write_sdict(writer_->munit);
    //     int count = sis_map_list_getsize(writer_->units);
    //     for (int i = 0; i < count; i++)
    //     {
    //         s_sis_disk_ctrl *unit = (s_sis_disk_ctrl *)sis_map_list_geti(writer_->units, i);
    //         sis_disk_ctrl_write_sdict(unit);
    //     }
    // }
    // return  sis_map_list_getsize(writer_->munit->map_sdicts);   
    return 0;
}
s_sis_dynamic_db *sis_disk_io_map_get_sinfo(s_sis_map_fctrl *fctrl, const char *sname_)
{
    return 0;
}

int sis_disk_io_map_w_stop(s_sis_map_fctrl *fctrl)
{
    return 0;
}

int sis_disk_io_map_w_close(s_sis_map_fctrl *fctrl)
{
    return 0;
}

int sis_disk_io_map_control_remove(const char *path_, const char *name_, int idate_)
{
    return 0;
}

int sis_disk_io_map_w_data(s_sis_map_fctrl *cls_, const char *kname_, const char *sname_, void *in_, size_t ilen_)
{
    //  s_sis_disk_sdict *sdict = sis_disk_map_get_sdict(writer_->map_fctrl, sname_);
    // if (!sdict)
    // {
    //     return 0;
    // }
    // s_sis_disk_kdict *kdict = sis_disk_map_get_kdict(writer_->munit->map_kdicts, kname_);
    // if (!kdict)
    // {
    //     LOG(8)("new key: %s\n", kname_);
    //     kdict = sis_disk_ctrl_set_kdict(writer_->munit, kname_);
    //     sis_disk_io_map_incr_kdict(writer_->munit, kname_);
    // }
    // w 
}

void is_disk_io_map_r_unsub(s_sis_map_fctrl *fctrl)
{
}
int sis_disk_io_map_r_sub(s_sis_map_fctrl *fctrl, const char *keys_, const char *sdbs_, int startdate_, int stopdate_)
{
    return 0;
}
// {
//     int o = _disk_reader_open(reader_, SIS_DISK_TYPE_SNO, idate_);
//     if (o)
//     {
//         // 通知订阅者读取或订阅结束
//         if (reader_->callback->cb_stop)
//         {
//             reader_->callback->cb_stop(reader_->callback->cb_source, idate_);
//         }
//         LOG(5)("no open %s. %d\n", reader_->fname, o);
//         return o;
//     }
//     reader_->status_sub = 1;
//     // 按顺序输出 keys_ sdbs_ = NULL 实际表示 *
//     sis_disk_io_sub_sno(reader_->munit, keys_, sdbs_, NULL, reader_->callback);
//     // 订阅结束
//     reader_->status_sub = 0;

//     _disk_reader_close(reader_);
//     return 0;
// }
s_sis_object *sis_disk_io_map_r_get_obj(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, s_sis_msec_pair *smsec_)
{
    return NULL;
}

// s_sis_disk_kdict *sis_disk_io_map_incr_key(s_sis_disk_ctrl *cls_, const char *kname_)
// {
//     s_sis_disk_kdict *kdict = sis_disk_map_get_kdict(cls_->map_kdicts, kname_);
//     if (!kdict)
//     {
//         kdict = sis_disk_kdict_create(kname_);
//         kdict->index = sis_map_list_set(cls_->map_kdicts, kname_, kdict);
//         sis_object_incr(kdict->name);
//         sis_pointer_list_push(cls_->new_kinfos, kdict->name);
//     }
//     return kdict;
// }
// int sis_disk_io_write_map(s_sis_disk_ctrl *cls_, s_sis_disk_kdict *kdict_, s_sis_disk_sdict *sdict_, void *in_, size_t ilen_)
// {
//     // s_sis_dynamic_db *sdb = sis_disk_sdict_last(sdict_);
//     // if (!sdb || (ilen_ % sdb->size) != 0)
//     // {
//     //     return 0;
//     // }
//     // char snoname[255];
//     // sis_sprintf(snoname, 255, "%s.%s", SIS_OBJ_SDS(kdict_->name), SIS_OBJ_SDS(sdict_->name));        
//     // s_sis_disk_wcatch *wcatch = (s_sis_disk_wcatch *)sis_map_list_get(cls_->sno_wcatch, snoname);
//     // if (!wcatch)
//     // {
//     //     wcatch = sis_disk_wcatch_create(kdict_->name, sdict_->name);
//     //     sis_map_list_set(cls_->sno_wcatch, snoname, wcatch);
//     // }
//     // if (sis_memory_get_size(wcatch->memory) < 1)
//     // {
//     //     cls_->sno_size += sis_memory_cat_ssize(wcatch->memory, kdict_->index);
//     //     cls_->sno_size += sis_memory_cat_ssize(wcatch->memory, sdict_->index);
//     //     wcatch->winfo.start = sis_dynamic_db_get_time(sdb, 0, in_, ilen_);
//     // }
//     // wcatch->winfo.sdict = sdict_->sdbs->count;
//     // wcatch->winfo.ipage = cls_->sno_pages;

//     // int count = ilen_ / sdb->size;
//     // cls_->sno_count += count;
//     // for (int i = 0; i < count; i++)
//     // {
//     //     cls_->sno_size += sis_memory_cat_ssize(wcatch->memory, cls_->sno_series);
//     //     cls_->sno_series++;
//     //     cls_->sno_size += sis_memory_cat(wcatch->memory, (char *)in_ + i * sdb->size, sdb->size);
//     // }
//     // _set_disk_io_sno_msec(cls_, sdb, count - 1, in_, ilen_);
//     // // if (cls_->sno_series % 100000 == 1)
//     // // printf("%zu %s %lld %d %d--> %zu\n", cls_->sno_size , snoname, cls_->sno_count, count, cls_->sno_series, cls_->work_fps->max_page_size);
//     // if (cls_->sno_size > cls_->work_fps->max_page_size)
//     // {
//     //     // printf("%zu %zu %zu %d\n", cls_->sno_size, cls_->work_fps->max_page_size, cls_->sno_series, cls_->sno_pages);
//     //     _disk_io_write_sno_watch(cls_);
//     // }
//     return 0;
// }

// int sis_disk_io_write_map_stop(s_sis_disk_ctrl *cls_)
// {
//     // _disk_io_write_sno_watch(cls_);
//     // sis_map_list_destroy(cls_->sno_wcatch);
//     // cls_->sno_wcatch = NULL;
//     return 0;
// }

// // 关闭所有文件 重写索引
// void sis_disk_writer_close(s_sis_disk_writer *writer_)
// {
//     if (writer_->status && writer_->munit)
//     {
//         if (writer_->style == SIS_DISK_TYPE_SDB)
//         {
//             // printf("%d %d\n", writer_->munit->new_kinfos->count, writer_->munit->new_sinfos->count);
//             sis_disk_ctrl_write_kdict(writer_->munit);
//             sis_disk_ctrl_write_sdict(writer_->munit);
//         }
//         sis_disk_ctrl_write_stop(writer_->munit);
//         int count = sis_map_list_getsize(writer_->units);
//         for (int i = 0; i < count; i++)
//         {
//             s_sis_disk_ctrl *ctrl = sis_map_list_geti(writer_->units, i);
//             sis_disk_ctrl_write_stop(ctrl);
//         }
//     }
//     writer_->status = 0;
// }
