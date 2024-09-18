
#include "worker.h"

#include <sis_modules.h>
#include <sisdb_wmap.h>
#include "sis_utils.h"

// 从行情流文件中获取数据源 需要支持直接写压缩数据然后解压写入

static s_sis_method _sisdb_wmap_methods[] = {
  {"getcb",  cmd_sisdb_wmap_getcb, 0, NULL},
};
///////////////////////////////////////////////////
// *** s_sis_modules sis_modules_[dir name]  *** //
///////////////////////////////////////////////////
s_sis_modules sis_modules_sisdb_wmap = {
    sisdb_wmap_init,
    NULL,
    NULL,
    NULL,
    sisdb_wmap_uninit,
    NULL,
    NULL,
    sizeof(_sisdb_wmap_methods)/sizeof(s_sis_method),
    _sisdb_wmap_methods,
};

bool sisdb_wmap_init(void *worker_, void *node_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sis_json_node *node = (s_sis_json_node *)node_;

    s_sisdb_wmap_cxt *context = SIS_MALLOC(s_sisdb_wmap_cxt, context);
    worker->context = context;
    {
        context->work_path = sis_sds_save_create(sis_json_get_str(node, "work-path"), "data");   
        context->work_name = sis_sds_save_create(sis_json_get_str(node, "work-name"), "mapdb");    
        context->style = sis_json_get_int(node, "work-mode", 0) == 0 ? SIS_DISK_TYPE_MSN : SIS_DISK_TYPE_MAP; 
    }     
    context->work_sdbs = sis_sdsnew("*");
    context->work_keys = sis_sdsnew("*");
    return true;
}

void sisdb_wmap_uninit(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;

    context->status = SIS_WMAP_EXIT;
    sisdb_wmap_stop(context);

    sis_sds_save_destroy(context->work_path);
    sis_sds_save_destroy(context->work_name);
    sis_sdsfree(context->work_keys);
    sis_sdsfree(context->work_sdbs);

    sis_sdsfree(context->wmap_keys);
    sis_sdsfree(context->wmap_sdbs);
    sis_free(context);
    worker->context = NULL;
}
///////////////////////////////////////////
//  callback define begin
///////////////////////////////////////////


void sisdb_wmap_stop(s_sisdb_wmap_cxt *context)
{
    if (context->writer)
    {
        sis_disk_writer_close(context->writer);
        sis_disk_writer_destroy(context->writer);
        context->writer = NULL;
    }
}
bool sisdb_wmap_start(s_sisdb_wmap_cxt *context)
{
    sis_sdsfree(context->wmap_keys); context->wmap_keys = NULL;
    sis_sdsfree(context->wmap_sdbs); context->wmap_sdbs = NULL;
    
    if (context->wmode)
    {
        sis_disk_control_remove(sis_sds_save_get(context->work_path), sis_sds_save_get(context->work_name), 
            context->style, context->work_date > 0 ? context->work_date : -1);
    }
        
    context->writer = sis_disk_writer_create(
        sis_sds_save_get(context->work_path), 
        sis_sds_save_get(context->work_name), 
        context->style);
    if (sis_disk_writer_open(context->writer, 0) == 0)
    {
        LOG(5)("open wmap fail.\n");
        return false;
    }
    return true;
}

static msec_t _wmap_msec = 0;

