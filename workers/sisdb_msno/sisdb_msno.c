
#include "worker.h"
#include "server.h"

#include <sis_modules.h>
#include <sisdb_msno.h>
#include <sis_obj.h>
#include "sis_utils.h"

// 从行情流文件中获取数据源
static s_sis_method _sisdb_msno_methods[] = {
//   {"get",    cmd_sisdb_msno_get, 0, NULL},
  {"sub",    cmd_sisdb_msno_sub, 0, NULL},
  {"unsub",  cmd_sisdb_msno_unsub, 0, NULL},
};

///////////////////////////////////////////////////
// *** s_sis_modules sis_modules_[dir name]  *** //
///////////////////////////////////////////////////
s_sis_modules sis_modules_sisdb_msno = {
    sisdb_msno_init,
    NULL,
    NULL,
    NULL,
    sisdb_msno_uninit,
    NULL,
    NULL,
    sizeof(_sisdb_msno_methods)/sizeof(s_sis_method),
    _sisdb_msno_methods,
};

///////////////////////////////////////////
//  callback define begin
///////////////////////////////////////////

bool sisdb_msno_init(void *worker_, void *node_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sis_json_node *node = (s_sis_json_node *)node_;
    s_sisdb_msno_cxt *context = SIS_MALLOC(s_sisdb_msno_cxt, context);
    worker->context = context;

    {
        const char *str = sis_json_get_str(node, "work-path");
        if (str)
        {
            context->work_path = sis_sdsnew(str);
        }
        else
        {
            context->work_path = sis_sdsnew("./");
        }
    }
    s_sis_json_node *innode = sis_json_cmp_child_node(node, "work-snos");  
    if (innode)
    {         
        int nums = 0;
        s_sis_json_node *next = sis_json_first_node(innode);
        while (next)
        {           
            nums++;
            next = next->next;
        }
        if (nums > 1)
        {
            context->readers = sis_pointer_list_create();
            context->readers->vfree = sisdb_msub_reader_destroy;
            s_sis_json_node *next = sis_json_first_node(innode);
            while (next)
            {           
                // 增加根路径
                sis_json_merge_rpath(next, "work-path", context->work_path);
                s_sisdb_msub_reader *reader = sisdb_msub_reader_create(context, next);
                if (reader)
                {
                    sis_pointer_list_push(context->readers, reader);
                }
                next = next->next;
            }
            context->maps_sdbs = sis_map_list_create(sisdb_msub_db_destroy);
        }
        else
        {
            s_sis_json_node *next = sis_json_first_node(innode);
            sis_json_merge_rpath(next, "work-path", context->work_path);
            context->worker = sis_worker_create(worker, next);
            context->maps_sdbs = sis_map_list_create(sis_dynamic_db_destroy);
        }
    }
    sis_mutex_init(&context->worklock, NULL);
    context->maps_keys = sis_map_list_create(sis_sdsfree_call); 
    // context->maps_sdbs = sis_map_list_create(sisdb_msub_db_destroy);

    context->status = SIS_MSNO_NONE;

    return true;
}

void sisdb_msno_uninit(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;

    // printf("%s : %d\n", __func__, context->status);
    if (context->worker)
    {
        if (context->worker)
        {
            sis_worker_destroy(context->worker);
        }
    }
    else
    {
        sisdb_msno_sub_stop(context);
        context->status = SIS_MSNO_NONE;
        sis_pointer_list_destroy(context->readers);
    }

    if (context->maps_sdbs)
    {
        sis_map_list_destroy(context->maps_sdbs);
    }
    if (context->maps_keys)
    {
        sis_map_list_destroy(context->maps_keys);    
    }
    sis_sdsfree(context->sub_keys);
    sis_sdsfree(context->sub_sdbs);

    sis_free(context);
    worker->context = NULL;
}


///////////////////////////////////////////
//  callback define begin mul sno
///////////////////////////////////////////

