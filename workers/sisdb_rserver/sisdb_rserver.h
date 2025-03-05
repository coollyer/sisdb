#ifndef _sisdb_rserver_h
#define _sisdb_rserver_h

#include "sis_method.h"
#include "sis_list.lock.h"
#include "sis_net.h"
#include "sis_net.msg.h"
#include "sis_message.h"
#include "worker.h"
#include "sis_dynamic.h"
#include "sisdb_sys.h"

#define SISDB_STATUS_NONE  0
#define SISDB_STATUS_INIT  1
#define SISDB_STATUS_WORK  2
#define SISDB_STATUS_EXIT  3

// 一个通用的读取磁盘数据 通过网络回复的插件
// 仅设置数据根目录 自动判断数据类型 并初始化
// 用户只需要输入用户表的名字 就可以获取数据数据
// 对于多级数据表 sname 以 . 分隔 因此数据表目录不能带.
// 数据不压缩 全部以json + array 明文返回


// 每个用户的每次请求都建立一个工作者
typedef struct s_rserver_worker
{
    int                 cid;       // 用户编号
	int                 authed;    // 0  1 已经验证
    int                 access;    // 
    int                 stype;
	s_sis_pointer_list *workers;   // s_sis_worker
} s_rserver_worker;

typedef struct s_rserver_user
{
    char     username[128];    // 
	char     password[128];    // 
    int      access;           // 
} s_rserver_user;

typedef struct s_sisdb_rserver_cxt
{
	int                 status;       // 工作状态 0 表示没有初始化 1 表示已经初始化

	s_sis_sds           work_path;    // 系统相关信息

    s_sis_map_kint     *user_work;    // 用户工作信息 s_rserver_worker 索引为 cid
	s_sis_map_list     *user_auth;    // 用户权限列表 s_rserver_user 索引为 username

	s_sis_net_class    *socket;       // 服务监听器 s_sis_net_server
	s_sis_lock_list    *socket_recv;  // 所有收到的数据放队列中 供多线程分享任务
	s_sis_lock_reader  *reader_recv;  // 读取发送队列

}s_sisdb_rserver_cxt;

//////////////////////////////////
// s_rserver_worker
///////////////////////////////////
s_rserver_worker *rserver_worker_create(int sid);

void rserver_worker_destroy(void *);

s_sis_worker *rserver_worker_inited(s_sisdb_rserver_cxt *, s_rserver_worker *, s_sis_net_message *netmsg);
void rserver_worker_closed(s_rserver_worker *rworker, s_sis_worker *curwork);

//////////////////////////////////
// s_sisdb_rserver
///////////////////////////////////

bool  sisdb_rserver_init(void *, void *);
void  sisdb_rserver_uninit(void *);
void  sisdb_rserver_working(void *);

int cmd_sisdb_rserver_auth(void *worker_, void *argv_);
int cmd_sisdb_rserver_show(void *worker_, void *argv_);

int cmd_sisdb_rserver_sub(void *worker_, void *argv_);
int cmd_sisdb_rserver_unsub(void *worker_, void *argv_);

int cmd_sisdb_rserver_get(void *worker_, void *argv_);
// int cmd_sisdb_rserver_getdb(void *worker_, void *argv_);


#endif