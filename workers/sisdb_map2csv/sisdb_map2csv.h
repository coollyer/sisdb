#ifndef _sisdb_map2csv_h
#define _sisdb_map2csv_h

#include "sis_method.h"
#include "worker.h"
#include "snodb.h"
#include "sis_db.h"
#include "sis_map.h"

// 通过stream写csv太慢 这里就是为了提高 效率和速度高
#define SIS_DBINFO_INIT  0
#define SIS_DBINFO_WORK  1

#define SIS_MAP2CSV_INIT  0
#define SIS_MAP2CSV_WORK  1
#define SIS_MAP2CSV_EXIT  2

typedef struct s_sisdb_map2csv_dbinfo
{
    s_sis_dynamic_db    *db;
    s_sis_sds            dbhead;       // csv 格式 避免重复拼接
    size_t               csize;
    s_sis_memory        *wcatch;       // 写盘缓存
    int                  status;       // 0 未打开或已关闭 1 已打开并写好头 2 正常写数据
    s_sis_sds            csvname;      // 文件名
    s_sis_file_handle    csvfp;        // csv 文件句柄
} s_sisdb_map2csv_dbinfo;

typedef struct s_sisdb_map2csv_cxt
{
    int                status;        // 工作状态
    int                curr_date; 

    s_sis_date_pair    work_date;
    s_sis_worker      *source;       // 原始行情源

    s_sis_map_list    *map_sdbs;     // 表结构 s_sisdb_map2csv_dbinfo

    s_sis_sds          wpath;        // 写入目录
    s_sis_sds          wname;        // 写入前缀

    s_sis_sds          sub_sdbs;     // 
    s_sis_sds          sub_keys;     // 

    size_t             catch_size;   // 缓存大小

}s_sisdb_map2csv_cxt;

s_sisdb_map2csv_dbinfo *sis_sisdb_map2csv_dbinfo_create(s_sis_dynamic_db *db, size_t csize);
void sis_sisdb_map2csv_dbinfo_destroy(void *);

int sis_sisdb_map2csv_dbinfo_open(s_sisdb_map2csv_dbinfo *, int wdate);

void sis_sisdb_map2csv_dbinfo_stop(s_sisdb_map2csv_dbinfo *);

int sis_sisdb_map2csv_dbinfo_write(s_sisdb_map2csv_dbinfo *, const char *key, void *inmem, size_t isize);


bool sisdb_map2csv_init(void *, void *);
void sisdb_map2csv_uninit(void *);
void sisdb_map2csv_working(void *);

int cmd_sisdb_map2csv_setcb(void *worker_, void *argv_);

void sisdb_map2csv_sub_start(s_sisdb_map2csv_cxt *context);
void sisdb_map2csv_sub_stop(s_sisdb_map2csv_cxt *context);

#endif