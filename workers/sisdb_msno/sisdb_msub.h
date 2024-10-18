#ifndef _sisdb_msub_h
#define _sisdb_msub_h

#include "sis_db.h"
#include "worker.h"

#define  SIS_READER_INIT     1 // 打开订阅模式
#define  SIS_READER_OPEN     2 // 开始订阅模式
#define  SIS_READER_STOP     3 // 停止订阅模式

typedef struct s_sis_msub_v
{
	uint32     kidx;
	uint16     sidx;
	uint16     size;
	char       data[0];   // 数据区
} s_sis_msub_v;

typedef struct s_sisdb_msub_reader
{
    char               cname[255];
    void              *father;
    int8               status;    // 本次订阅状态
    int8               valid;     // 本次订阅是否有效 0 
    int8               stoped;    // 数据已经读取完毕
    
    msec_t             nowmsec;     // 最新数据的时间 0表示没有数据但未结束 1表示结束且没有数据 
    int                maxsize;     // 最大结构体的长度
    s_sis_node_list   *datas;       // s_sis_msub_v
    s_sis_worker      *worker;
} s_sisdb_msub_reader;

typedef struct s_sisdb_msub_db
{
    int                ridx;          // 在reader中的索引
    int                sidx;          // 在map中的索引
    char               mname[255];    // 联合表名
    s_sis_dynamic_db  *rdb;
} s_sisdb_msub_db;

s_sisdb_msub_reader *sisdb_msub_reader_create(void *father, s_sis_json_node *node);
void sisdb_msub_reader_destroy(void *reader_);

void sisdb_msub_reader_init(s_sisdb_msub_reader *reader, int maxsize, int pagesize);
int  sisdb_msub_reader_push(s_sisdb_msub_reader *reader, int kidx, int sidx, void *in, size_t isize);

void *sisdb_msub_reader_pop(s_sisdb_msub_reader *reader);
void *sisdb_msub_reader_first(s_sisdb_msub_reader *reader);

///////////////////////////////////////////
//  s_sisdb_msub_db
///////////////////////////////////////////

s_sisdb_msub_db *sisdb_msub_db_create();
void sisdb_msub_db_destroy(void *mdb_);

#endif