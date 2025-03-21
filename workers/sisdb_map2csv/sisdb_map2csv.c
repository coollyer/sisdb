﻿
#include "worker.h"
#include "server.h"

#include <sis_modules.h>
#include <sisdb_map2csv.h>
#include <sis_obj.h>
#include "sis_utils.h"
#include "sis_list.h"


///////////////////////////////////////////////////
// *** s_sis_modules sis_modules_[dir name]  *** //
///////////////////////////////////////////////////
s_sis_modules sis_modules_sisdb_map2csv = {
    sisdb_map2csv_init,
    NULL,
    sisdb_map2csv_working,
    NULL,
    sisdb_map2csv_uninit,
    NULL,
    NULL,
    0,
    NULL,
};

/////////////////////////////////
//
/////////////////////////////////
bool sisdb_map2csv_init(void *worker_, void *node_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sis_json_node *node = (s_sis_json_node *)node_;
    s_sisdb_map2csv_cxt *context = SIS_MALLOC(s_sisdb_map2csv_cxt, context);
    worker->context = context;

    {
        s_sis_json_node *innode = sis_json_cmp_child_node(node, "source");
        if (!innode)
        {
            return false;
        }
        context->source = sis_worker_create(worker, innode);
        if (!context->source)
        {
            LOG(5)("create method fail. no source.\n");
            return false;
        }
    }
    {
        s_sis_json_node *innode = sis_json_cmp_child_node(node, "work-date");
        if (innode)
        {
            context->work_date.start = sis_json_get_int(innode, "0", 0);
            context->work_date.stop  = sis_json_get_int(innode, "1", 0);
        }
        else
        {
            context->work_date.start = 0;
            context->work_date.stop = 0;
        }
    }
    {
        context->sub_keys = sis_sdsempty();
        const char *str = sis_json_get_str(node, "sub-keys");
        if (str)
        {
            context->sub_keys = sis_sdscat(context->sub_keys, str);
        }
        else
        {
            context->sub_keys = sis_sdscat(context->sub_keys, "*");
        }
    }
    {
        context->sub_sdbs = sis_sdsempty();
        const char *str = sis_json_get_str(node, "sub-sdbs");
        if (str)
        {
            context->sub_sdbs = sis_sdscat(context->sub_sdbs, str);
        }
        else
        {
            context->sub_sdbs = sis_sdscat(context->sub_sdbs, "*");
        }
    }
    const char *wpath = sis_json_get_str(node, "work-path");
    if (wpath)
    {
        context->wpath = sis_sdsnew(wpath);
    }
    else
    {
        context->wpath = sis_sdsnew(".");
    }
    const char *wname = sis_json_get_str(node, "work-name");
    if (wname)
    {
        context->wname = sis_sdsnew(wname);
    }
    else
    {
        context->wname = sis_sdsnew("sno");
    }

    context->catch_size = sis_json_get_int(node, "catch_size", 0);
    if (context->catch_size > 0)
    {
        context->catch_size *= 1024;
    }
    // == 0 表示来一条数据就写一条
    context->map_sdbs = sis_map_list_create(sis_sisdb_map2csv_dbinfo_destroy);

    context->status = SIS_MAP2CSV_INIT;

    return true;
}

void sisdb_map2csv_uninit(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_map2csv_cxt *context = (s_sisdb_map2csv_cxt *)worker->context;

    if (context->source)
    {
        sis_worker_destroy(context->source);
    }  
    sis_sdsfree(context->sub_keys);
    sis_sdsfree(context->sub_sdbs);
    sis_sdsfree(context->wpath);
    sis_sdsfree(context->wname);
    
    sis_map_list_destroy(context->map_sdbs);

    sis_free(context);
    worker->context = NULL;
}


/////////////////////////////////
//
/////////////////////////////////

