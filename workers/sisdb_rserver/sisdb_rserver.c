#include "sis_modules.h"
#include "worker.h"
#include "server.h"
#include "sis_method.h"

#include "sis_disk.io.h"
#include "sis_message.h"
#include "sisdb_rserver.h"
#include "sisdb_rsno.h"
#include "sisdb_rmap.h"
// 需要一个直接读取csv的插件
// #include "sisdb_rcsv.h"

///////////////////////////////////////////////////
// *** s_sis_modules sis_modules_[dir name]  *** //
///////////////////////////////////////////////////

struct s_sis_method sisdb_rserver_methods[] = {
    {"auth",     cmd_sisdb_rserver_auth,    SIS_METHOD_ACCESS_READ,  NULL},   // 用户登录
    {"show",     cmd_sisdb_rserver_show,    SIS_METHOD_ACCESS_READ,  NULL},   // 显示有多少数据集

    {"sub",      cmd_sisdb_rserver_sub,     SIS_METHOD_ACCESS_READ, NULL},    // 订阅多个数据集数据 并按时序输出
    {"unsub",    cmd_sisdb_rserver_unsub,   SIS_METHOD_ACCESS_READ, NULL},    // 订阅多个数据集数据 并按时序输出

    {"get",      cmd_sisdb_rserver_get,     SIS_METHOD_ACCESS_READ, NULL},   // 打开一个数据集
    // {"getdb",    cmd_sisdb_rserver_getdb,   SIS_METHOD_ACCESS_READ, NULL},   // 打开一个数据集
};
// 共享内存数据库
s_sis_modules sis_modules_sisdb_rserver = {
    sisdb_rserver_init,
    NULL,
    sisdb_rserver_working,
    NULL,
    sisdb_rserver_uninit,
    NULL,
    NULL,
    sizeof(sisdb_rserver_methods) / sizeof(s_sis_method),
    sisdb_rserver_methods,
};

//////////////////////////////////
// s_rserver_worker
///////////////////////////////////
void rserver_task_destroy(void *task_)
{
    s_rserver_task *task = task_;
    if (task->worker) 
    {
        sis_worker_destroy(task->worker);
    }
    sis_free(task);
}

s_rserver_worker *rserver_worker_create(int sid)
{
    s_rserver_worker *rworker = SIS_MALLOC(s_rserver_worker, rworker);
    rworker->cid = sid;
    rworker->workers = sis_pointer_list_create();
    rworker->workers->vfree = rserver_task_destroy;
    LOG(0)("init : %p %p\n", rworker, rworker->workers);
    return rworker;
}

void rserver_worker_destroy(void *rworker_)
{
    s_rserver_worker *rworker = rworker_;
    sis_pointer_list_destroy(rworker->workers);
    sis_free(rworker);
}

//////////////////////////////////
// s_sisdb_rserver
///////////////////////////////////

static int  cb_reader_recv(void *worker_, s_sis_object *in_);
static void cb_socket_recv(void *worker_, s_sis_net_message *msg);

static void _cb_connect_open(void *worker_, int sid)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;
	sis_net_class_set_cb(context->socket, sid, worker, cb_socket_recv);
    LOG(5)("client connect ok. [%d]\n", sid);
}
static void _cb_connect_close(void *worker_, int sid)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;

    s_rserver_worker *rworker = sis_map_kint_get(context->user_work, sid);
    if (rworker)
    {
        s_sis_message *msg = (s_sis_message *)sis_message_create();
        for (int i = 0; i < rworker->workers->count; i++)
        {
            s_rserver_task *curtask = sis_pointer_list_get(rworker->workers, i);
            sis_worker_command(curtask->worker, "unsub", msg);
        }
        sis_message_destroy(msg);
        sis_map_kint_del(context->user_work, sid);
    }
    // 清理客户信息
    LOG(5)("client disconnect. [%d]\n", sid);	
}