static int cb_sub_start(void *worker_, void *argv_)
{
    s_sisdb_msub_reader *mreader = worker_;
    s_sisdb_msno_cxt *context = mreader->father;
    
    mreader->status = SIS_READER_OPEN;
    
    sis_mutex_lock(&context->worklock);
    context->work_date = sis_atoll(argv_);
    int started = 1;
    for (int i = 0; i < context->readers->count; i++)
    {
        s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, i);
        if (mreader->valid && mreader->status != SIS_READER_OPEN)
        {
            started = 0;
            break;
        }
    }
    if (started)
    {
        context->cb_sub_start(context->cb_source, argv_);
    }
    sis_mutex_unlock(&context->worklock);
    return SIS_METHOD_OK;
}
static int cb_sub_stop(void *worker_, void *argv_)
{
	s_sisdb_msub_reader *mreader = worker_;
    s_sisdb_msno_cxt *context = mreader->father;
    mreader->status = SIS_READER_STOP;
    // printf("---%p %d\n", mreader, mreader->status);
    sis_mutex_lock(&context->worklock);
    int stoped = 1;
    for (int i = 0; i < context->readers->count; i++)
    {
        s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, i);
        if (mreader->valid && mreader->status != SIS_READER_STOP)
        {
            stoped = 0;
            break;
        }
    }
    if (stoped)
    {
        // context->cb_sub_stop(context->cb_source, argv_);
        context->status = SIS_MSNO_STOP;
    }
    sis_mutex_unlock(&context->worklock);
    return SIS_METHOD_OK;
}

static int cb_dict_keys(void *worker_, void *argv_)
{
	s_sisdb_msub_reader *mreader = worker_;
    s_sisdb_msno_cxt *context = mreader->father;
    
    sis_mutex_lock(&context->worklock);
    const char *keys = argv_;
    s_sis_string_list *klist = sis_string_list_create();
    sis_string_list_load(klist, keys, sis_strlen(keys), ",");
    int count = sis_string_list_getsize(klist);
    for (int i = 0; i < count; i++)
    {
        const char *code = sis_string_list_get(klist, i);
        sis_map_list_set(context->maps_keys, code, sis_sdsnew(code));
    }
    sis_string_list_destroy(klist);
    sis_mutex_unlock(&context->worklock);
    return SIS_METHOD_OK;
}
static int cb_dict_sdbs(void *worker_, void *argv_)
{
	// s_sisdb_msub_reader *mreader = worker_;
    // s_sisdb_msno_cxt *context = mreader->father;
    // sis_mutex_lock(&context->worklock);
    // sis_mutex_unlock(&context->worklock);
    return SIS_METHOD_OK;
}

s_sis_sds _sisdb_msno_match_key(s_sisdb_msno_cxt *context)
{
    s_sis_sds o = NULL;
    int count = sis_map_list_getsize(context->maps_keys);
	for (int i = 0; i < count; i++)
	{
		s_sis_sds key = sis_map_list_geti(context->maps_keys, i);
        if (context->sub_keys[0] != '*')
        {
            if (sis_str_subcmp(key, context->sub_keys, ',') < 0)
            {
                continue;
            }
        }
        if (o)
		{
			o = sis_sdscatfmt(o, ",%s", key);
		}
		else
		{
			o = sis_sdsnew(key);
		}
    }
	return o;
}
s_sis_sds _sisdb_msno_match_sdb(s_sisdb_msno_cxt *context)
{
    s_sis_json_node *innode = sis_json_create_object();
    int count = sis_map_list_getsize(context->maps_sdbs);
    for (int i = 0; i < count; i++)
    {
        s_sisdb_msub_db *mdb = sis_map_list_geti(context->maps_sdbs, i);
        if (context->sub_sdbs[0] != '*')
        {
            if (sis_str_subcmp_strict(mdb->rdb->name, context->sub_sdbs, ',') < 0)
            {
                continue;
            }
        }
		sis_json_object_add_node(innode, mdb->mname, sis_sdbinfo_to_json(mdb->rdb));
    }
    s_sis_sds o = sis_json_to_sds(innode, 1);
	sis_json_delete_node(innode);
	return o;
}

