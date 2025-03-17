
#include "worker.h"
#include "server.h"

#include <sis_modules.h>
#include <sisdb_rcsv.h>
#include <sis_obj.h>
#include "sis_utils.h"
#include "sis_db.h"
#include "csv2db.h"

// 从行情流文件中获取数据源
static s_sis_method _sisdb_rcsv_methods[] = {
  {"get",    cmd_sisdb_rcsv_get, 0, NULL},
  {"getdb",  cmd_sisdb_rcsv_getdb, 0, NULL},
  {"sub",    cmd_sisdb_rcsv_sub, 0, NULL},
  {"unsub",  cmd_sisdb_rcsv_unsub, 0, NULL},
  {"setcb",  cmd_sisdb_rcsv_setcb, 0, NULL}
};

///////////////////////////////////////////////////
// *** s_sis_modules sis_modules_[dir name]  *** //
///////////////////////////////////////////////////
s_sis_modules sis_modules_sisdb_rcsv = {
    sisdb_rcsv_init,
    NULL,
    sisdb_rcsv_working,
    NULL,
    sisdb_rcsv_uninit,
    NULL,
    NULL,
    sizeof(_sisdb_rcsv_methods)/sizeof(s_sis_method),
    _sisdb_rcsv_methods,
};

bool sisdb_rcsv_init(void *worker_, void *node_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sis_json_node *node = (s_sis_json_node *)node_;
    s_sisdb_rcsv_cxt *context = SIS_MALLOC(s_sisdb_rcsv_cxt, context);
    worker->context = context;

    s_sis_json_node *init = sis_json_cmp_child_node(node, "work-date");
    if (init)
    {
        context->work_date.start = sis_json_get_int(init, "0", 0);
        context->work_date.stop  = sis_json_get_int(init, "1", 0);
    }
    else
    {
        context->work_date.start = 0;
        context->work_date.stop = 0;
    }
    // 都为0表示只取 path/name.csv 文件
    {
        context->work_path = sis_sds_save_create(sis_json_get_str(node, "work-path"), "data");   
        context->work_name = sis_sds_save_create(sis_json_get_str(node, "work-name"), "csvdb"); 
    }
    // 命名通常为 path/name.csv 无日期
    // path/name.20221212.csv 有日期
    // {
    //     context->sub_keys = sis_sdsempty();
    //     const char *str = sis_json_get_str(node, "sub-keys");
    //     if (str)
    //     {
    //         context->sub_keys = sis_sdscat(context->sub_keys, str);
    //     }
    //     else
    //     {
    //         context->sub_keys = sis_sdscat(context->sub_keys, "*");
    //     }
    // }
    // 字段为kname的为key需要过滤的信息
    return true;
}

void sisdb_rcsv_uninit(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)worker->context;

    // printf("%s : %d\n", __func__, context->status);
    if (context->status == SIS_RCSV_WORK || context->status == SIS_RCSV_CALL )
    {
        sisdb_rcsv_sub_stop(context);
        context->status = SIS_RCSV_EXIT;
    }

    sis_sds_save_destroy(context->work_path);
    sis_sds_save_destroy(context->work_name);
    sis_sdsfree(context->work_keys);
    sis_sdsfree(context->work_sdbs);
    sis_sdsfree(context->rcsv_keys);
    sis_sdsfree(context->rcsv_sdbs);

    sis_free(context);
    worker->context = NULL;
}


///////////////////////////////////////////
//  callback define begin
///////////////////////////////////////////
// static msec_t _speed_open = 0;
// static msec_t _speed_start = 0;