s_sisdb_map2csv_dbinfo *sis_sisdb_map2csv_dbinfo_create(s_sis_dynamic_db *db, size_t csize)
{
    s_sisdb_map2csv_dbinfo *dinfo =  SIS_MALLOC(s_sisdb_map2csv_dbinfo, dinfo);
    dinfo->db = db;
    s_sis_sds chead = sis_sdbinfo_to_csv_sds(db);
    dinfo->dbhead = sis_sdsempty();
    dinfo->dbhead = sis_sdscatfmt(dinfo->dbhead, "kname,%S\r\n", chead);
    sis_sdsfree(chead);
    dinfo->csize = csize;
    if (csize > 0)
    {
        dinfo->wcatch = sis_memory_create_size(csize);
    }
    dinfo->status = SIS_DBINFO_INIT;
    return dinfo;
}

void _sisdb_map2csv_dbinfo_write(s_sisdb_map2csv_dbinfo *dinfo, const char *inmem, size_t isize)
{
    if (dinfo->csvfp >= 0)
    {
        sis_file_write(dinfo->csvfp, inmem, isize);
    }
}
void _sisdb_map2csv_dbinfo_close(s_sisdb_map2csv_dbinfo *dinfo)
{
    if (dinfo->csvfp)
    {
        if (dinfo->wcatch)
        {
            size_t size = sis_memory_get_size(dinfo->wcatch);
            if (size > 0)
            {
                _sisdb_map2csv_dbinfo_write(dinfo, sis_memory(dinfo->wcatch), size);
                sis_memory_clear(dinfo->wcatch);
            }
        }
        sis_file_fsync(dinfo->csvfp);
        sis_file_close(dinfo->csvfp);
        dinfo->csvfp = NULL;
    }
}
void sis_sisdb_map2csv_dbinfo_destroy(void *info_)
{
    s_sisdb_map2csv_dbinfo *dinfo = info_;
    sis_dynamic_db_destroy(dinfo->db);
    sis_sdsfree(dinfo->dbhead);
    sis_sdsfree(dinfo->csvname);
    _sisdb_map2csv_dbinfo_close(dinfo);
    if (dinfo->wcatch)
    {
        sis_memory_destroy(dinfo->wcatch);
    }
    dinfo->status = SIS_DBINFO_INIT;
    sis_free(dinfo);
}
int sis_sisdb_map2csv_dbinfo_open(s_sisdb_map2csv_dbinfo *dinfo, int wdate)
{
    // LOG(0)("write--- %s\n", dinfo->csvname);
    if (dinfo->status != SIS_DBINFO_INIT)
    {
        return dinfo->status;
    }
    
    _sisdb_map2csv_dbinfo_close(dinfo);
    sis_check_path(dinfo->csvname);
    dinfo->csvfp = sis_file_open(dinfo->csvname, SIS_FILE_IO_READ | SIS_FILE_IO_WRITE | SIS_FILE_IO_CREATE|SIS_FILE_IO_APPEND, 0);
    if (!dinfo->csvfp)
    {
        LOG(8)("write csv fail. %s\n", dinfo->csvname);
    } 
    else
    {
        sis_file_seek(dinfo->csvfp, 0, SEEK_SET);
        if (!dinfo->wheaded)
        {
            _sisdb_map2csv_dbinfo_write(dinfo, dinfo->dbhead, sis_sdslen(dinfo->dbhead));
            dinfo->wheaded = 1;
        }
        dinfo->status = SIS_DBINFO_WORK;
    }   
    return dinfo->status;
}

void sis_sisdb_map2csv_dbinfo_stop(s_sisdb_map2csv_dbinfo *dinfo)
{
    if (dinfo->status != SIS_DBINFO_WORK)
    {
        return ;
    }
    _sisdb_map2csv_dbinfo_close(dinfo);
    dinfo->status = SIS_DBINFO_INIT;
}