static void _sisdb_msno_send_head(s_sisdb_msno_cxt *context)
{
    if (context->started == 0)
    {
        int started = 1;
        for (int i = 0; i < context->readers->count; i++)
        {
            s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, i);
            if (mreader->valid && !sisdb_msub_reader_first(mreader))
            {
                started = 0;
                break;
            }
        }
        context->started = started;
    }
    if (context->sendhead == 1 || context->started == 0)
    {
        return ;
    }
    if (context->cb_dict_keys)
    {
        s_sis_sds str = _sisdb_msno_match_key(context);
        context->cb_dict_keys(context->cb_source, str);
	    sis_sdsfree(str);
    } 
    if (context->cb_dict_sdbs)
    {
        s_sis_sds str = _sisdb_msno_match_sdb(context);
        context->cb_dict_sdbs(context->cb_source, str);
        sis_sdsfree(str); 
    } 
    context->sendhead = 1;
}
static int cb_sub_chars(void *worker_, void *argv)
{
    s_sisdb_msub_reader *mreader = worker_;
    s_sisdb_msno_cxt *context = mreader->father;
    s_sis_db_chars *inmem = argv;
    sis_mutex_lock(&context->worklock);
    _sisdb_msno_send_head(context);
    char mname[1024];
    sis_str_merge(mname, 1024, '.', mreader->cname, inmem->sname);
    s_sisdb_msub_db *mdb = sis_map_list_get(context->maps_sdbs, mname);
    if (mdb)
    {
        if (mreader->nowmsec == 0)
        {
            if (mdb->rdb->field_time)
            {
                mreader->nowmsec = sis_dynamic_db_get_time(mdb->rdb, 0, inmem->data, inmem->size);
            }
            else
            {
                mreader->nowmsec = sis_time_make_msec(context->work_date, 0, 1);
            }
        }
        int kidx = sis_map_list_get_index(context->maps_keys, inmem->kname);
        sisdb_msub_reader_push(mreader, kidx, mdb->sidx, inmem->data, inmem->size);
        // LOG(0)("push: %d %d %d %d %d\n", mdb->ridx, mdb->sidx, kidx, sis_node_list_getsize(mreader->datas), sis_msec_get_itime(sis_dynamic_db_get_time(mdb->rdb, 0, inmem->data, inmem->size)));
    }
    sis_mutex_unlock(&context->worklock);
    return SIS_METHOD_OK;
}
// ///////////////////////////////////////////
// //  callback define end.
// ///////////////////////////////////////////

void sisdb_msno_sub_stop(s_sisdb_msno_cxt *context)
{
    if (context->status == SIS_MSNO_WORK)
    {
        for (int i = 0; i < context->readers->count; i++)
        {
            s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, i);
            sis_worker_command(mreader->worker, "unsub", NULL);
        }
    }
}
// ///////////////////////////////////////////
// //  method define
// /////////////////////////////////////////

int sisdb_msno_init_sdbs(s_sisdb_msno_cxt *context, int idate)
{
    // if (context->curr_date == idate && sis_map_list_getsize(context->maps_sdbs) > 0)
    // 加速 只读一次
    if (sis_map_list_getsize(context->maps_sdbs) > 0)
    {
        return 0;
    }
    sis_map_list_clear(context->maps_sdbs);
    // 读取全部的数据结构
    // printf("==1==%d \n", context->readers->count);
    s_sis_message *msg = sis_message_create();
    sis_message_set_int(msg, "sub-date", idate);
    sis_message_set_str(msg, "sub-sdbs", "*", 1);
    for (int i = 0; i < context->readers->count; i++)
    {
        s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, i);
        mreader->maxsize = 0;
        if (sis_worker_command(mreader->worker, "getdb", msg) == SIS_METHOD_OK)
        {
            s_sis_sds sdbs = sis_message_get(msg, "dbinfo");
            {
                s_sis_json_handle *injson = sis_json_load(sdbs, sis_sdslen(sdbs));
                if (!injson)
                {
                    continue;;
                }
                s_sis_json_node *innode = sis_json_first_node(injson->node);
                while (innode)
                {
                    s_sis_dynamic_db *db = sis_dynamic_db_create(innode);
                    if (db)
                    {
                        s_sisdb_msub_db *mdb = sisdb_msub_db_create();
                        mdb->rdb = db;
                        mdb->ridx = i;
                        mdb->sidx = sis_map_list_getsize(context->maps_sdbs);
                        sis_str_merge(mdb->mname, 255, '.', mreader->cname, db->name);
                        // 这里的maxsize并不准确 但不影响
                        mreader->maxsize = sis_max(mreader->maxsize, db->size);
                        sis_map_list_set(context->maps_sdbs, mdb->mname, mdb);
                    }
                    innode = sis_json_next_node(innode);
                }
                sis_json_close(injson);
            }
        }
    }
    sis_message_destroy(msg);
    return sis_map_list_getsize(context->maps_sdbs);
}

