#ifndef _sis_disk_io_map_h
#define _sis_disk_io_map_h

#include "sis_disk.h"
#include "sis_dynamic.h"
#include "sis_utils.h"

// 映射文件 方便实时数据获取 采用固定32K大小存储 不压缩
// 作用：实时数据写入和共享 多个因子各自写入文件 其他不同用户筛选读取
//      合并所有日线数据或5分钟数据到一个大文件中 有新数据可以不断增加数据 避免小文件过多 提高访问速度
//      传递出去的数据 kname sname data 都是不会做修改的数据 避免内存数据拷贝 *** 加快速度 ***
/////////////////////////////////
//    头 + key + sdb 定义后 为数据区
//    数据区 首先为 kidx+sidx 的块信息  索引定义区
//          然后为 kidx+sidx 的数据区 每条数据需要有一个排序编号
// 顺序播报时 对所有 kidx+sidx 建立指针 最小的编号先发布 
// 特别注意： 32K满了以后 需要增加文件长度 此时需要重新 unmap mmap 一次
// 因此文件打开即保持 每次读请求需要检查文件长度
// 写入时先写数据 再加锁后写索引
// 读取索引时需要加锁 这样读取数据时就不用加锁 
// ///////////////////
// 目前版本不支持 sdb 的后期增加变更
// 现在就需要支持新增key时 需要 在 s_sis_map_ctrl 增加字段信息
//    新增的每个key 的名称 开始索引块号 数据块号

#pragma pack(push, 1)

// 首先是 SIS_DISK_HID_MAP_CTRL 块
// 然后是 SIS_DISK_HID_DICT_KEY 需要支持块存储
//       SIS_DISK_HID_DICT_SDB 需要支持块存储
//       SIS_DISK_HID_MAP_INDEX 加载全部索引信息
//       SIS_DISK_HID_MAP_BLOCK 名字 + 开始索引块号 实际开辟的块需要乘以 表数量
// 数据区根据需要进行加载
//       SIS_DISK_HID_MAP_DATA

#define  SIS_MAP_LOAD_OK     0
#define  SIS_MAP_STYLE_ERR  -1
#define  SIS_MAP_SIZE_ERR   -2
#define  SIS_MAP_DATA_ERR  -3

typedef struct s_sis_map_binfo {
    int32        blks;     // 块数
    int32        fbno;     // 第一个有效块 块号
} s_sis_map_binfo;


#define SIS_MAP_MAX_SDBNUM         1024
#define SIS_MAP_INCR_SIZE         (16*1024)  // 每次文件增长不少于500M
// 需要扩容时 最少按 1024 个块增加
#define SIS_MAP_OTHER_SIZE  (SIS_DISK_MAXLEN_MAPPAGE - 16 - (SIS_MAP_MAX_SDBNUM + 2) * 8 - 11 * 4 - 1)
#define SIS_MAP_MAYUSE_LEN  (SIS_DISK_MAXLEN_MAPPAGE - sizeof(s_sis_map_block))
// 剩余字段保证 sizeof(s_sis_disk_main_head) + sizeof(s_sis_map_ctrl) = SIS_DISK_MAXLEN_MAPPAGE
////////////////////////////////////////////////
// map 文件的控制信息 SIS_DISK_HID_MAP_CTRL
typedef struct s_sis_map_ctrl {
    SIS_DISK_BLOCK_HEAD
    int32            bsize;         // 块的尺寸
    int64            fsize;         // 当前最新map的尺寸 <= 文件长度
    int32            maxblks;       // 最大块的数量 boffset + maxblks * bsize = fsize文件长度
    int32            useblks;       // 使用块的数量 boffset + useblks * bsize = 新数据写入位置
    int64            lastsno;       // 当前最新序列号
    int64            boffset;       // 当前最新时间 毫秒 
    int32            keynums;       // key数量
    int32            sdbnums;       // sdb数量
    s_sis_map_binfo  kbinfo;        // key块信息
    s_sis_map_binfo  sbinfo;        // sdb块信息
    s_sis_map_binfo  ibinfo[SIS_MAP_MAX_SDBNUM];  // idx块信息
    char             other[SIS_MAP_OTHER_SIZE];       
    // 需要建立内存映射 方便读写
    // sdbnums keynums 增加时所有读者需要读取增加的数据
} s_sis_map_ctrl;

////////////////////////////////////////////////
//// -- 下面就是固定块的信息 /////
////////////////////////////////////////////////

// map 文件的块头信息 放在索引块和数据块头部
typedef struct s_sis_map_block {
    SIS_DISK_BLOCK_HEAD
    // int32        fbno;       // 当前块号
    int32        size;       // 数据大小
    int32        next;       // 下一个块 编号
} s_sis_map_block;