bool sisdb_rserver_init(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sis_json_node *node = (s_sis_json_node *)argv_;

    s_sisdb_rserver_cxt *context = SIS_MALLOC(s_sisdb_rserver_cxt, context);
    worker->context = context;

    s_sis_json_node *authnode = sis_json_cmp_child_node(node, "auth");
    if (authnode)
    {
        context->user_auth = sis_map_list_create(sis_free_call);
        s_sis_json_node *next = sis_json_first_node(authnode);
        while(next)
        {
            s_rserver_user *userinfo = SIS_MALLOC(s_rserver_user, userinfo);
            sis_strcpy(userinfo->username, 32, sis_json_get_str(next,"username"));
            sis_strcpy(userinfo->password, 32, sis_json_get_str(next,"password"));
            sis_map_list_set(context->user_auth, userinfo->username, userinfo);
            // printf("%s %s\n", userinfo->username, userinfo->password);
            next = next->next;
        } 
    } 
    // 工作者列表
    context->user_work = sis_map_kint_create(rserver_worker_destroy);

    {
        s_sis_json_node *sonnode = sis_json_cmp_child_node(node, "work-path");
        if (sonnode)
        {
            context->work_path = sis_sdsnew(sonnode->value);
        }
        else
        {
            context->work_path = sis_sdsnew("data");
        }  
    }  

    context->socket_recv = sis_lock_list_create(16 * 1024 * 1024);
    context->reader_recv = sis_lock_reader_create(context->socket_recv, 
        SIS_UNLOCK_READER_HEAD, worker, cb_reader_recv, NULL);
    sis_lock_reader_open(context->reader_recv);

    // 记载端口配置  
    s_sis_json_node *srvnode = sis_json_cmp_child_node(node, "server");
    s_sis_url  url;
    memset(&url, 0, sizeof(s_sis_url));
    if (sis_url_load(srvnode, &url))
    {
        url.io = SIS_NET_IO_WAITCNT;
        url.role = SIS_NET_ROLE_ANSWER;
        context->socket = sis_net_class_create(&url);
        context->socket->cb_source = worker;
        context->socket->cb_connected = _cb_connect_open;
        context->socket->cb_disconnect = _cb_connect_close;
    }
    // 打开网络服务
    if (!sis_net_class_open(context->socket))
    {
        LOG(5)("open socket fail.\n");
        return false;
    }  
    context->status = SISDB_STATUS_INIT;

    return true;
}

void sisdb_rserver_working(void *worker_)
{
}

void sisdb_rserver_uninit(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;

    context->status = SISDB_STATUS_EXIT;

    if (context->socket)
    {
        sis_net_class_destroy(context->socket);
    }
	sis_lock_reader_close(context->reader_recv);
    sis_lock_list_destroy(context->socket_recv);
    
    if (context->user_work)
    {
        sis_map_kint_destroy(context->user_work);
    }
    if (context->user_auth)
    {
    	sis_map_list_destroy(context->user_auth);
    }
    sis_sdsfree(context->work_path);
    sis_free(context);
    worker->context = NULL;
}

// // 所有子数据集通过网络主动返回的数据都在这里返回
// static int cb_net_message(void *context_, void *argv_)
// {
//     s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)context_;
//     sis_net_class_send(context->socket, (s_sis_net_message *)argv_);
//     return 0;
// }