// int cmd_sisdb_msno_get(void *worker_, void *argv_)
// {
//     s_sis_worker *worker = (s_sis_worker *)worker_; 
//     s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;

//     s_sis_message *msg = (s_sis_message *)argv_; 
    
//     const char *subkeys = sis_message_get_str(msg, "sub-keys");
//     const char *subsdbs = sis_message_get_str(msg, "sub-sdbs");
//     if (sis_is_multiple_sub(subkeys, sis_strlen(subkeys)) || sis_is_multiple_sub(subsdbs, sis_strlen(subsdbs)))
//     {
//         return SIS_METHOD_NIL;
//     }
//     s_sisdb_msub_db *mdb = sis_map_list_get(context->maps_sdbs, subsdbs);
//     if (!mdb || mdb->ridx < 0 || mdb->ridx > context->readers->count - 1)
//     {
//         return SIS_METHOD_NIL;
//     }
//     s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, mdb->ridx);
//     if (!mreader->worker)
//     {
//          return SIS_METHOD_NIL;
//     }
//     return sis_worker_command(mreader->worker, "get", msg);
// }

void _sisdb_msno_init(s_sisdb_msno_cxt *context, s_sis_message *msg)
{
    
    sis_map_list_clear(context->maps_keys);
    
    sis_message_set_str(msg, "work-path", context->work_path, sis_strlen(context->work_path));

    context->work_date = sis_time_get_idate(0);
    if (sis_message_exist(msg, "sub-date"))
    {
        context->work_date = sis_message_get_int(msg, "sub-date");
    }
    sisdb_msno_init_sdbs(context, context->work_date);

    {
        s_sis_sds str = sis_message_get_str(msg, "sub-keys");
        if (str)
        {
            sis_sdsfree(context->sub_keys);
            context->sub_keys = sis_sdsdup(str);
        }
    }
    {
        s_sis_sds str = sis_message_get_str(msg, "sub-sdbs");
        if (str)
        {
            sis_sdsfree(context->sub_sdbs);
            context->sub_sdbs = sis_sdsdup(str);
        }
    }
    context->cb_source      = sis_message_get(msg, "cb_source");
    context->cb_sub_start   = sis_message_get_method(msg, "cb_sub_start"  );
    context->cb_sub_stop    = sis_message_get_method(msg, "cb_sub_stop"   );
    context->cb_dict_sdbs   = sis_message_get_method(msg, "cb_dict_sdbs"  );
    context->cb_dict_keys   = sis_message_get_method(msg, "cb_dict_keys"  );
    context->cb_sub_chars   = sis_message_get_method(msg, "cb_sub_chars"  );
}
s_sisdb_msub_reader *_thread_msno_get_reader(s_sisdb_msno_cxt *context)
{
    s_sisdb_msub_reader *reader = NULL;
    for (int i = 0; i < context->readers->count; i++)
    {
        s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, i);
        if (!mreader->valid || mreader->stoped)
        {
            continue;
        }
        if (sis_node_list_getsize(mreader->datas) < 1 && mreader->status != SIS_READER_STOP)
        {
            // 只要有数据被消耗完 中止提取数据
            return NULL;
        }
        if (!reader)
        {
            reader = mreader;
        }
        else
        {
            if (reader->nowmsec > mreader->nowmsec)
            {
                reader = mreader;
                // printf("==1== %d %d\n", i, sis_msec_get_itime(reader->nowmsec));
            }
        }
    }
    return reader;
}
static void *_thread_msno_ctrl_sub(void *argv_)
{
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)argv_;
    while (context->status != SIS_MSNO_NONE)
    {
        // printf("===1===\n");
        if (!context->started && context->status != SIS_MSNO_STOP)
        {
            sis_sleep(30);
            continue;
        }
        if (context->status == SIS_MSNO_STOP)
        {
            int stoped = 1;
            for (int i = 0; i < context->readers->count; i++)
            {
                s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, i);
                if (sis_node_list_getsize(mreader->datas) > 0)
                {
                    // printf("=====stop %d %d %d\n", i, sis_node_list_getsize(mreader->datas), mreader->status);
                    stoped = 0;
                }
                else
                {
                    mreader->stoped = 1;
                }
            }
            if (stoped)
            {
                break;
            }
        }
        // s_sisdb_msub_reader *reader = NULL;
        // printf("=== [%d] ==\n", context->readers->count);
        // 每次锁定尽量把所有数据发送完毕 避免内存堆积过大
        // 除非某个读者没有数据 就等等
        if (!sis_mutex_trylock(&context->worklock))
        {
            s_sisdb_msub_reader *reader = NULL; 
            while ((reader = _thread_msno_get_reader(context)) != NULL)
            {
                if (context->cb_sub_chars)
                {
                    s_sis_msub_v *msubv = sis_node_list_get(reader->datas, 0);   
                    if (msubv)
                    {
                        s_sis_db_chars inmem = {0};
                        s_sisdb_msub_db *mdb = sis_map_list_geti(context->maps_sdbs, msubv->sidx);
                        inmem.kname = (char *)sis_map_list_geti(context->maps_keys, msubv->kidx);
                        inmem.sname = mdb->mname;
                        inmem.size  = msubv->size;
                        inmem.data  = msubv->data;
                        context->cb_sub_chars(context->cb_source, &inmem);
                        
                        sis_node_list_pop(reader->datas);
                        // printf("==1== %d %lld %d %d %s %s| %d %d | %d\n", sis_msec_get_itime(reader->nowmsec), reader->nowmsec, msubv->kidx, msubv->sidx, inmem.kname, inmem.sname, reader->datas->count, reader->datas->nodes->count, reader->datas->nouse);
                        // 更新最新记录
                        msubv = sis_node_list_get(reader->datas, 0); 
                        // if (!msubv)
                        // {
                        //     LOG(0)("%d \n", sis_node_list_getsize(reader->datas));
                        // }
                        if (msubv)
                        {
                            // printf("==2== %d %lld\n", sis_msec_get_itime(reader->nowmsec), reader->nowmsec);
                            s_sisdb_msub_db *mdb = sis_map_list_geti(context->maps_sdbs, msubv->sidx);
                            if (mdb->rdb->field_time)
                            {
                                reader->nowmsec = sis_dynamic_db_get_time(mdb->rdb, 0, msubv->data, msubv->size);
                            }
                            else
                            {
                                // 不改变时序
                                reader->nowmsec = sis_time_make_msec(context->work_date, 0, 1);
                            }
                            // printf("==2.1== %d %lld\n", sis_msec_get_itime(reader->nowmsec), reader->nowmsec);
                        }
                    }
                    if (!msubv)
                    {
                        if (reader->status != SIS_READER_STOP)
                        {
                            // reader->nowmsec = 0; // 数据未结束 要等待一会儿
                        }
                        else
                        {
                            reader->stoped = 1;
                        }
                    }
                    
                    // printf("==2== %s %d %p count : %d\n", reader->cname, sis_msec_get_itime(reader->nowmsec), msubv, sis_node_list_getsize(reader->datas));
                }
                // reader = _thread_msno_get_reader(context);
            }
            sis_mutex_unlock(&context->worklock);
        }
        // printf("===111===\n");
        sis_sleep(10); 
    }
    context->status = SIS_MSNO_NONE;
    // 这里最后发送停止回调
    char sdate[32];
    sis_lldtoa(context->work_date, sdate, 32, 10);
    context->cb_sub_stop(context->cb_source, sdate);

    return NULL;
}
int _sisdb_msno_is_subdb(s_sisdb_msno_cxt *context, int ridx)
{
    if (context->sub_sdbs[0] == '*')
    {
        return 1;
    }
    int count = sis_str_substr_nums(context->sub_sdbs, sis_sdslen(context->sub_sdbs), ',');
    for (int i = 0; i < count; i++)
    {
        char sname[255];
        sis_str_substr(sname, 255, context->sub_sdbs, ',', i);

        int nums = sis_map_list_getsize(context->maps_sdbs);
        for (int k = 0; k < nums; k++)
        {
            s_sisdb_msub_db *mdb = sis_map_list_geti(context->maps_sdbs, k);
            if (!sis_strcasecmp(mdb->rdb->name, sname) && mdb->ridx == ridx)
            {
                return 1;
            }
        }
    }
    return 0;
}

