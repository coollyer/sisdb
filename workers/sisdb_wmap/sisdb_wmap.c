
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
        context->work_type = sis_json_get_int(node, "work-type", 0) == 0 ? SIS_DISK_TYPE_MDB : SIS_DISK_TYPE_MSN; 
        context->covermode = sis_json_get_int(node, "covermode", 0);
        context->overwrite = sis_json_get_int(node, "overwrite", 0); 
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
    
    if (context->overwrite)
    {
        sis_disk_control_remove(sis_sds_save_get(context->work_path), sis_sds_save_get(context->work_name), 
            context->work_type, -1);
    }
        
    context->writer = sis_disk_writer_create(
        sis_sds_save_get(context->work_path), 
        sis_sds_save_get(context->work_name), 
        context->work_type);
    if (sis_disk_writer_open(context->writer, 0) == 0)
    {
        LOG(5)("open wmap fail.\n");
        return false;
    }
    else
    {
        LOG(5)("open wmap ok. %s %s\n", sis_sds_save_get(context->work_path), sis_sds_save_get(context->work_name));
    }
    return true;
}

static msec_t _wmap_msec = 0;

static int cb_sub_open(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    context->open_date = 0;
    if (argv_)
    {
        context->open_date = sis_atoll((char *)argv_);
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
        LOG(5)("wmap open. %d status : %d\n", context->open_date, context->status);
        _wmap_msec = sis_time_get_now_msec();
    }
    return SIS_METHOD_OK;
}
static int cb_sub_close(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    context->stop_date = 0;
    if (argv_)
    {
        context->stop_date = sis_atoll((char *)argv_);
    }
    if (context->status == SIS_WMAP_FAIL)
    {
        context->status = SIS_WMAP_INIT;
        return SIS_METHOD_ERROR;
    }
    LOG(5)("wmap close cost = %lld\n", sis_time_get_now_msec() - _wmap_msec);
    sisdb_wmap_stop(context);
    LOG(5)("wmap close cost = %lld\n", sis_time_get_now_msec() - _wmap_msec);

    sis_sdsfree(context->wmap_keys); context->wmap_keys = NULL;
    sis_sdsfree(context->wmap_sdbs); context->wmap_sdbs = NULL;

    context->status = SIS_WMAP_INIT;
    return SIS_METHOD_OK;
}
static int cb_sub_start(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context; 
    context->curr_date = sis_atoll((char *)argv_);
    return SIS_METHOD_OK;
}
static int cb_sub_stop(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_wmap_cxt *context = (s_sisdb_wmap_cxt *)worker->context;
    context->curr_date = 0;
    sis_disk_writer_sync(context->writer);
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
        // printf("status = %d %s %s\n", context->status, context->wmap_keys, context->wmap_sdbs);
        sis_disk_writer_inited(context->writer, 
            context->wmap_keys, sis_sdslen(context->wmap_keys),
            context->wmap_sdbs, sis_sdslen(context->wmap_sdbs));
        context->status = SIS_WMAP_HEAD;
    }
    if (context->status == SIS_WMAP_HEAD)
    {
        if (context->covermode)
        {
            // 是否清理旧的数据
            sis_disk_writer_delete(context->writer, "*", "*",
                // context->wmap_keys,
                // context->wsdbs,  // 只有表名 不需要结构
                context->curr_date);
            context->status = SIS_WMAP_MOVE;
        }
    }
    s_sis_db_chars *pchars = (s_sis_db_chars *)argv_;
    // printf("--1-- %s %s \n",pchars->kname, pchars->sname);
    sis_disk_writer_data(context->writer, pchars->kname, pchars->sname, pchars->data, pchars->size);
    // if (!sis_strcasecmp(pchars->kname, "SH600745"))
    // {
    //     printf("%s %d\n", __func__, pchars->size);
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
    if (sis_message_exist(msg, "overwrite"))
    {
        context->overwrite = sis_message_get_int(msg, "overwrite");
    }
    if (sis_message_exist(msg, "covermode"))
    {
        context->covermode = sis_message_get_int(msg, "covermode");
    }

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