static void cb_socket_recv(void *worker_, s_sis_net_message *msg)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;
	
    // SIS_NET_SHOW_MSG("server recv:", msg);
    sis_net_message_incr(msg);
    s_sis_object *obj = sis_object_create(SIS_OBJECT_NETMSG, msg);
    sis_lock_list_push(context->socket_recv, obj);
	sis_object_destroy(obj);
	
}
void sisdb_rserver_reply_ok(s_sisdb_rserver_cxt *context, s_sis_net_message *netmsg)
{
    if (netmsg->switchs.sw_tag == 0)
    {
        netmsg->tag = SIS_NET_TAG_OK;
        netmsg->switchs.sw_tag = 1;
    }
    // sis_net_message_set_info(netmsg, NULL, 0);
    // sis_net_message_set_cmd(netmsg, NULL);
    sis_net_class_send(context->socket, netmsg);
}
void sisdb_rserver_reply_error(s_sisdb_rserver_cxt *context, s_sis_net_message *netmsg)
{
    s_sis_sds reply = sis_sdsnew("method call fail ");
    if (netmsg->cmd)
    {
        reply = sis_sdscatfmt(reply, "[%s].", netmsg->cmd);
    }
    sis_net_msg_tag_error(netmsg, reply, sis_sdslen(reply));
    sis_net_class_send(context->socket, netmsg);
    sis_sdsfree(reply);
}
void sisdb_rserver_reply_null(s_sisdb_rserver_cxt *context, s_sis_net_message *netmsg, const char *name)
{
    s_sis_sds reply = sis_sdsnew("null of ");
    if (name)
    {
        reply = sis_sdscatfmt(reply, "[%s].", name);
    }    
    sis_net_msg_tag_error(netmsg, reply, sis_sdslen(reply));
    sis_net_class_send(context->socket, netmsg);
    sis_sdsfree(reply);
}
void sisdb_rserver_reply_no_method(s_sisdb_rserver_cxt *context, s_sis_net_message *netmsg, const char *name)
{
    s_sis_sds reply = sis_sdsnew("no find method ");
    if (name)
    {
        reply = sis_sdscatfmt(reply, "[%s].", name);
    }
    sis_net_msg_tag_error(netmsg, reply, sis_sdslen(reply));
    sis_net_class_send(context->socket, netmsg);
    sis_sdsfree(reply);
}
void sisdb_rserver_reply_no_service(s_sisdb_rserver_cxt *context, s_sis_net_message *netmsg, const char *name)
{
    s_sis_sds reply = sis_sdsnew("no find service ");
    if (name)
    {
        reply = sis_sdscatfmt(reply, "[%s].", name);
    }    
    sis_net_msg_tag_error(netmsg, reply, sis_sdslen(reply));
    sis_net_class_send(context->socket, netmsg);
    sis_sdsfree(reply);
}
void sisdb_rserver_reply_no_auth(s_sisdb_rserver_cxt *context, s_sis_net_message *netmsg)
{
    // s_sis_net_message *newmsg = sis_net_message_create();
    // newmsg->cid = netmsg->cid;
    // printf("====1.1.1=====%p\n", netmsg);
    sis_net_msg_tag_error(netmsg, "no auth.", 8);
    sis_net_class_send(context->socket, netmsg);
    // sis_net_message_destroy(newmsg);
}

static int cb_reader_recv(void *worker_, s_sis_object *in_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;
    s_sis_net_message *netmsg = SIS_OBJ_NETMSG(in_);
    // sis_net_message_incr(netmsg);
    
    SIS_NET_SHOW_MSG("server recv:", netmsg);
    
    s_sis_method *method = sis_worker_get_method(worker, netmsg->cmd);
    if (!method)
    {
        sisdb_rserver_reply_no_method(context, netmsg, netmsg->cmd);
        return 0;
    }
    int iret = method->proc(worker, netmsg);

	// sis_net_message_decr(netmsg);
	return iret;
}

//////////////////////////////////

int cmd_sisdb_rserver_auth(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;
    s_sis_net_message *netmsg = (s_sis_net_message *)argv_;

    if (!context->user_auth)
    {
        s_rserver_worker *rworker = sis_map_kint_get(context->user_work, netmsg->cid);
        if (!rworker)
        {
            rworker = rserver_worker_create(netmsg->cid);
            rworker->authed = 1;
            rworker->access = SIS_ACCESS_ADMIN;
            sis_map_kint_set(context->user_work, netmsg->cid, rworker);
        }
        sis_net_message_set_info(netmsg, NULL, 0);
        sis_net_msg_tag_ok(netmsg);
    }
    else
    {
        s_sis_json_handle *handle = sis_json_load(netmsg->info, sis_sdslen(netmsg->info));
        if (!handle)
        {
            sis_net_msg_tag_error(netmsg, "auth ask error.", 15);
        }
        else
        {
            int ok = 0;
            const char *username = sis_json_get_str(handle->node, "username");
            const char *password = sis_json_get_str(handle->node, "password");
            s_rserver_user *userinfo = sis_map_list_get(context->user_auth, username);
            if (!userinfo)
            {
                ok = -1;
            }
            else if (sis_strcasecmp(userinfo->password, password))
            {
                ok = -2;
            }
            else
            {
                s_rserver_worker *rworker = rserver_worker_create(netmsg->cid);
                rworker->authed = 1;
                rworker->access = userinfo->access;
                // 这里可以记录用户信息
                sis_map_kint_set(context->user_work, netmsg->cid, rworker);
            }
            if (ok == 0)
            {
                sis_net_message_set_info(netmsg, NULL, 0);
                sis_net_msg_tag_ok(netmsg);
            }
            else
            {
                LOG(5)("auth %d %s %s %s\n", ok, username, password, userinfo ? userinfo->password : "nil");
                sis_net_msg_tag_error(netmsg, "auth fail.", 10);
            }
            sis_json_close(handle);
        }
    }
    sis_net_class_send(context->socket, netmsg);
    return SIS_METHOD_OK;
}

