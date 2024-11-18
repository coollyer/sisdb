#ifndef _sisdb_rcsv_h
#define _sisdb_rcsv_h

#include "sis_method.h"
#include "sis_disk.h"

// 需要支持读取的数据直接压缩好返回 根据回调函数

#define  SIS_RCSV_NONE     0 // 订阅未开始
#define  SIS_RCSV_WORK     1 // 自动运行模式
#define  SIS_RCSV_CALL     2 // 订阅运行模式
#define  SIS_RCSV_EXIT     3 // 退出
// #define  SIS_RCSV_BREAK    4 // 中断订阅模式

typedef struct s_sisdb_rcsv_cxt
{
    int                status;         // 工作状态
    int                work_type; 
    int                work_mode; 

    s_sis_sds_save    *work_path;      // 数据文件路径，可配置 也可传入
    s_sis_sds_save    *work_name;      // 工作者名称，可配置 也可传入

    s_sis_date_pair    work_date;       // 需要发送行情的日期，例如20210923

    s_sis_sds          work_keys;       // 需要发送行情的股票列表，以逗号分隔的字符串表示
    s_sis_sds          work_sdbs;       // 需要发送行情的格式数据，用JSON字符串表示
    s_sis_disk_reader *work_reader;

	s_sis_sds          rcsv_keys;      // 实际读取的keys 
	s_sis_sds          rcsv_sdbs;      // 实际读取的sdbs
    
    s_sis_thread       work_thread;    // 读文件时间长 需要启动一个线程处理

    void              *cb_source;       // 要将行情发往的目的地，一般是目标工作者的对象指针
    sis_method_define *cb_sub_open;     // 所有行情订阅开始时执行的回调函数，日期必须是字符格式的日期
    sis_method_define *cb_sub_close;    // 所有行情订阅结束时执行的回调函数，日期必须是字符格式的日期
    sis_method_define *cb_sub_start;    // 一日行情订阅开始时执行的回调函数，日期必须是字符格式的日期
    sis_method_define *cb_sub_stop;     // 一日行情订阅结束时执行的回调函数，日期必须是字符格式的日期
    sis_method_define *cb_dict_sdbs;    // 表结构 json字符串 
    sis_method_define *cb_dict_keys;    // 需要发送行情的股票列表，代码串 字符串
    sis_method_define *cb_sub_chars;    // s_sis_db_chars
    // sis_method_define *cb_sub_bytes;    // s_sis_db_bytes
    // 因为支持多日订阅时 可能代码有变化 使用kidx相对复杂 所以禁用 cb_sub_bytes

}s_sisdb_rcsv_cxt;

bool sisdb_rcsv_init(void *, void *);
void sisdb_rcsv_uninit(void *);
void sisdb_rcsv_working(void *);

int cmd_sisdb_rcsv_get(void *worker_, void *argv_);
int cmd_sisdb_rcsv_getdb(void *worker_, void *argv_);
int cmd_sisdb_rcsv_sub(void *worker_, void *argv_);
int cmd_sisdb_rcsv_unsub(void *worker_, void *argv_);
int cmd_sisdb_rcsv_setcb(void *worker_, void *argv_);

void sisdb_rcsv_sub_start(s_sisdb_rcsv_cxt *context);
void sisdb_rcsv_sub_stop(s_sisdb_rcsv_cxt *context);

#endif