// SIS_DISK_HID_DICT_KEY 按块保存数据 新增**不改变顺序**修改后存储
// SIS_DISK_HID_DICT_SDB 按块保存数据 新增**不改变顺序**修改后存储

// 索引信息 SIS_DISK_HID_MAP_INDEX
// key索引 sdb索引 开始块号 每页记录数 总记录数 当前块记录数 最新一条sno偏移位置 数据开始偏移 最新一条数据偏移位置 块数
// 不同sdb放在不同的块里 这样保证增加ks的时候顺序不变
typedef struct s_sis_map_index {
    int32           kidx;        // key索引
    int32           sidx;        // sdb索引
    int32           perrecs;     // 每页记录数
    int32           fbno;        // 第一个有效块 块号
    // ----       
    int32           sumrecs;     // 总记录数  sumrecs / perrecs >= blocks
    int32           currecs;     // 当前块记录数
    // int64        soffset;     // 最新一条sno偏移位置 起始位置为 0
    // int64        doffset;     // 数据开始位置 soffset + perrecs * 8
    // int64        noffset;     // 最新一条数据偏移位置 doffset + （currecs - 1） * dsize
    s_sis_map_binfo vbinfo;    // 数据块信息
} s_sis_map_index;

// 数据块信息 SIS_DISK_HID_MAP_DATA
// 一条记录一条记录顺序写入
/////////////////////////////////////
// 下面是管理信息
/////////////////////////////////////
// 块管理
typedef struct s_sis_map_bctrl {
    int8               needsno;   // 是否需要顺序号
    int32              recsize;   // 记录长度
    int32              perrecs;   // 每块记录数
    int32              sumrecs;   // 总记录数
    // int32              sumblks;   // 总块数 和 ibinfo 重叠
    int32              currecs;   // 当前块记录数
    s_sis_node        *nodes;     // 块信息指针
} s_sis_map_bctrl;


typedef struct s_sis_map_kdict {
    int          index;       // 索引号
	s_sis_sds    kname;       // 名字
} s_sis_map_kdict;

typedef struct s_sis_map_sdict {
    int                 index;  // 索引号
    s_sis_dynamic_db   *table;  // 表信息 
}s_sis_map_sdict;

typedef struct s_sis_map_ksctrl {
    int64             ksidx;          // 索引
    s_sis_map_kdict  *kdict;
    s_sis_map_sdict  *sdict;
    s_sis_map_bctrl  *mapbctrl_var;   // 数据的块管理
    s_sis_rwlock_t    rwlock;         // 全局锁 
} s_sis_map_ksctrl;

#define SIS_MAP_SUB_TSNO       0    // 以 sno 排序
#define SIS_MAP_SUB_DATA       1    // 以 时间排序

#define SIS_MAP_SUB_SIZE      (32*1024)  // 以 sno 订阅时的缓存数量
#define SIS_MAP_MIN_DIFF       1000    // 以 时间排序 差额在1000毫秒

// 订阅时控制
typedef struct s_sis_map_subinfo {
    s_sis_map_ksctrl  *ksctrl;   // 单股单表的指针
    int8               nottfd;   // 无时序字段 
    int32              cursor;   // 当前记录号 0 表示第一条数据 如果实际没有数据 表示当前在等待新数据 
    int64             *sortfd;   // 当前排序字段指针 注意数据用完后 要设置为 NULL
    int64             *timefd;   // 当前数据的字段指针 
} s_sis_map_subinfo;
// 每次找里面时间最早的发布 允许误差在 1 秒 
typedef struct s_sis_map_subctrl {
    s_sis_sds             sub_keys;
    s_sis_sds             sub_sdbs;
    s_sis_msec_pair       mpair;
    s_sis_disk_reader_cb *callback;

    s_sis_pointer_list   *subvars;  // s_sis_map_subinfo 这里都是有时间字段
} s_sis_map_subctrl;
// 使用序列号订阅的临时缓存结构
typedef struct s_sis_map_subsno {
    void             *inmem;       // 索引号
    int               idate;       // 0 表示无时序数据
    s_sis_map_ksctrl *ksctrl;
} s_sis_map_subsno;