///////////////////////////////////////////
//  callback define begin one sno
///////////////////////////////////////////

static int cb_single_sub_start(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;
    context->sendhead = 0;
    context->cb_sub_start(context->cb_source, argv_);
    return SIS_METHOD_OK;
}
static int cb_single_dict_keys(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;
    sis_sdsfree(context->work_keys);
    context->work_keys = sis_sdsdup(argv_); 
    return SIS_METHOD_OK;
}

static int cb_single_dict_sdbs(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;
    sis_sdsfree(context->work_sdbs);
    context->work_sdbs = sis_sdsdup(argv_); 
    return SIS_METHOD_OK;
}
static int cb_single_sub_stop(void *worker_, void *argv_)
{
	s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;
    context->sendhead = 0;
    context->cb_sub_stop(context->cb_source, argv_);
    context->status = SIS_MSNO_NONE;
    return SIS_METHOD_OK;
}

static int cb_single_sub_chars(void *worker_, void *argv)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;
    if (context->sendhead == 0)
    {
        if (context->cb_dict_keys)
        {
            context->cb_dict_keys(context->cb_source, context->work_keys);
        } 
        if (context->cb_dict_sdbs)
        {
            context->cb_dict_sdbs(context->cb_source, context->work_sdbs);
        } 
        context->sendhead = 1;
    }
    context->cb_sub_chars(context->cb_source, argv);
    return SIS_METHOD_OK;
}