// static void cb_open(void *context_, int idate)
// {
//     s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)context_;
//     if (context->cb_sub_open)
//     {
//         char sdate[32];
//         sis_llutoa(idate, sdate, 32, 10);
//         context->cb_sub_open(context->cb_source, sdate);
//     } 
//     _speed_open = sis_time_get_now_msec();
//     LOG(8)("rcsv sub open. %d \n", idate);
// }
// static void cb_close(void *context_, int idate)
// {
//     s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)context_;
//     if (context->cb_sub_close)
//     {
//         char sdate[32];
//         sis_llutoa(idate, sdate, 32, 10);
//         context->cb_sub_close(context->cb_source, sdate);
//     } 
//     LOG(8)("rcsv sub close. %d cost : %lld\n", idate, sis_time_get_now_msec() - _speed_open);
// }
// static void cb_start(void *context_, int idate)
// {
//     s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)context_;
//     if (context->cb_sub_start)
//     {
//         char sdate[32];
//         sis_llutoa(idate, sdate, 32, 10);
//         context->cb_sub_start(context->cb_source, sdate);
//     } 
//     _speed_start = sis_time_get_now_msec();
//     LOG(8)("rcsv sub start. %d \n", idate);
// }
// static void cb_stop(void *context_, int idate)
// {
//     s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)context_;
//     LOG(8)("rcsv sub stop. %d cost : %lld\n", idate, sis_time_get_now_msec() - _speed_start);
//      // stop 放这里
//     if (context->cb_sub_stop)
//     {
//         char sdate[32];
//         sis_llutoa(idate, sdate, 32, 10);
//         context->cb_sub_stop(context->cb_source, sdate);
//     } 
// }
// static void cb_break(void *context_, int idate)
// {
//     s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)context_;
//     LOG(8)("sno sub break. %d cost : %lld\n", idate, sis_time_get_now_msec() - _speed_start);
//      // stop 放这里
//     // if (context->status != SIS_RCSV_BREAK)
//     {
//         if (context->cb_sub_stop)
//         {
//             context->cb_sub_stop(context->cb_source, "0");
//         } 
//     }
// }

// // #include "stk_struct.v4.h"
// // static int _read_nums = 0;
// static void cb_chardata(void *context_, const char *kname_, const char *sname_, void *out_, size_t olen_)
// {
//     s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)context_;
//     if (context->cb_sub_chars)
//     {
//         s_sis_db_chars inmem = {0};
//         inmem.kname= kname_;
//         inmem.sname= sname_;
//         inmem.data = out_;
//         inmem.size = olen_;
//         context->cb_sub_chars(context->cb_source, &inmem);
//     }
// } 

///////////////////////////////////////////
//  callback define end.
///////////////////////////////////////////

/**
 * @brief 
 * @param argv_ 工作者上下文
 * @return void* 
 */
static void *_thread_csvs_read_sub(void *argv_)
{
    // s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)argv_;

    // s_sis_disk_reader_cb *rcsv_cb = SIS_MALLOC(s_sis_disk_reader_cb, rcsv_cb);
    // rcsv_cb->cb_source    = context;
    // rcsv_cb->cb_open      = cb_open;
    // rcsv_cb->cb_close     = cb_close;
    // rcsv_cb->cb_start     = cb_start;
    // rcsv_cb->cb_dict_keys = cb_dict_keys;
    // rcsv_cb->cb_dict_sdbs = cb_dict_sdbs;
    // rcsv_cb->cb_chardata  = cb_chardata;
    // rcsv_cb->cb_stop      = cb_stop;
    // rcsv_cb->cb_break     = cb_break;

    // context->work_reader = sis_disk_reader_create(
    //     sis_sds_save_get(context->work_path), 
    //     sis_sds_save_get(context->work_name), 
    //     context->work_type, rcsv_cb);

    // s_sis_msec_pair pair; 
    // pair.start = (msec_t)sis_time_make_time(context->work_date.start, 0) * 1000;
    // pair.stop = (msec_t)sis_time_make_time(context->work_date.stop, 235959) * 1000 + 999;

    // LOG(5)("sub csv open. [%d] %d %d\n", context->work_date.start, context->work_date.stop, context->status);
    // sis_disk_reader_sub_csv(context->work_reader, context->work_mode, context->work_keys, context->work_sdbs, &pair);
    // LOG(5)("sub csv stop. [%d] %d %d\n", context->work_date.start, context->work_date.stop, context->status);

    // sis_disk_reader_destroy(context->work_reader);
    // context->work_reader = NULL;

    // sis_free(rcsv_cb);

    // LOG(5)("sub sno stop. ok [%d] %d\n", context->work_date, context->status);
    // context->status = SIS_RCSV_NONE;
    // 这里设置状态 重新创建线程后需要等待一会儿
    return NULL;
}
/**
 * @brief 启动一天的行情订阅
 * @param context 工作者上下文
 */
