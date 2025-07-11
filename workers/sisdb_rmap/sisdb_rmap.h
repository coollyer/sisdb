﻿#ifndef _sisdb_rmap_h
#define _sisdb_rmap_h

#include "sis_method.h"
#include "sis_disk.h"

// 需要支持读取的数据直接压缩好返回 根据回调函数

#define  SIS_RMAP_NONE     0 // 订阅未开始
#define  SIS_RMAP_WORK     1 // 自动运行模式
#define  SIS_RMAP_CALL     2 // 订阅运行模式
#define  SIS_RMAP_EXIT     3 // 退出
// #define  SIS_RMAP_BREAK    4 // 中断订阅模式

typedef struct s_sisdb_rmap_cxt
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

    s_sis_disk_reader *fget_reader;    // 快速读取数据的

	s_sis_sds          rmap_keys;      // 实际读取的keys 
	s_sis_sds          rmap_sdbs;      // 实际读取的sdbs
    
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

}s_sisdb_rmap_cxt;

bool sisdb_rmap_init(void *, void *);
void sisdb_rmap_uninit(void *);
void sisdb_rmap_working(void *);

int cmd_sisdb_rmap_get(void *worker_, void *argv_);
int cmd_sisdb_rmap_getdb(void *worker_, void *argv_);
int cmd_sisdb_rmap_sub(void *worker_, void *argv_);
int cmd_sisdb_rmap_bsub(void *worker_, void *argv_);
int cmd_sisdb_rmap_unsub(void *worker_, void *argv_);
int cmd_sisdb_rmap_setcb(void *worker_, void *argv_);

int cmd_sisdb_rmap_fopen(void *worker_, void *argv_);
int cmd_sisdb_rmap_fkeys(void *worker_, void *argv_);
int cmd_sisdb_rmap_fget (void *worker_, void *argv_);
int cmd_sisdb_rmap_fstop(void *worker_, void *argv_);

void sisdb_rmap_sub_start(s_sisdb_rmap_cxt *context);
void sisdb_rmap_sub_stop(s_sisdb_rmap_cxt *context);



#endif