static int cb_sub_open(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    
    if (!argv_)
    {
        context->work_date = sis_time_get_idate(0);
    }
    else
    {
        context->work_date = sis_atoll((char *)argv_);
    }
    if (!context->writer)
    {
        if(!sisdb_wmap_start(context))
        {
            context->status = SIS_WMAP_FAIL;
        }
        else
        {
            context->status = SIS_WMAP_OPEN;
        }
        LOG(5)("wmap start. %d status : %d\n", context->work_date, context->status);
        _wmap_msec = sis_time_get_now_msec();
    }
    return SIS_METHOD_OK;
}
static int cb_sub_close(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    if (context->status == SIS_WMAP_FAIL)
    {
        context->status = SIS_WMAP_INIT;
        return SIS_METHOD_ERROR;
    }
    LOG(5)("wmap stop cost = %lld\n", sis_time_get_now_msec() - _wmap_msec);
    sisdb_wmap_stop(context);
    LOG(5)("wmap stop cost = %lld\n", sis_time_get_now_msec() - _wmap_msec);

    sis_sdsfree(context->wmap_keys); context->wmap_keys = NULL;
    sis_sdsfree(context->wmap_sdbs); context->wmap_sdbs = NULL;

    context->status = SIS_WMAP_INIT;
    return SIS_METHOD_OK;
}
static int cb_sub_start(void *worker_, void *argv_)
{
	// s_sis_worker *worker = (s_sis_worker *)worker_; 
    // s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context; 
    return SIS_METHOD_OK;
}
static int cb_sub_stop(void *worker_, void *argv_)
{
	// s_sis_worker *worker = (s_sis_worker *)worker_; 
    // s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    return SIS_METHOD_OK;
}
static int cb_dict_keys(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    if (context->status == SIS_WMAP_OPEN)
    {
        sis_sdsfree(context->wmap_keys);
        context->wmap_keys = sis_sdsdup((s_sis_sds)argv_); 
    }   
    return SIS_METHOD_OK;
}
static int cb_dict_sdbs(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    if (context->status == SIS_WMAP_OPEN)
    {
        sis_sdsfree(context->wmap_sdbs);
        context->wmap_sdbs = sis_sdsdup((s_sis_sds)argv_);
    }
    return SIS_METHOD_OK;
}

static int cb_sub_chars(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    if (context->status == SIS_WMAP_FAIL)
    {
        return SIS_METHOD_ERROR;
    }
    if (context->status == SIS_WMAP_OPEN)
    {
        sis_disk_writer_inited(context->writer, 
            context->wmap_keys, sis_sdslen(context->wmap_keys),
            context->wmap_sdbs, sis_sdslen(context->wmap_sdbs));
        context->status = SIS_WMAP_HEAD;
    }

    s_sis_db_chars *inmem = (s_sis_db_chars *)argv_;
    sis_disk_writer_data(context->writer, inmem->kname, inmem->sname, inmem->data, inmem->size);
    // if (!sis_strcasecmp(inmem->kname, "SH600745"))
    // {
    //     printf("%s %zu\n", __func__, inmem->size);
    // }
	return SIS_METHOD_OK;
}

///////////////////////////////////////////
//  callback define end.
///////////////////////////////////////////

///////////////////////////////////////////
//  method define
/////////////////////////////////////////

int cmd_sisdb_wmap_getcb(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    if (context->status != SIS_WMAP_NONE && context->status != SIS_WMAP_INIT)
    {
        return SIS_METHOD_ERROR;
    }
    s_sis_message *msg = (s_sis_message *)argv_;
    
    sis_sds_save_set(context->work_path, sis_message_get_str(msg, "work-path"));
    sis_sds_save_set(context->work_name, sis_message_get_str(msg, "work-name"));
    
    context->wmode = sis_message_get_int(msg, "overwrite");

    sis_message_set(msg, "cb_source", worker, NULL);
    sis_message_set_method(msg, "cb_sub_open"    ,cb_sub_open);
    sis_message_set_method(msg, "cb_sub_close"   ,cb_sub_close);
    sis_message_set_method(msg, "cb_sub_start"   ,cb_sub_start);
    sis_message_set_method(msg, "cb_sub_stop"    ,cb_sub_stop);
    sis_message_set_method(msg, "cb_dict_sdbs"   ,cb_dict_sdbs);
    sis_message_set_method(msg, "cb_dict_keys"   ,cb_dict_keys);
    sis_message_set_method(msg, "cb_sub_chars"   ,cb_sub_chars);

    context->status = SIS_WMAP_INIT;
    return SIS_METHOD_OK; 
}