void sisdb_rcsv_sub_start(s_sisdb_rcsv_cxt *context) 
{
    // 有值就干活 完毕后释放
    if (context->status == SIS_RCSV_WORK)
    {
        _thread_csvs_read_sub(context);
    }
    else
    {
        // 这里用同步方式
        sis_thread_create(_thread_csvs_read_sub, context, &context->work_thread);
    }
}
void sisdb_rcsv_sub_stop(s_sisdb_rcsv_cxt *context)
{
    if (context->work_reader)
    {
        printf("stop sub..0.. %d %d\n", context->status, context->work_reader->status_sub);
        sis_disk_reader_unsub(context->work_reader);
        // 下面代码在线程中死锁 
        // while (context->status != SIS_RCSV_NONE)
        // {
        //     printf("stop sub... %d %d\n", context->status, context->work_reader->status_sub);
        //     sis_sleep(100);
        // }
        // sis_thread_create sis_thread_join 应该配对出现
        // sis_thread_join(&context->work_thread);
        printf("stop sub..1.. %d\n", context->status);
    }
}
///////////////////////////////////////////
//  method define
/////////////////////////////////////////

/**
 * @brief 将字段和回调函数设置从通信上下文中取出并设置到工作者上下文中
 * @param context 工作者上下文
 * @param msg 通信上下文
 */
