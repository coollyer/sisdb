#ifndef _sisdb_wmdb_h
#define _sisdb_wmdb_h

#include "sis_method.h"
#include "sis_disk.h"

#define  SIS_WMAP_NONE     0 // 
#define  SIS_WMAP_INIT     1 // 是否初始化
#define  SIS_WMAP_OPEN     2 // 是否打开
#define  SIS_WMAP_HEAD     3 // 是否写了头
#define  SIS_WMAP_EXIT     4 // 退出
#define  SIS_WMAP_FAIL     5 // 打开失败

// 支持 chars 非压缩写入

typedef struct s_sisdb_wmdb_cxt
{
    int                status;
    s_sis_sds_save    *work_path;     // 可配置 也可传入
    s_sis_sds_save    *work_name;     // 可配置 也可传入
    s_sis_disk_writer *writer;        // 写盘类
	
	int                style;         // 0 msn 1 map
	int                wmode;         // 0 append 1 rewrite

	int                stop_time;     // 停止时间
	int                work_date;     // 工作日期  
	s_sis_sds          work_keys;     // 筛选后的 
	s_sis_sds          work_sdbs;     // 筛选后的

	s_sis_sds          wmap_keys;     // 外部写入的keys
	s_sis_sds          wmap_sdbs;     // 外部写入的sdbs

}s_sisdb_wmdb_cxt;

bool sisdb_wmdb_init(void *, void *);
void sisdb_wmdb_uninit(void *);

int cmd_sisdb_wmdb_getcb(void *worker_, void *argv_);

bool sisdb_wmdb_start(s_sisdb_wmdb_cxt *context);
void sisdb_wmdb_stop(s_sisdb_wmdb_cxt *context);

#endif