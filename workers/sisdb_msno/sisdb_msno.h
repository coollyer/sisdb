#ifndef _sisdb_msno_h
#define _sisdb_msno_h

#include "sis_method.h"
#include "sisdb_msub.h"

// 对一或多sno文件进行排序订阅输出

#define  SIS_MSNO_NONE     0 // 订阅未开始
#define  SIS_MSNO_WORK     2 // 订阅已开始
#define  SIS_MSNO_STOP     1 // 订阅未开始

// 目前仅支持无时序的 get 和单时序的订阅 订阅时要注意
// 合并多序列放到外层调用来处理 
typedef struct s_sisdb_msno_cxt
{
    int                 status;          // 工作状态
    int                 sendhead;
    int                 started;
    s_sis_mutex_t       worklock;        // 总锁 修改或读取

    s_sis_sds           work_path;       // 数据文件路径，可配置 也可传入
    int                 work_date;       // 需要发送行情的日期，例如20210923
    s_sis_sds           sub_keys;        // 需要发送行情的股票列表，以逗号分隔的字符串表示
    s_sis_sds           sub_sdbs;        // 需要发送行情的格式数据，用JSON字符串表示
 
    s_sis_map_list     *maps_keys;       // 合并后的键值对应的map
    s_sis_map_list     *maps_sdbs;       // 合并后的 表 对应的map s_sisdb_msub_db
    
    s_sis_thread       work_thread;      // 读文件时间长 需要启动一个线程处理

    s_sis_pointer_list *readers;         // 读取文件列表

    // 单一操作
    s_sis_worker       *worker;          // 单一表读取时使用
    s_sis_sds           work_keys;        // 需要发送行情的股票列表，以逗号分隔的字符串表示
    s_sis_sds           work_sdbs;        // 需要发送行情的格式数据，用JSON字符串表示

    void               *cb_source;       // 要将行情发往的目的地，一般是目标工作者的对象指针
    sis_method_define  *cb_sub_start;    // 一日行情订阅开始时执行的回调函数，日期必须是字符格式的日期
    sis_method_define  *cb_sub_stop;     // 一日行情订阅结束时执行的回调函数，日期必须是字符格式的日期
    sis_method_define  *cb_dict_sdbs;    // 表结构 json字符串 
    sis_method_define  *cb_dict_keys;    // 需要发送行情的股票列表，代码串 字符串
    sis_method_define  *cb_sub_chars;    // s_sis_db_chars

} s_sisdb_msno_cxt;

bool sisdb_msno_init(void *, void *);
void sisdb_msno_uninit(void *);

// int cmd_sisdb_msno_get(void *worker_, void *argv_);
int cmd_sisdb_msno_sub(void *worker_, void *argv_);
int cmd_sisdb_msno_unsub(void *worker_, void *argv_);

int sisdb_msno_init_sdbs(s_sisdb_msno_cxt *context, int idate);
void sisdb_msno_sub_stop(s_sisdb_msno_cxt *context);

#endif