void _sisdb_rcsv_init(s_sisdb_rcsv_cxt *context, s_sis_message *msg)
{
    // sis_sds_save_set(context->work_path, sis_message_get_str(msg, "work-path"));
    // sis_sds_save_set(context->work_name, sis_message_get_str(msg, "work-name"));
    // {
    //     s_sis_sds str = sis_message_get_str(msg, "sub-keys");
    //     if (str)
    //     {
    //         sis_sdsfree(context->work_keys);
    //         context->work_keys = sis_sdsdup(str);
    //     }
    // }
    // {
    //     s_sis_sds str = sis_message_get_str(msg, "sub-sdbs");
    //     if (str)
    //     {
    //         sis_sdsfree(context->work_sdbs);
    //         context->work_sdbs = sis_sdsdup(str);
    //     }
    // }
    // if (sis_message_exist(msg, "work-mode"))
    // {
    //     context->work_mode = sis_message_get_int(msg, "work-mode");
    // }
    // context->work_date.move = 0;
    // if (sis_message_exist(msg, "start-date"))
    // {
    //     context->work_date.start = sis_message_get_int(msg, "start-date");
    // }
    // else
    // {
    //     if (sis_message_exist(msg, "sub-date"))
    //     {
    //         context->work_date.start = sis_message_get_int(msg, "sub-date");
    //     }
    // }
    // if (sis_message_exist(msg, "stop-date"))
    // {
    //     context->work_date.stop = sis_message_get_int(msg, "stop-date");
    // }
    // else
    // {
    //     if (sis_message_exist(msg, "sub-date"))
    //     {
    //         context->work_date.stop = sis_message_get_int(msg, "sub-date");
    //     }
    // }
    // context->cb_source      = sis_message_get(msg, "cb_source");
    // context->cb_sub_open    = sis_message_get_method(msg, "cb_sub_open"   );
    // context->cb_sub_close   = sis_message_get_method(msg, "cb_sub_close"  );
    // context->cb_sub_start   = sis_message_get_method(msg, "cb_sub_start"  );
    // context->cb_sub_stop    = sis_message_get_method(msg, "cb_sub_stop"   );
    // context->cb_dict_sdbs   = sis_message_get_method(msg, "cb_dict_sdbs"  );
    // context->cb_dict_keys   = sis_message_get_method(msg, "cb_dict_keys"  );
    // context->cb_sub_chars   = sis_message_get_method(msg, "cb_sub_chars"  );
}
int cmd_sisdb_rcsv_get(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)worker->context;

    s_sis_message *msg = (s_sis_message *)argv_; 
    int submode = sis_message_get_int(msg, "sub-mode");
    
    if (submode == 1)
    {
        // 先检查csv文件是否存在
        int subdate = sis_message_get_int(msg, "sub-date");
        s_sis_memory *memory = sis_csv2db_read_as_memory(
            sis_sds_save_get(context->work_path),
            sis_sds_save_get(context->work_name),
            subdate,
            sis_message_get_str(msg, "fields"),
            sis_message_get_int(msg, "offset"),
            sis_message_get_int(msg, "count"));
        
        // s_sis_memory *memory = sis_csv2db_as_memory(
        //     sis_sds_save_get(context->work_path),
        //     sis_sds_save_get(context->work_name),
        //     subdate);
        if (memory)
        {
            sis_message_set(msg, "object", sis_object_create(SIS_OBJECT_MEMORY, memory), sis_object_destroy);
            return SIS_METHOD_OK;
        }
    }
    return SIS_METHOD_NIL;
}
int cmd_sisdb_rcsv_getdb(void *worker_, void *argv_)
{
    // s_sis_worker *worker = (s_sis_worker *)worker_; 
    // s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)worker->context;

    // s_sis_message *msg = (s_sis_message *)argv_; 
    // // 设置表结构
    // // 设置数据对象
    // s_sis_disk_reader *reader = sis_disk_reader_create(
    //     sis_sds_save_get(context->work_path), 
    //     sis_sds_save_get(context->work_name), 
    //     context->work_type, NULL);
    // if (!reader)
    // {
    //     return SIS_METHOD_NIL;
    // }
    // // 这里不指定日期 MPAP文件只有一个结构
    // s_sis_object *obj = sis_disk_reader_get_sdbs(reader, 0);
    // sis_disk_reader_destroy(reader);
    // if (!obj)
    // {
    //     return SIS_METHOD_NIL;
    // }
    // s_sis_sds dbinfo = sis_sdsdup(SIS_OBJ_SDS(obj));
    // sis_object_destroy(obj);
    // sis_message_set(msg, "dbinfo", dbinfo, sis_sdsfree_call);
    return SIS_METHOD_OK;
}

int cmd_sisdb_rcsv_sub(void *worker_, void *argv_)
{
    // s_sis_worker *worker = (s_sis_worker *)worker_; 
    // s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)worker->context;

    // SIS_WAIT_OR_EXIT(context->status == SIS_RCSV_NONE);  
    // // 可能上一个线程还未结束
    // sis_sleep(3);

    // s_sis_message *msg = (s_sis_message *)argv_; 
    // if (!msg)
    // {
    //     return SIS_METHOD_ERROR;
    // }
    // _sisdb_rcsv_init(context, msg);
    
    // context->status = SIS_RCSV_CALL;
    
    // sisdb_rcsv_sub_start(context);

    return SIS_METHOD_OK;
}
int cmd_sisdb_rcsv_unsub(void *worker_, void *argv_)
{
    // s_sis_worker *worker = (s_sis_worker *)worker_; 
    // s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)worker->context;
    // context->status = SIS_RCSV_BREAK;
    // sisdb_rcsv_sub_stop(context);

    return SIS_METHOD_OK;
}

