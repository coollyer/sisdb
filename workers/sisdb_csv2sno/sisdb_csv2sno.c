
#include "worker.h"
#include "server.h"

#include <sis_modules.h>
#include <sisdb_csv2sno.h>
#include <sis_obj.h>
#include "sis_utils.h"
#include "sis_list.h"
#include "sis_db.h"

// 从行情流文件中获取数据源
static s_sis_method _sisdb_csv2sno_methods[] = {
  {"setcb",  cmd_sisdb_csv2sno_setcb, 0, NULL}
};

///////////////////////////////////////////////////
// *** s_sis_modules sis_modules_[dir name]  *** //
///////////////////////////////////////////////////
s_sis_modules sis_modules_sisdb_csv2sno = {
    sisdb_csv2sno_init,
    NULL,
    sisdb_csv2sno_working,
    NULL,
    sisdb_csv2sno_uninit,
    NULL,
    NULL,
    sizeof(_sisdb_csv2sno_methods)/sizeof(s_sis_method),
    _sisdb_csv2sno_methods,
};
/////////////////////////////////
//
/////////////////////////////////
s_csvdb_field *csvdb_field_create(int style, const char *fname)
{
    s_csvdb_field *o = SIS_MALLOC(s_csvdb_field, o);
    o->style = style;
    sis_strcpy(o->name, 128, fname);
    if (style == 1)
    {
        o->data = sis_int_list_create();
    }
    else if (style == 2)
    {
        o->data = sis_double_list_create();
    }
    else
    {
        o->data = sis_string_list_create();
    }
    return o;
}
void csvdb_field_destroy(void *fc)
{
    s_csvdb_field *o = fc;
    if (o->style == 1)
    {
        sis_int_list_destroy(o->data);
    }
    else if (o->style == 2)
    {
        sis_double_list_destroy(o->data);
    }
    else
    {
        sis_string_list_destroy(o->data);
    }
}
int csvdb_field_getsize(s_csvdb_field *fc)
{
    if (fc->style == 1)
    {
        s_sis_int_list *is = fc->data;
        return is->count;
    }
    else if (fc->style == 2)
    {
        return sis_double_list_getsize(fc->data);
    }
    else
    {
        return sis_string_list_getsize(fc->data);
    }
}
int csvdb_field_set_string(s_csvdb_field *fc, const char *in)
{
    if (fc->style == 0)
    {
        sis_string_list_push(fc->data, in, sis_strlen(in));
    }
    return csvdb_field_getsize(fc);
}
int csvdb_field_set_int(s_csvdb_field *fc, int64 in)
{
    if (fc->style == 1)
    {
        s_sis_int_list *is = fc->data;
        sis_int_list_push(is, in);
    }
    return csvdb_field_getsize(fc);
}
int csvdb_field_set_double(s_csvdb_field *fc, double in)
{
    if (fc->style == 2)
    {
        sis_double_list_push(fc->data, in);
    }
    return csvdb_field_getsize(fc);
}
const char *csvdb_field_get_string(s_csvdb_field *fc, int index)
{
    return sis_string_list_get(fc->data, index);
}
int64 csvdb_field_get_int(s_csvdb_field *fc, int index)
{
    s_sis_int_list *is = fc->data;
    return sis_int_list_get(is, index);
}
int csvdb_field_get_double(s_csvdb_field *fc, int index)
{
    return sis_double_list_get(fc->data, index);
}
/////////////////////////////////
//
/////////////////////////////////
bool sisdb_csv2sno_init(void *worker_, void *node_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sis_json_node *node = (s_sis_json_node *)node_;
    s_sisdb_csv2sno_cxt *context = SIS_MALLOC(s_sisdb_csv2sno_cxt, context);
    worker->context = context;

    // 默认为 0
    context->work_mode = sis_json_get_int(node, "work-mode", 0);
    {
        context->rpath = sis_sdsempty();
        const char *str = sis_json_get_str(node, "csv-path");
        // printf("%s\n", str);
        if (str)
        {
            context->rpath = sis_sdscat(context->rpath, str);
        }
        else
        {
            context->rpath = sis_sdscat(context->rpath, "csv");
        }
    }
    {
        context->rname = sis_sdsempty();
        const char *str = sis_json_get_str(node, "csv-name");
        if (str)
        {
            context->rname = sis_sdscat(context->rname, str);
        }
        else
        {
            context->rname = sis_sdscat(context->rname, "csv");
        }
    }
    {
        s_sis_json_node *innode = sis_json_cmp_child_node(node, "wsno");
        if (innode)
        {
            context->wsno = sis_worker_create(worker, innode);
            if (!context->wsno)
            {
                LOG(5)("create method fail. no wsno.\n");
            }
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
    // 下面开始关联插件的关系
    if (context->wsno)
    {
        s_sis_message *msg = sis_message_create();
        sis_message_set_int(msg, "overwrite", 1);
        if (sis_worker_command(context->wsno, "getcb", msg) != SIS_METHOD_OK)
        {
            LOG(5)("wfile getcb callback fail.\n");
            sis_message_destroy(msg);
            return false;
        }
        // 读取写文件的回调
        context->cb_wdb_sub_start = sis_message_get_method(msg, "cb_sub_start");
        context->cb_wdb_sub_stop  = sis_message_get_method(msg, "cb_sub_stop");
        context->cb_wdb_dict_sdbs = sis_message_get_method(msg, "cb_dict_sdbs");
        context->cb_wdb_dict_keys = sis_message_get_method(msg, "cb_dict_keys");
        context->cb_wdb_sub_chars = sis_message_get_method(msg, "cb_sub_chars");
        sis_message_destroy(msg);
    }

    context->flist = sis_pointer_list_create();
    context->flist->vfree = csvdb_field_destroy;

    return true;
}

void sisdb_csv2sno_uninit(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_csv2sno_cxt *context = (s_sisdb_csv2sno_cxt *)worker->context;

    sis_sdsfree(context->rpath);
    sis_sdsfree(context->rname);
    sis_sdsfree(context->wkeys);
    sis_sdsfree(context->wsdbs);
    if (context->wsno)
    {
        sis_worker_destroy(context->wsno);
    }
    if (context->flist)
    {
        sis_pointer_list_destroy(context->flist);
    }
    sis_free(context);
    worker->context = NULL;
}


///////////////////////////////////////////
//  callback define begin
///////////////////////////////////////////
static int cb_csv_read(void *source, void *argv)
{
    s_sisdb_csv2sno_cxt *context = (s_sisdb_csv2sno_cxt *)source;
	s_sis_file_csv_unit *unit = (s_sis_file_csv_unit *)argv;
	printf("===== %5d : %3d \n", unit->index, unit->cols);
	char str[100];
	for (int i = 0; i < unit->cols; i++)
	{
		// sis_out_binary("--", unit->argv[i], unit->argsize[i]);
		sis_strncpy(str, 100, unit->argv[i], unit->argsize[i]);
		sis_trim(str);
        printf("%s\n", str);
		// sis_out_binary("--", str, strlen(str));
	}
	return 0;
}



///////////////////////////////////////////
//  callback define end.
///////////////////////////////////////////
void _csv2sno_w_factor(s_sisdb_csv2sno_cxt *context, int idate)
{
    // 1. 提取 keys
    // 2. 生成 sdbs
    s_sis_dynamic_db *cdb = NULL;
    // 3. 开始写入数据

    char sdate[32];
    sis_lldtoa(idate, sdate, 32, 10);
    context->cb_wdb_sub_start(context->wsno, sdate);
    context->cb_wdb_dict_keys(context->wsno, context->wkeys);
    context->cb_wdb_dict_sdbs(context->wsno, context->wsdbs);

    s_csvdb_field *tfield = sis_pointer_list_get(context->flist, 0);
    if (tfield)
    {
        int count = csvdb_field_getsize(tfield);
        for (int k = 0; k < count; k++)
        {
            s_sis_db_chars chars;
            chars.sname = "sis_factor";
            chars.size = cdb->size;
            chars.data = sis_malloc(chars.size);
            char *data =  chars.data;
            
            msec_t mt = csvdb_field_get_int(tfield, k); 
            memmove(data, &mt, sizeof(msec_t));
            
            s_csvdb_field *kfield = sis_pointer_list_get(context->flist, 1);
            chars.kname = csvdb_field_get_string(kfield, k); 
            
            for (int i = 2; i < context->flist->count; i++)
            {
                s_csvdb_field *cfield = sis_pointer_list_get(context->flist, i);
                double fv = csvdb_field_get_double(cfield, k);
                memmove(data + sizeof(msec_t) + (i - 2) * sizeof(double), &fv, sizeof(double)); 
            }
            context->cb_wdb_sub_chars(context->wsno, &chars);
        }
    }
    context->cb_wdb_sub_stop(context->wsno, sdate);
}
void _csv2sno_start_read(s_sisdb_csv2sno_cxt *context, int idate) 
{
    char fn[255];
    sis_sprintf(fn, 255, "%s/%s.%d.csv", context->rpath, context->rname, idate);
    LOG(5)("start read csv : %s\n", fn);

    sis_pointer_list_clear(context->flist);

    sis_file_csv_read_sub(fn, ',', context, cb_csv_read);
    // 这里读取数据完毕 开始提取数据

    LOG(5)("start write sno : %d\n", idate);
    if (context->work_mode == 1)
    {
        _csv2sno_w_factor(context, idate);
    }

}

///////////////////////////////////////////
//  method define
/////////////////////////////////////////

int cmd_sisdb_csv2sno_setcb(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_csv2sno_cxt *context = (s_sisdb_csv2sno_cxt *)worker->context;
    s_sis_message *msg = (s_sis_message *)argv_; 
    {
        s_sis_sds str = sis_message_get_str(msg, "sub-keys");
        if (str)
        {
            sis_sdsfree(context->rkeys);
            context->rkeys = sis_sdsdup(str);
        }
    }
    if (sis_message_exist(msg, "sub-date"))
    {
        context->rdate = sis_message_get_int(msg, "sub-date");
    }
    else
    {
        context->rdate = sis_time_get_idate(0);
    }
    context->cb_source      = sis_message_get(msg, "cb_source");
    context->cb_sub_start   = sis_message_get_method(msg, "cb_sub_start"  );
    context->cb_sub_stop    = sis_message_get_method(msg, "cb_sub_stop"   );
    // context->cb_dict_sdbs   = sis_message_get_method(msg, "cb_dict_sdbs"  );
    // context->cb_dict_keys   = sis_message_get_method(msg, "cb_dict_keys"  );
    context->cb_sub_chars   = sis_message_get_method(msg, "cb_sub_chars"  );


    return SIS_METHOD_OK;
}

void sisdb_csv2sno_working(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_csv2sno_cxt *context = (s_sisdb_csv2sno_cxt *)worker->context;
    if (context->work_mode == 0)
    {
        return ;
    }
    int today = sis_time_get_idate(0);
    if (context->work_date.start == 0)
    {
        context->work_date.start = today;
    }
    int stop = context->work_date.stop ? context->work_date.stop : today;
    while (context->work_date.start <= stop)
    {
        msec_t start = sis_time_get_now_msec();
        LOG(5)("sub history start. [%d]\n", context->work_date.start);
        _csv2sno_start_read(context, context->work_date.start);
        LOG(5)("sub history end. [%d] cost = %lld\n", context->work_date.start, sis_time_get_now_msec() - start);
        context->work_date.start = sis_time_get_offset_day(context->work_date.start, 1);
    }
    if (context->work_date.stop > 0)
    {
        // stop 不为 0 通常表示只做一次运算
        // exit(0);  // stop 不为0 直接退出
    }
}

#if 0
// 测试 snapshot 转 新格式的例子
const char *sisdb_csv2sno = "\"sisdb_csv2sno\" : { \
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
        s_sis_json_handle *handle = sis_json_load(sisdb_csv2sno, sis_strlen(sisdb_csv2sno));
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