void _sisdb_msno_read_single(s_sis_worker *worker)
{
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;
    s_sis_message *msg = sis_message_create();
    sis_message_set(msg, "cb_source", worker, NULL);
    sis_message_set_str(msg, "sub-keys", context->sub_keys, sis_sdslen(context->sub_keys));
    sis_message_set_str(msg, "sub-sdbs", context->sub_sdbs, sis_sdslen(context->sub_sdbs));
    sis_message_set_method(msg, "cb_sub_start", cb_single_sub_start);
    sis_message_set_method(msg, "cb_sub_stop",  cb_single_sub_stop);
    sis_message_set_method(msg, "cb_dict_keys", cb_single_dict_keys);
    sis_message_set_method(msg, "cb_dict_sdbs", cb_single_dict_sdbs);
    sis_message_set_method(msg, "cb_sub_chars", cb_single_sub_chars);     
    sis_message_set_int(msg, "sub-date", context->work_date);
    sis_worker_command(context->worker, "sub", msg);
    sis_message_destroy(msg);
}
///////////////////////////////////////////
//  method define begin one sno
///////////////////////////////////////////

int cmd_sisdb_msno_sub(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;

    SIS_WAIT_OR_EXIT(context->status == SIS_MSNO_NONE);  

    s_sis_message *msg = (s_sis_message *)argv_; 
    if (!msg)
    {
        return SIS_METHOD_ERROR;
    }
    if (context->worker)
    {
        context->status = SIS_MSNO_WORK;
        context->sendhead = 0;

        context->work_date = sis_time_get_idate(0);
        if (sis_message_exist(msg, "sub-date"))
        {
            context->work_date = sis_message_get_int(msg, "sub-date");
        }
        {
            s_sis_sds str = sis_message_get_str(msg, "sub-keys");
            sis_sdsfree(context->sub_keys);
            if (str)
            {
                context->sub_keys = sis_sdsdup(str);
            }
            else
            {
                context->sub_keys = sis_sdsnew("*");
            }
        }
        {
            s_sis_sds str = sis_message_get_str(msg, "sub-sdbs");
            sis_sdsfree(context->sub_sdbs);
            if (str)
            {
                context->sub_sdbs = sis_sdsdup(str);
            }
            else
            {
                context->sub_sdbs = sis_sdsnew("*");
            }
        }
        context->cb_source      = sis_message_get(msg, "cb_source");
        context->cb_sub_start   = sis_message_get_method(msg, "cb_sub_start"  );
        context->cb_sub_stop    = sis_message_get_method(msg, "cb_sub_stop"   );
        context->cb_dict_sdbs   = sis_message_get_method(msg, "cb_dict_sdbs"  );
        context->cb_dict_keys   = sis_message_get_method(msg, "cb_dict_keys"  );
        context->cb_sub_chars   = sis_message_get_method(msg, "cb_sub_chars"  );

        _sisdb_msno_read_single(worker);
    }
    else
    {
        _sisdb_msno_init(context, msg);
        LOG(5)("init msno ok. %d, %s\n", sis_map_list_getsize(context->maps_sdbs), context->sub_keys);
        // for (int i = 0; i < sis_map_list_getsize(context->maps_sdbs); i++)
        // {
        //     s_sisdb_msub_db *mdb = sis_map_list_geti(context->maps_sdbs, i);
        //     printf("mdb: %s %d\n", mdb->rdb->name, mdb->ridx);
        // }
        
        if (sis_map_list_getsize(context->maps_sdbs) > 0)
        {
            context->status = SIS_MSNO_WORK;
            context->sendhead = 0;
            context->started  = 0;

            sis_thread_create(_thread_msno_ctrl_sub, context, &context->work_thread);

            s_sis_message *msg = sis_message_create();
            sis_message_set_int(msg, "sub-date",  context->work_date);
            sis_message_set_str(msg, "sub-keys",  context->sub_keys, sis_strlen(context->sub_keys));
            // ??? 订阅 * 时失败 后面再看什么问题
            // sis_message_set_str(msg, "sub-sdbs",  context->sub_sdbs, sis_strlen(context->sub_sdbs));
            sis_message_set_str(msg, "work-path", context->work_path, sis_strlen(context->work_path));
            sis_message_set_method(msg, "cb_sub_start", cb_sub_start);
            sis_message_set_method(msg, "cb_sub_stop" , cb_sub_stop );
            sis_message_set_method(msg, "cb_dict_sdbs", cb_dict_sdbs);
            sis_message_set_method(msg, "cb_dict_keys", cb_dict_keys);
            sis_message_set_method(msg, "cb_sub_chars", cb_sub_chars);
            for (int i = 0; i < context->readers->count; i++)
            {
                s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, i);  
                sisdb_msub_reader_init(mreader, mreader->maxsize, 1024);
                if (_sisdb_msno_is_subdb(context, i))
                {
                    mreader->valid = 1;
                }  
                else
                {
                    mreader->valid = 0;
                }        
            }
            for (int i = 0; i < context->readers->count; i++)
            {
                s_sisdb_msub_reader *mreader = sis_pointer_list_get(context->readers, i);
                mreader->status = SIS_READER_STOP;  
                if (mreader->valid)
                {
                    sis_message_set(msg, "cb_source", mreader, NULL);
                    // 这里需要筛选出匹配当前reader的sdbs
                    sis_message_set_str(msg, "sub-sdbs",  context->sub_sdbs, sis_strlen(context->sub_sdbs));
                    if (sis_worker_command(mreader->worker, "sub", msg) == SIS_METHOD_OK)
                    {
                        mreader->status = SIS_READER_INIT;
                    }
                    else
                    {
                        mreader->nowmsec = sis_time_make_msec(context->work_date, 235959, 999);
                    }
                }          
            }
            sis_message_destroy(msg);
        }
        else
        {
            char sdate[32];
            sis_llutoa(sis_message_get_int(msg, "sub-date"), sdate, 32, 10);
            context->cb_sub_stop(context->cb_source, sdate);
        }
    }

    return SIS_METHOD_OK;
}
int cmd_sisdb_msno_unsub(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_msno_cxt *context = (s_sisdb_msno_cxt *)worker->context;

    if (context->worker)
    {
        sis_worker_command(context->worker, "unsub", NULL);
    }
    else
    {
        sisdb_msno_sub_stop(context);
    }

    return SIS_METHOD_OK;
}