int cmd_sisdb_rserver_show(void *worker_, void *argv_)
{
    // s_sis_worker *worker = (s_sis_worker *)worker_; 
    // s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;
    // s_sis_net_message *netmsg = (s_sis_net_message *)argv_;
    // 这个函数用于检索当前工作目录下所有合法的数据目录 暂时不实现
    return SIS_METHOD_OK;
}

// 对每个用户单独开启用一个线程用于处理该用户的订阅数据 这是一个耗时的操作 只允许读
// 可操作多个数据集合的数据
int cmd_sisdb_rserver_sub(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;
    s_sis_net_message *netmsg = (s_sis_net_message *)argv_;

    s_rserver_worker *rworker = sis_map_kint_get(context->user_work, netmsg->cid);
    if (!rworker || !rworker->authed)
    {
        sisdb_rserver_reply_no_auth(context, netmsg);
        return SIS_METHOD_ERROR;
    }
    // 这里处理订阅服务
    s_rserver_task *curtask = rserver_worker_inited(context, rworker, netmsg);
    if (curtask)
    {
        s_sis_message *msg = sis_message_create();
        if (sis_worker_command(curtask->worker, netmsg->cmd, msg) == SIS_METHOD_OK)
        {
            // 得到数据并发送信息
        }
        sis_message_destroy(msg);
    }
    return SIS_METHOD_OK;
}
int cmd_sisdb_rserver_unsub(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;
    s_sis_net_message *netmsg = (s_sis_net_message *)argv_;

    s_rserver_worker *rworker = sis_map_kint_get(context->user_work, netmsg->cid);
    if (!rworker || !rworker->authed)
    {
        sisdb_rserver_reply_no_auth(context, netmsg);
        return SIS_METHOD_ERROR;
    }
    // 这里处理订阅服务
    s_rserver_task *curtask = rserver_worker_inited(context, rworker, netmsg);
    if (curtask)
    {
        if (sis_worker_command(curtask->worker, netmsg->cmd, NULL) == SIS_METHOD_OK)
        {
            // 得到数据并发送信息
        }
    }
    return SIS_METHOD_OK;
}