s_sis_sds _sisdb_map2csv_to_csv_sds(s_sis_dynamic_db *db_, const char *key, void *in_, size_t ilen_)
{
	const char *val = (const char *)in_;
	s_sis_sds o = sis_sdsempty();
    o = sis_sdscatfmt(o, "%s", key);
	int fnums = sis_map_list_getsize(db_->fields);
	// sis_out_binary(":::", val, ilen_);
    for (int i = 0; i < fnums; i++)
    {
        s_sis_dynamic_field *f = (s_sis_dynamic_field *)sis_map_list_geti(db_->fields, i);
        o = sis_dynamic_field_to_msec_csv(o, f, val);
    }
    o = sis_csv_make_end(o);
	return o;
}

int sis_sisdb_map2csv_dbinfo_write(s_sisdb_map2csv_dbinfo *dinfo, const char *key, void *inmem, size_t isize)
{
    if (dinfo->status != SIS_DBINFO_WORK)
    {
        return 0;
    }
    s_sis_sds cdata = _sisdb_map2csv_to_csv_sds(dinfo->db, key, inmem, isize);
    int size = sis_sdslen(cdata);
    if (dinfo->wcatch)
    {
        if (sis_memory_get_size(dinfo->wcatch) + size > dinfo->csize)
        {
            _sisdb_map2csv_dbinfo_write(dinfo, sis_memory(dinfo->wcatch), sis_memory_get_size(dinfo->wcatch));
            sis_memory_clear(dinfo->wcatch);
        }
        sis_memory_cat(dinfo->wcatch, cdata, size);
    }
    else
    {
        _sisdb_map2csv_dbinfo_write(dinfo, cdata, size);
    }
    // printf("%s %d %d\n", dinfo->db->name, size, (int)sis_memory_get_size(dinfo->wcatch));
    sis_sdsfree(cdata);
    return size;
}

///////////////////////////////////////////
//  callback define begin
///////////////////////////////////////////
static int cb_sub_open(void *worker_, void *argv_)
{
    return SIS_METHOD_OK;
}
static int cb_sub_close(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_map2csv_cxt *context = (s_sisdb_map2csv_cxt *)worker->context;
    context->status = SIS_MAP2CSV_EXIT;
    context->work_date.move = context->work_date.stop;
    return SIS_METHOD_OK;
}
static int cb_sub_start(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_map2csv_cxt *context = (s_sisdb_map2csv_cxt *)worker->context;

    context->status = SIS_MAP2CSV_WORK;
    context->curr_date = sis_atoll((char *)argv_);
    return SIS_METHOD_OK;
}
static int cb_dict_keys(void *worker_, void *argv_)
{
    return SIS_METHOD_OK;
}

s_sis_sds sisdb_map2csv_get_fname(const char *work_path, const char *work_name,  
				const char *head_name, const char *name_ext)
{
	s_sis_sds fpath = sis_sdsempty();
	fpath = sis_sdscatfmt(fpath, "%s/%s/%s.%s", work_path, work_name, head_name, name_ext);
	return fpath;
}
static int cb_dict_sdbs(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_map2csv_cxt *context = (s_sisdb_map2csv_cxt *)worker->context;
    if (argv_)
    {
        
        s_sis_json_handle *injson = sis_json_load(argv_, sis_sdslen(argv_));
        if (!injson)
        {
            return 0;
        }
        s_sis_json_node *innode = sis_json_first_node(injson->node);
        while (innode)
        {
            s_sis_dynamic_db *db = sis_dynamic_db_create(innode);
            if (db)
            {
                s_sisdb_map2csv_dbinfo *dinfo = sis_sisdb_map2csv_dbinfo_create(db, context->catch_size);
                // dinfo->csvname = sisdb_sno2csv_get_fname(context->wpath, context->wname, db->name, "csv", context->curr_date);
                dinfo->csvname = sisdb_map2csv_get_fname(context->wpath, context->wname, db->name, "csv");
                sis_map_list_set(context->map_sdbs, db->name, dinfo);
            }
            innode = sis_json_next_node(innode);
        }
        sis_json_close(injson);
    }
    return SIS_METHOD_OK;
}
static int cb_sub_stop(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_map2csv_cxt *context = (s_sisdb_map2csv_cxt *)worker->context;
    // LOG(0)("stop : %d\n", context->work_status);
    if (context->status == SIS_MAP2CSV_WORK)
    {
        // 结束写文件
        // 清理缓存等
        int count = sis_map_list_getsize(context->map_sdbs);
        for (int i = 0; i < count; i++)
        {
            s_sisdb_map2csv_dbinfo *dinfo = sis_map_list_geti(context->map_sdbs, i);
            sis_sisdb_map2csv_dbinfo_stop(dinfo);
        }
    }
    int subdate = sis_atoll((char *)argv_);
    context->work_date.move = subdate;
    context->status = SIS_MAP2CSV_INIT;
    context->curr_date = 0;
    return SIS_METHOD_OK;
}