int cmd_sisdb_rcsv_setcb(void *worker_, void *argv_)
{
    // s_sis_worker *worker = (s_sis_worker *)worker_; 
    // s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)worker->context;

    // if (context->status != SIS_RCSV_NONE)
    // {
    //     return SIS_METHOD_ERROR;
    // }
    // s_sis_message *msg = (s_sis_message *)argv_; 
    // if (!msg)
    // {
    //     return SIS_METHOD_ERROR;
    // }
    // _sisdb_rcsv_init(context, msg);
    
    // context->status = SIS_RCSV_WORK;

    return SIS_METHOD_OK;
}

void sisdb_rcsv_working(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rcsv_cxt *context = (s_sisdb_rcsv_cxt *)worker->context;
    
    SIS_WAIT_OR_EXIT(context->status == SIS_RCSV_WORK);  
    if (context->status == SIS_RCSV_WORK)
    {
        LOG(5)("sub history start. [%d]\n", context->work_date.start);
        sisdb_rcsv_sub_start(context);
        LOG(5)("sub history end. [%d]\n", context->work_date.stop);
    }
}
#if 0
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void* thread_function(void* arg) {
    printf("Thread started\n");
    usleep(2000000); // sleep for 1 second
    printf("Thread ended\n");
    return NULL;
}

int main() {
    pthread_t thread;
    printf("Creating thread\n");
    pthread_create(&thread, NULL, thread_function, NULL);
    // usleep(100000);

    printf("Main thread sleeping\n");
    usleep(5000000); // sleep for 0.5 second
    printf("Main thread awake\n");

    pthread_join(thread, 0);
    printf("Thread joined\n");

    printf("============\n");
    s_sis_thread       work_thread;
    printf("Creating thread\n");
    sis_thread_create_sync(thread_function, NULL, &work_thread);
    printf("Main thread sleeping\n");
    usleep(5000000); // sleep for 0.5 second
    printf("Main thread awake\n");

    sis_thread_join(&work_thread);
    // sis_thread_clear(&work_thread);
    printf("Thread joined\n");
    return 0;
}
// 主线程运行时间长
// Creating thread
// Main thread sleeping
// Thread started
// Thread ended
// Main thread awake
// Thread joined
// ============
// Creating thread
// Main thread sleeping
// Thread started
// Thread ended
// Main thread awake
// Thread joined
// 主线程运行时间短
// Creating thread
// Main thread sleeping
// Thread started
// Main thread awake
// Thread ended
// Thread joined
// ============
// Creating thread
// Main thread sleeping
// Thread started
// Main thread awake
// Thread ended
// Thread joined
// 线程执行顺序
#endif
#if 0
// 测试 snapshot 转 新格式的例子
const char *sisdb_rcsv = "\"sisdb_rcsv\" : { \
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
    // printf("=====%s : \n", __func__);
    return 0;
}
int cb_dict_sdbs1(void *worker_, void *argv_)
{
    // printf("%s : %s\n", __func__, (char *)argv_);
    // printf("=====%s : \n", __func__);
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
    if (!sis_strcasecmp(inmem->sname, "stk_orders") && inmem->kname[1] == 'H')
    {
        printf("inmem->sname %s \n", inmem->kname);
        // s_v4_stk_snapshot *snap = (s_v4_stk_snapshot *)inmem->data;
        // printf("%s %lld %d %lld\n", inmem->kname, snap->time, sis_zint32_i(snap->newp), snap->volume);
    }
    _recv_count++;
    return 0;
}

int main()
{
    sis_server_init();
    s_sis_worker *nowwork = NULL;
    {
        s_sis_json_handle *handle = sis_json_load(sisdb_rcsv, sis_strlen(sisdb_rcsv));
        nowwork = sis_worker_create(NULL, handle->node);
        sis_json_close(handle);
    }

    s_sis_message *msg = sis_message_create(); 
    sis_message_set(msg, "cb_source", nowwork, NULL);
    sis_message_set_int(msg, "sub-date", 20210601);
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