// 数据区 前部为序列号 后部为数据区 全部用指针操作
// 增加数据时需要加读写锁 
// map文件读写的控制结构
typedef struct s_sis_map_fctrl {

    s_sis_sds           fname;
  
    int                 rwmode;       // 0 读 1 写
    int                 status;   
    char               *mapmem;       // 映射内存
    s_sis_map_ctrl     *maphead;      // 头指针
    s_sis_pointer_list *mapbctrl_idx; // 索引的块管理 s_sis_map_bctrl 按表结构顺序

    s_sis_rwlock_t      rwlock;       // 全局锁 
      
    s_sis_sds           wkeys;        // keys
    s_sis_sds           wsdbs;        // sdbs
    s_sis_map_kint     *map_kscs;     // 索引信息 s_sis_map_ksctrl
    
    s_sis_map_list     *map_keys;     // 键信息 s_sis_map_kdict
    s_sis_map_list     *map_sdbs;     // 表信息 s_sis_map_sdict
    // 订阅相关信息
    int                 sub_stop;     // 0 订阅 1 停止订阅
    int                 sub_mode;     // 0 按sno订阅 1按时间字段订阅
} s_sis_map_fctrl;

#pragma pack(pop)

///////////////////////////////////////////////////////
// s_sis_map_kdict s_sis_map_sdict
///////////////////////////////////////////////////////

s_sis_map_kdict *sis_map_kdict_create(int index, const char *kname);
void sis_map_kdict_destroy(void *);

s_sis_map_sdict *sis_map_sdict_create(int index, s_sis_dynamic_db *db);
void sis_map_sdict_destroy(void *);

///////////////////////////////////////////////////////
// s_sis_map_bctrl
///////////////////////////////////////////////////////
s_sis_map_bctrl *sis_map_bctrl_create(int needsno, int recsize);
void sis_map_bctrl_destroy(void *bctrl_);
// 得到数据区的开始指针
void *sis_map_bctrl_get_var(s_sis_map_bctrl *bctrl, int recno_);
// 得到对应记录的序列号
int64 *sis_map_bctrl_get_sno(s_sis_map_bctrl *bctrl, int recno_);
// 设置对应记录的序列号 可能会锁
void sis_map_bctrl_set_sno(s_sis_map_bctrl *bctrl, int recno_, int64 sno_);

///////////////////////////////////////////////////////
// s_sis_map_ksctrl
///////////////////////////////////////////////////////

s_sis_map_ksctrl *sis_map_ksctrl_create(s_sis_map_kdict *kdict, s_sis_map_sdict *sdict);
void sis_map_ksctrl_destroy(void *);
// 得到时间字段的指针位置 是否加锁?
int64 *sis_map_ksctrl_get_timefd(s_sis_map_ksctrl *ksctrl, int recno);

///////////////////////////////////////////////////////
// s_sis_map_fctrl
///////////////////////////////////////////////////////

s_sis_map_fctrl *sis_map_fctrl_create();
void sis_map_fctrl_destroy(void *);

////////////////////
// pubic 
////////////////////
// 按块号获取头地址
s_sis_map_block *sis_map_block_head(s_sis_map_fctrl *fctrl, int blockno);
// 按块号获取数据地址
char *sis_map_block_memory(s_sis_map_fctrl *fctrl, int blockno);

// 得到记录的索引号
int64 sis_disk_io_map_get_ksidx(int32 kidx, int64 sidx);
// 增加一个key
s_sis_map_kdict *sis_disk_io_map_incr_kdict(s_sis_map_fctrl *fctrl, const char *kname);

// 这两个填补key和sdb可以增补 并可能扩展大小 ｜ 状态为新文件时 不扩容 等 start 时一次性写入
int sis_disk_io_map_set_kdict(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_);
int sis_disk_io_map_set_sdict(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_);
// 得到数据表的结构
s_sis_dynamic_db *sis_disk_io_map_get_dbinfo(s_sis_map_fctrl *fctrl, const char *sname_);
// 增加一个key后需要做的工作
int sis_disk_io_map_kdict_change(s_sis_map_fctrl *fctrl, s_sis_map_kdict *kdict);
// 增加一个sdb后需要做的工作
int sis_disk_io_map_sdict_change(s_sis_map_fctrl *fctrl, s_sis_map_sdict *sdict);
// 打开map文件并做好映射 但不读数据
int sis_disk_io_map_open(s_sis_map_fctrl *fctrl);
// 得到map的文件
s_sis_sds sis_disk_io_map_get_fname(const char *fpath_, const char *fname_);
// 得到keys
s_sis_sds sis_disk_io_map_as_keys(s_sis_map_list *map_keys_);
// 得到sdbs
s_sis_sds sis_disk_io_map_as_sdbs(s_sis_map_list *map_sdbs_);

////////////////////
// write 
////////////////////
// 这里都是牵涉到加锁数据的函数
void sis_disk_io_map_ksctrl_incr_key(s_sis_map_fctrl *fctrl, s_sis_map_kdict *kdict);
void sis_disk_io_map_ksctrl_incr_sdb(s_sis_map_fctrl *fctrl, s_sis_map_sdict *sdict);