static int cb_sub_chars(void *worker_, void *argv)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_map2csv_cxt *context = (s_sisdb_map2csv_cxt *)worker->context;
    
    s_sis_db_chars *pchars = (s_sis_db_chars *)argv;

    s_sisdb_map2csv_dbinfo *dinfo = sis_map_list_get(context->map_sdbs, pchars->sname);
    // LOG(0)("%p %s\n", dinfo, pchars->sname);
    if (dinfo)
    {
        sis_sisdb_map2csv_dbinfo_open(dinfo, context->curr_date);
        sis_sisdb_map2csv_dbinfo_write(dinfo, pchars->kname, pchars->data, pchars->size);
    }
    return SIS_METHOD_OK;
}

///////////////////////////////////////////
//  callback define end.
///////////////////////////////////////////


void _sisdb_map2csv_start_read(s_sis_worker *worker, int start, int stop)
{
    s_sisdb_map2csv_cxt *context = (s_sisdb_map2csv_cxt *)worker->context;
    s_sis_message *msg = sis_message_create();
    sis_message_set(msg, "cb_source", worker, NULL);
    sis_message_set_str(msg, "sub-keys", context->sub_keys, sis_sdslen(context->sub_keys));
    sis_message_set_str(msg, "sub-sdbs", context->sub_sdbs, sis_sdslen(context->sub_sdbs));
    sis_message_set_method(msg, "cb_sub_open",  cb_sub_open);
    sis_message_set_method(msg, "cb_sub_close", cb_sub_close);
    sis_message_set_method(msg, "cb_sub_start", cb_sub_start);
    sis_message_set_method(msg, "cb_sub_stop",  cb_sub_stop);
    sis_message_set_method(msg, "cb_dict_keys", cb_dict_keys);
    sis_message_set_method(msg, "cb_dict_sdbs", cb_dict_sdbs);
    sis_message_set_method(msg, "cb_sub_chars", cb_sub_chars);     
    sis_message_set_int(msg, "start-date", start);
    sis_message_set_int(msg, "stop-date", stop);
    sis_worker_command(context->source, "sub", msg);
    sis_message_destroy(msg);
}


///////////////////////////////////////////
//  method define
/////////////////////////////////////////

void sisdb_map2csv_working(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_map2csv_cxt *context = (s_sisdb_map2csv_cxt *)worker->context;
    msec_t smsec = sis_time_get_now_msec();
    LOG(5)("sub history start. [%d]\n", context->work_date.start);
    _sisdb_map2csv_start_read(worker, context->work_date.start, context->work_date.stop);
    SIS_WAIT_OR_EXIT(context->status == SIS_MAP2CSV_EXIT || context->work_date.move == context->work_date.stop);
    LOG(5)("sub history end. [%d] cost : %d\n", context->work_date.stop, (int)(sis_time_get_now_msec() - smsec));
}