#if 0
// 测试 snapshot 转 新格式的例子
const char *sisdb_msno = "\"sisdb_msno\" : { \
    \"work-path\" : \"../../data/\" }";

#include "server.h"
#include "sis_db.h"
#include "stk_struct.v0.h"

int _recv_count = 0;

int cb_sub_start1(void *worker_, void *argv_)
{
    printf("%s : %s\n", __func__, (char *)argv_);
    _recv_count = 0;
    return 0;
}

int cb_sub_stop1(void *worker_, void *argv_)
{
    printf("%s : %s\n", __func__, (char *)argv_);
    return 0;
}
int cb_dict_keys1(void *worker_, void *argv_)
{
    // printf("%s : %s\n", __func__, (char *)argv_);
    printf("=====%s : \n", __func__);
    return 0;
}
int cb_dict_sdbs1(void *worker_, void *argv_)
{
    // printf("%s : %s\n", __func__, (char *)argv_);
    printf("=====%s : \n", __func__);
    return 0;
}

int cb_sub_chars1(void *worker_, void *argv_)
{
    s_sis_db_chars *inmem = (s_sis_db_chars *)argv_;
    if (_recv_count % 1000000 == 0)
    {
        printf("%d %s %s : %d\n", _recv_count, inmem->kname, inmem->sname, inmem->size);
    }
    // if (!sis_strcasecmp(inmem->sname, "stk_snapshot"))
    // {
        // s_v4_stk_snapshot *snap = (s_v4_stk_snapshot *)inmem->data;
        // printf("%s %lld %d %lld\n", inmem->kname, snap->time, sis_zint32_i(snap->newp), snap->volume);
    // }
    _recv_count++;
    return 0;
}

int main()
{
    sis_server_init();
    s_sis_worker *nowwork = NULL;
    {
        s_sis_json_handle *handle = sis_json_load(sisdb_msno, sis_strlen(sisdb_msno));
        nowwork = sis_worker_create(NULL, handle->node);
        sis_json_close(handle);
    }

    s_sis_message *msg = sis_message_create(); 
    sis_message_set(msg, "cb_source", nowwork, NULL);
    sis_message_set_int(msg, "sub-date", 20210617);
    sis_message_set_method(msg, "cb_sub_start"     ,cb_sub_start1      );
    sis_message_set_method(msg, "cb_sub_stop"      ,cb_sub_stop1       );
    sis_message_set_method(msg, "cb_dict_sdbs"     ,cb_dict_sdbs1      );
    sis_message_set_method(msg, "cb_dict_keys"     ,cb_dict_keys1      );
    sis_message_set_method(msg, "cb_sub_chars"     ,cb_sub_chars1    );
    sis_worker_command(nowwork, "sub", msg);
    sis_message_destroy(msg); 
    while (1)
    {
        sis_sleep(5000);
    }
    
    sis_worker_destroy(nowwork);
    sis_server_uninit();
}
#endif