// 把索引数据和文件做映射 到 mapbctrl_idx 中
void sis_disk_io_map_write_ksctrl(s_sis_map_fctrl *fctrl);
// 对 mapbctrl_idx 数据做初始化 后续有新增的数据 不能执行该命令 必须先做映射
void sis_disk_io_map_write_mindex(s_sis_map_fctrl *fctrl);

void sis_disk_io_map_write_kdict(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_write_sdict(s_sis_map_fctrl *fctrl);

// 初始化信息 或检查文件合法性
// 如果以前有数据 需要先把以前的数据加载到 fctrl
// 如果以前没有数据 就什么也不做
int sis_disk_io_map_w_open(s_sis_map_fctrl *fctrl, const char *fpath_, const char *fname_);
// 创建一个空的文件
int sis_disk_io_map_inited(s_sis_map_fctrl *fctrl);

// 开始写文件 此时 key 和 sdb 已经有基础数据了
// 根据文件状态 新文件时就从头写数据 文件已经存在就增量写数据
int sis_disk_io_map_w_start(s_sis_map_fctrl *fctrl);
// 不关闭文件 仅仅做数据同步 
int sis_disk_io_map_w_stop(s_sis_map_fctrl *fctrl);
// 关闭文件 释放mmap映射
int sis_disk_io_map_close(s_sis_map_fctrl *fctrl);

// 删除文件 可能被锁定 需要有延时重复机制
int sis_disk_io_map_control_remove(const char *fpath_, const char *fname_);

// 写入数据
int sis_disk_io_map_w_data(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, void *in_, size_t ilen_);

////////////////////
// read 
////////////////////
// 这里都是牵涉到加锁数据的函数

void sis_disk_io_map_read_ksctrl(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_read_mindex(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_read_kdict(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_read_sdict(s_sis_map_fctrl *fctrl);

// 对打开的文件进行必要信息预加载
int sis_disk_io_map_load(s_sis_map_fctrl *fctrl);
// 以只读方式打开文件
int sis_disk_io_map_r_open(s_sis_map_fctrl *fctrl, const char *fpath_, const char *fname_);
// 取消订阅
// map 文件的订阅必须用unsub否则会一直等待是否有新数据
void sis_disk_io_map_r_unsub(s_sis_map_fctrl *fctrl);
// 开始订阅
// 1 对于 sno 连续值排序 采用固定固定 10000 条轮询 把 10000 以内所有数据指针放入固定位置 然后pub 
//            再处理 10000- 20000 直到所有相关数据结束 或者 超过最新的 sno
// 2 对于时间字段 非连续排序 要加快速度 只能保证单股单表顺序 误差控制在 1 秒内
//            先找到相关数据最小的时间 然后轮询所有数据 误差在1秒内的全部发出 然后增加 1秒 再继续轮询 直到数据读完 或超过时间
int sis_disk_io_map_r_sub(s_sis_map_fctrl *fctrl, const char *keys_, const char *sdbs_, s_sis_msec_pair *smsec_, void *cb_);
// 得到单key单sdb的指定区间数据
s_sis_object *sis_disk_io_map_r_get_obj(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, s_sis_msec_pair *smsec_);

// 找到第一条大于开始时间的记录 无论如何都会返回 >= 0的数据
// 返回的记录号 可能没有数据
int sis_map_ksctrl_find_start_cursor(s_sis_map_ksctrl *ksctrl, s_sis_msec_pair *msec);
// 得到最后一块的数据头
s_sis_map_block *sis_map_ksctrl_last_head(s_sis_map_ksctrl *ksctrl);
////////////////////
// s_sis_map_subctrl 
////////////////////
s_sis_map_subctrl *sis_map_subctrl_create(const char *keys, const char *sdbs, s_sis_msec_pair *mpair, s_sis_disk_reader_cb *callback);
void sis_map_subctrl_destroy(s_sis_map_subctrl *subctrl);
// 得到订阅字段信息
void sis_map_subinfo_set_fd(s_sis_map_subinfo *subinfo, s_sis_map_fctrl *fctrl);
// 订阅初始化
int sis_map_subctrl_init(s_sis_map_subctrl *subctrl, s_sis_map_fctrl *fctrl);
// 按序列号订阅
int sis_map_subctrl_sub_sno(s_sis_map_subctrl *msubctrl, s_sis_map_fctrl *fctrl);
// 按时间订阅
int sis_map_subctrl_sub_data(s_sis_map_subctrl *msubctrl, s_sis_map_fctrl *fctrl);

// 订阅方法 

#endif