int cmd_sisdb_rserver_get(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_rserver_cxt *context = (s_sisdb_rserver_cxt *)worker->context;
    s_sis_net_message *netmsg = (s_sis_net_message *)argv_;

    s_rserver_worker *rworker = sis_map_kint_get(context->user_work, netmsg->cid);
    if (!rworker || !rworker->authed)
    {
        sisdb_rserver_reply_no_auth(context, netmsg);
        return SIS_METHOD_ERROR;
    }
    // 这里处理订阅服务
    s_rserver_task *curtask = rserver_worker_inited(context, rworker, netmsg);
    if (curtask)
    {
        // LOG(5)("init type : %p %d %s\n", rworker, curtask->stype, netmsg->subject);
        s_sis_message *msg = sis_message_create();
        if (curtask->stype == SIS_DTYPE_SNO)
        {
            char kname[128];
            char sname[128];
            sis_str_divide(netmsg->subject, '.', kname, sname);
            sis_message_set_str(msg, "sub-keys", kname, sis_strlen(kname));
            sis_message_set_str(msg, "sub-sdbs", sname, sis_strlen(sname));
            s_sis_json_handle *handle = sis_json_load(netmsg->info, sis_sdslen(netmsg->info));
            if (handle)
            {
                if (sis_json_cmp_child_node(handle->node, "sub-date")) 
                {
                    int stopdate = sis_json_get_int(handle->node, "sub-date", 0);
                    sis_message_set_int(msg, "sub-date", stopdate);
                }
                else
                {
                    int stopdate = sis_json_get_int(handle->node, "stop-date", 0);
                    sis_message_set_int(msg, "sub-date", stopdate);
                }
                sis_json_close(handle);
            }
        }
        else if (curtask->stype == SIS_DTYPE_MAP)
        {
            char kname[128];
            char sname[128];
            sis_str_divide(netmsg->subject, '.', kname, sname);
            sis_message_set_str(msg, "sub-keys", kname, sis_strlen(kname));
            sis_message_set_str(msg, "sub-sdbs", sname, sis_strlen(sname));
            s_sis_json_handle *handle = sis_json_load(netmsg->info, sis_sdslen(netmsg->info));
            if (handle)
            {
                int startdate = sis_json_get_int(handle->node, "start-date", 0);
                int stopdate = sis_json_get_int(handle->node, "stop-date", 0);
                if (sis_json_cmp_child_node(handle->node, "sub-date")) 
                {
                    startdate = sis_json_get_int(handle->node, "sub-date", 0);
                    stopdate  = startdate;
                }
                sis_message_set_int(msg, "start-date", startdate);
                sis_message_set_int(msg, "stop-date", stopdate);

                if (sis_json_cmp_child_node(handle->node, "offset")) 
                {
                    int offset = sis_json_get_int(handle->node, "offset", 0);
                    sis_message_set_int(msg, "offset", offset);
                    // offset = -1 count = 1 取最后一条记录 
                    // offset = 0 count = 1 取符合条件的第一条记录 
                    int count = sis_json_get_int(handle->node, "count", 0);
                    if (offset < 0)
                    {
                        count = count > sis_abs(offset) ? sis_abs(offset) : count; 
                    }
                    sis_message_set_int(msg, "count", count);
                    // count = 1 取一条记录 和 offset 配合使用
                }
                sis_json_close(handle);
            }
        }
        else // if (curtask->stype == SIS_DTYPE_CSV)
        {
            sis_message_set_int(msg, "sub-mode", 1);  // 获取原始数据
            sis_message_set_str(msg, "sub-keys", "*", 1);
            sis_message_set_str(msg, "sub-sdbs", netmsg->subject, sis_strlen(netmsg->subject));
            s_sis_json_handle *handle = sis_json_load(netmsg->info, sis_sdslen(netmsg->info));
            if (handle)
            {
                if (sis_json_cmp_child_node(handle->node, "sub-date")) 
                {
                    int stopdate = sis_json_get_int(handle->node, "sub-date", 0);
                    sis_message_set_int(msg, "sub-date", stopdate);
                }
                else
                {
                    int stopdate = sis_json_get_int(handle->node, "start-date", 0);
                    sis_message_set_int(msg, "sub-date", stopdate);
                }
                sis_json_close(handle);
            }
        }
        LOG(0)("closes : ==1.1== %p %p %p %p \n", rworker, rworker->workers, curtask, curtask->worker);
        // s_sis_worker *www = curtask->worker;
        int r = sis_worker_command(curtask->worker, netmsg->cmd, msg);
        // int r = sis_worker_command(curtask->worker, netmsg->cmd, msg);
        LOG(0)("closes : ==1.2== %p %p %p %p \n", rworker, rworker->workers, curtask, curtask->worker);
        if (r == SIS_METHOD_OK)
        {
            // 得到数据并发送信息
            s_sis_sds o = NULL;
            LOG(0)("closes : ==1.1.1== %p %p %p %p \n", rworker, rworker->workers, curtask, curtask->worker);
            // LOG(5)("read type : %p %d %s\n", rworker, curtask->stype, sis_message_get_str(msg, "sub-sdbs"));
            if (curtask->stype == SIS_DTYPE_CSV)
            {   
                // csv 后期统一转为二进制数据返回
                s_sis_object *obj = sis_message_get(msg, "object");
                if (obj)
                {
                    sis_net_message_set_info(netmsg, SIS_OBJ_GET_CHAR(obj), SIS_OBJ_GET_SIZE(obj));
                    sisdb_rserver_reply_ok(context, netmsg);
                }
                else
                {
                    sis_net_msg_tag_error(netmsg, "no data.", 9);
                    sis_net_class_send(context->socket, netmsg);
                }
            }
            else
            {

                s_sis_object *obj = sis_message_get(msg, "object");
                s_sis_dynamic_db *rdb = sis_message_get(msg, "dbinfo");
                o = sis_sdb_fields_to_json_sds(rdb, SIS_OBJ_GET_CHAR(obj), SIS_OBJ_GET_SIZE(obj), NULL, NULL, true, true);
                if (o)
                {
                    sis_net_message_set_pinfo(netmsg, o);
                    sisdb_rserver_reply_ok(context, netmsg);
                }
                else
                {
                    sis_net_msg_tag_error(netmsg, "no data.", 9);
                    sis_net_class_send(context->socket, netmsg);
                }
            }
        }
        else
        {
            sis_net_msg_tag_error(netmsg, "call get error.", 16);
            sis_net_class_send(context->socket, netmsg);
        }
        sis_message_destroy(msg);
        // 服务完毕 关闭服务
        rserver_worker_closed(rworker, curtask);
    }
    return SIS_METHOD_OK;
}

s_rserver_task *rserver_worker_inited(s_sisdb_rserver_cxt *context, s_rserver_worker *rworker, s_sis_net_message *netmsg)
{
    s_rserver_task *curtask = SIS_MALLOC(s_rserver_task, curtask);
    char fpath[255];
    sis_sprintf(fpath, 255, "%s/%s", context->work_path, netmsg->service);
    curtask->stype = sis_disk_check_dtype(fpath);
    
    char rpath[255];
    sis_file_getpath(netmsg->service, rpath, 255);
    char rname[64];
    sis_file_getname(netmsg->service, rname, 64);
    char cpath[255];
    if (sis_strlen(rpath) > 0)
    {
        sis_sprintf(cpath, 255, "%s/%s/", context->work_path, rpath);
    }
    else
    {
        sis_sprintf(cpath, 255, "%s/", context->work_path);
    }
    printf("===  %s | %s | %s : %d\n", fpath, rpath, rname, curtask->stype);
    
    switch (curtask->stype)
    {
    case SIS_DTYPE_SNO:
        {
            // 这里生成 rsno
            char cpara[255];
            sis_sprintf(cpara, 255, 
                "{ work-path : \"%s\", work-name : \"%s\", classname:sisdb_rsno }", 
                cpath, rname);
    
            char cname[255];
            sis_sprintf(cname, 255, "%s_%d", rname, rworker->cid);
            curtask->worker = sis_worker_create_of_conf(NULL, cname, cpara);
        }
        break;
    case SIS_DTYPE_MAP:
        {
            // 这里生成 rmap
            char cpara[255];
            sis_sprintf(cpara, 255, 
                "{ work-path : \"%s\", work-name : \"%s\", classname:sisdb_rmap }", 
                cpath, rname);
    
            char cname[255];
            sis_sprintf(cname, 255, "%s_%d", rname, rworker->cid);
            curtask->worker = sis_worker_create_of_conf(NULL, cname, cpara);
        }
        break;
    case SIS_DTYPE_CSV:
        {
            // 这里生成 rcsv
            char cpara[255];
            sis_sprintf(cpara, 255, 
                "{ work-path : \"%s\", work-name : \"%s\", classname:sisdb_rcsv }", 
                fpath, netmsg->subject);
    
            char cname[255];
            sis_sprintf(cname, 255, "%s_%d", rname, rworker->cid);
            curtask->worker = sis_worker_create_of_conf(NULL, cname, cpara);
        }
        break;
    // case SIS_DTYPE_JSON:
    default:
        {
            sisdb_rserver_reply_no_service(context, netmsg, netmsg->service);
        }
        break;
    }
    if (curtask->worker)
    {
        s_sis_method *method = sis_worker_get_method(curtask->worker, netmsg->cmd);
        if (!method)
        {
            sisdb_rserver_reply_no_method(context, netmsg, netmsg->cmd);
            rserver_task_destroy(curtask);
            return NULL;
        }
        sis_pointer_list_push(rworker->workers, curtask);
        return curtask;
    }
    rserver_task_destroy(curtask);
    return NULL;
}

void rserver_worker_closed(s_rserver_worker *rworker, s_rserver_task *curtask)
{
    if (rworker->workers)
    {
        LOG(0)("closes : %p %p %p\n", rworker, rworker->workers, curtask);
        sis_pointer_list_find_and_delete(rworker->workers, curtask);
    }
}