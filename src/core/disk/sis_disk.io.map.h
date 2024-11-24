#ifndef _sis_disk_io_map_h
#define _sis_disk_io_map_h

#include "sis_disk.h"
#include "sis_dynamic.h"
#include "sis_utils.h"
#include "sis_sem.h"

// 映射文件 方便实时数据获取 采用固定32K大小存储 不压缩
// 作用：实时数据写入和共享 多个因子各自写入文件 其他不同用户筛选读取
//      合并所有日线数据或5分钟数据到一个大文件中 有新数据可以不断增加数据 避免小文件过多 提高访问速度
//      传递出去的数据 kname sname data 都是不会做修改的数据 避免内存数据拷贝 *** 加快速度 ***
// 好处：
//      可以把需要的数据集中放到一个文件中直接获取 不用浪费时间和内存做顺序重排
// 应用场景 ：
// 实盘 行情客户层 每天一个镜像文件 方便订阅、读取单全股票信息 可新增代码
//      因子生产 回测 一个文件可以放 2021年后所有数据 最小粒度为 1分钟 可随时增加代码和表 但数据以增量有序方式写入 
//      ??? 是否以此为基础 增加 MDB 类型数据 可以去掉写入顺序 仅仅以数据中的时间字段来排序数据 并支持插入 删除 修改等功能 ???
//                  
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
// 现在就需要支持新增key时 需要 在 s_sis_map_head 增加字段信息
//    新增的每个key 的名称 开始索引块号 数据块号

#pragma pack(push, 1)

// 首先是 SIS_DISK_HID_MAP_CTRL 块
// 然后是 SIS_DISK_HID_DICT_KEY 需要支持块存储
//       SIS_DISK_HID_DICT_SDB 需要支持块存储
//       SIS_DISK_HID_MAP_INDEX 加载全部索引信息
// 数据区根据需要进行加载
//       SIS_DISK_HID_MAP_DATA


#define  SIS_MAP_STATUS_CLOSED    0x0
#define  SIS_MAP_STATUS_OPENED    0x1
#define  SIS_MAP_STATUS_FERROR    0x2  // 文件存在但无法读取
#define  SIS_MAP_STATUS_NOFILE    0x4  // 文件不存在
#define  SIS_MAP_STATUS_CHANGE    0x8  // 文件有变化

#define  SIS_MAP_OPEN_OK     0
#define  SIS_MAP_MMAP_ERR   -1
#define  SIS_MAP_SIZE_ERR   -2
#define  SIS_MAP_DATA_ERR   -3
#define  SIS_MAP_STYLE_ERR  -4
#define  SIS_MAP_NO_EXISTS  -5

// map文件最小参数 没有key 没有sdb的情况下
// #define SIS_MAP_MIN_SIZE      (1024*8) 
#define SIS_MAP_MIN_SIZE  SIS_DISK_MAXLEN_MAPPAGE

#define SIS_MAP_HEAD_INTS          64       // 最少64个4字节整数

// 没用
// #define SIS_MAP_MAX_KEYNUM         1024*64 // 支持65536个股票  
// 按表分开存储 锁文件约有1M 默认建 .mlock 目录

#define SIS_MAP_INCR_BLKS         (16*1024)  // 每次文件增长不少于500M
// 需要扩容时 最少按 1024 个块增加
#define SIS_MAP_MAYUSE_LEN  (SIS_MAP_MIN_SIZE - sizeof(s_sis_map_block))
// 
#define SIS_MAP_MAX_SDBNUM  ((SIS_MAP_MIN_SIZE - 16 - SIS_MAP_HEAD_INTS * 4) / 4)   // 支持8192个表
// 如果平均每个表因子数量为10 那么最大支持 8万个因子

////////////////////////////////////////////////
// map 文件的控制信息 SIS_DISK_HID_MAP_CTRL
typedef struct s_sis_map_head {
    SIS_DISK_BLOCK_HEAD
    char    chr[3];                         //  1 字节对齐
    // 后面跟32个整数
    int32   bsize;                          //  2 块的尺寸
    int64   fsize;                          //  4 当前最新map的尺寸 == 文件长度
    int32   maxblks;                        //    最大块的数量 maxblks * bsize = fsize文件长度
    int32   useblks;                        //    使用块的数量 useblks * bsize = 新数据写入位置
    int64   lastsno;                        //  8 当前数据最新序列号
    int32   lastseq;                        //    当前索引最新序列号 可用于确定索引写入顺序
    int32   keynums;                        //    key数量
    int32   sdbnums;                        // 11 sdb数量
    // -----                 
    int32   recfbno;                        // 12 回收块头编号 当数据块缩减时 不用的块号放在这个块里  
    int32   keyfbno;                        // 13 起始key块信息
    int32   sdbfbno;                        // 14 起始sdb块信息
    int32   other[SIS_MAP_HEAD_INTS - 14];  //    SIS_MAP_HEAD_INTS = 合计所有数量 
    int32   idxfbno[SIS_MAP_MAX_SDBNUM];    // 表的起始idx块信息     
} s_sis_map_head;

// 头关键信息长度
#define SIS_MAP_HEAD_SIZE   (14 * 4)

////////////////////////////////////////////////
//// -- 下面就是固定块的信息 /////
////////////////////////////////////////////////

// map 文件的块头信息 放在索引块和数据块头部
typedef struct s_sis_map_block {
    SIS_DISK_BLOCK_HEAD
    // int32        fbno;    // 当前块号
    int32        size;       // 数据大小
    int32        next;       // 下一个块 编号
    char         vmem[0];    // 数据区
} s_sis_map_block;

// SIS_DISK_HID_DICT_KEY 按块保存数据 新增**不改变顺序**修改后存储
// SIS_DISK_HID_DICT_SDB 按块保存数据 新增**不改变顺序**修改后存储

// 索引信息 SIS_DISK_HID_MAP_INDEX
// key索引 sdb索引 开始块号 每页记录数 总记录数 当前块记录数 最新一条sno偏移位置 数据开始偏移 最新一条数据偏移位置 块数
// 不同sdb放在不同的块里 这样保证增加ks的时候顺序不变
typedef struct s_sis_map_index {
    int32           kidx;        // key索引
    int32           sidx;        // sdb索引
    int32           makeseq;     // 该条索引写入的顺序 对应 head -> lastidx
    int32           recsize;     // 记录长度
    int32           perrecs;     // 每页记录数
    // ----       
    int32           sumrecs;     // 总记录数  sumrecs / perrecs >= blocks
    int32           currecs;     // 当前块记录数
    int32           doffset;     // 数据开始位置 soffset + perrecs * 8
    // int64        noffset;     // 最新一条数据偏移位置 doffset + （currecs - 1） * dsize
    int32           varfbno;     // 起始数据块信息
    // int32           varblks;     // 数据块数量
} s_sis_map_index;

// 数据块信息 SIS_DISK_HID_MAP_DATA
// 一条记录一条记录顺序写入
/////////////////////////////////////
// 下面是管理信息
/////////////////////////////////////

typedef struct s_sis_map_kdict {
    int          index;       // 索引号
	s_sis_sds    kname;       // 名字
} s_sis_map_kdict;

typedef struct s_sis_map_sdict {
    int                 index;     // 索引号
    s_sis_dynamic_db   *table;     // 表信息 
    s_sis_int_list     *ksiblks;   // 索引块号列表 
}s_sis_map_sdict;

// 不使用 记录锁指针 是会造成文件句柄太多
typedef struct s_sis_map_ksctrl {    
    int64              ksidx;     // 索引
    s_sis_map_kdict   *kdict;     // 
    s_sis_map_sdict   *sdict;     //

    s_sis_map_index    mindex_r;  // 当前读取的索引 写入时该值无用
    s_sis_map_index   *mindex_p;  // 索引偏移位置 *** mapmem 改变时要更新 ***
    
    s_sis_int_list    *varblks;   // 数据的块号列表 
} s_sis_map_ksctrl;

#define SIS_MAP_DELAY_MS       30

#define SIS_MAP_SUB_TSNO       0    // 以 sno 排序
#define SIS_MAP_SUB_DATA       1    // 以 时间排序

#define SIS_MAP_SUB_SIZE       1000 // (32*1024)  // 以 sno 订阅时的缓存数量
#define SIS_MAP_MIN_DIFF       1000      // 以 时间排序 差额在1000毫秒

// 订阅时控制
typedef struct s_sis_map_subinfo {
    s_sis_map_ksctrl  *ksctrl;   // 单股单表的指针
    int8               nottfd;   // 无时序字段 
    int32              cursor;   // 当前记录号 0 表示第一条数据 如果实际没有数据 表示当前在等待新数据 
    int64             *sortfd;   // 当前排序字段指针 注意数据用完后 要设置为 NULL
    int64             *timefd;   // 当前数据的时间字段指针 一定有值 
} s_sis_map_subinfo;
// 每次找里面时间最早的发布 允许误差在 1 秒 
typedef struct s_sis_map_subctrl {
    s_sis_sds             sub_keys;
    s_sis_sds             sub_sdbs;
    s_sis_msec_pair       mpair;
    s_sis_aisleep        *aisleep;
    s_sis_disk_reader_cb *callback;
    int64                 curpub_msec; // 当前发布的最新时间
    s_sis_pointer_list   *subvars;     // s_sis_map_subinfo 这里都是有时间字段
} s_sis_map_subctrl;
// 使用序列号订阅的临时缓存结构
typedef struct s_sis_map_subsno {
    void             *inmem;       // 索引号
    int               idate;        // 0 表示无时序数据
    s_sis_map_ksctrl *ksctrl;
} s_sis_map_subsno;

#define SIS_MAP_LOCK_HEAD   0
#define SIS_MAP_LOCK_KEYS   1
#define SIS_MAP_LOCK_SDBS   2
#define SIS_MAP_LOCK_MIDX   3

// 数据区 前部为序列号 后部为数据区 全部用指针操作
// 增加数据时需要加读写锁 
// map文件读写的控制结构
typedef struct s_sis_map_fctrl {

    s_sis_sds           fname;
    int                 style;     // 数据是否有序列号信息 通常有序列号信息是用于当日实盘数据
                                   // 只读时 该值从头文件获取
    int                 rwmode;
    msec_t              wmsec;     // 上次写入的时间
    int                 wnums;     // 写入计数

    int                 status;   
    
    char               *mapmem;      // 映射内存
    s_sis_map_head     *mhead_p;     // 头指针地址 *** mapmem 改变时要更新 ***

    s_sis_map_head      mhead_r;   // 备份的头指针数据
    int                 ksirecs;   // 每块数据区 有多少 s_sis_map_index 记录
    // 锁的原理
    // s_sis_map_head 写入时空间不足需扩容重新mmap+reload后再写入 读取时读取数据偏移超出当前fsize时进行重新mmap+reload(增加块读取) 
    //                其他数据 keynums sdbnums 增加后读取需要同步扩容锁的数量 并加载增加的数据 通常 单key或单sdb增加数据
    //    该锁读者不应该经常读 仅仅订阅等待时才去刷新一次数据 如果数据有增加就增补数据
    // keys sdbs 锁因为要用到size 必须加锁才能读写 读写都需要锁
    // s_sis_map_index 锁是每条记录一个锁
    //    读者开始时读一次 然后获取数据前就解锁 按初期的数量获取数据 
    //    订阅者开始时读一次 仅在订阅等待时刷新一次索引数据 为提高效率 仅仅对索引加锁 数据不加锁不可修改只能增加
    //    实际数据不需要加锁 写者必须先写数据 再加锁 再写索引 解锁 这样读者读到的一定是写完的数据 因此读数据时不用加锁
    //    切记 ** 数据块中的size有冲突 读者不要用 ** 
    ///////////////////////////////
    //  初始化 写 ｜文件不存在
    //  新加载 写 ｜打开文件
    //        读 ｜打开文件
    //  重加载 写 ｜写入时空间不足
    //        读 ｜fsize增加
    //  数增加 写 ｜
    //        读 ｜
    ///////////////////////////////
    s_sis_map_rwlock   *rwlock;   // 用一个全局锁
    // 一是为了防止文件句柄数不足, 另外不管是keys sdbs head 信息变化都会相互影响
    // 不如就设置一把锁 我们需要做的就是读时尽量快速的读取索引信息 为写多留空间

    // // 为防止文件句柄数不足 仅仅使用以下4个锁
    // // 读取实际数据时不用加锁
    // // 写数据时需要先写数据区 再加锁 再写索引 
    // // 因此读取实际数据时 最后一个块的块头 next(-1) size 不可读 使用mindex中的开始头和块数 来自由读取  
    // s_sis_map_rwlock   *headlock;     // 读写锁
    // s_sis_map_rwlock   *keyslock;     // 读写锁
    // s_sis_map_rwlock   *sdbslock;     // 读写锁
    // s_sis_map_rwlock   *midxlock;     // 读写锁 仅仅在增加数据块时加锁 读仅仅能读数据块头信息
    // 0 --> maphead
    // 1 --> keys
    // 2 --> sdbs
    // 3 --> mindex
      
    s_sis_sds           wkeys;        // keys
    s_sis_sds           wsdbs;        // sdbs
    
    s_sis_map_kints    *map_kscs;     // 索引信息 s_sis_map_ksctrl
    
    s_sis_map_list     *map_keys;     // 键信息 s_sis_map_kdict
    s_sis_map_list     *map_sdbs;     // 表信息 s_sis_map_sdict
    // 订阅相关信息
    volatile int        sub_stop;     // 0 订阅 1 停止订阅
    int                 sub_mode;     // 0 按sno订阅 1按时间字段订阅
} s_sis_map_fctrl;

#pragma pack(pop)

// 计算数据需要的块数
#define MAP_GET_BLKS(f, z) ({ int _v_ = (f)/(z); _v_ * z < f ? (_v_ + 1) : _v_; })

///////////////////////////////////////////////////////
// s_sis_map_kdict s_sis_map_sdict
///////////////////////////////////////////////////////

s_sis_map_kdict *sis_map_kdict_create(int index, const char *kname);
void sis_map_kdict_destroy(void *);

s_sis_map_kdict *sis_disk_io_map_incr_kdict(s_sis_map_fctrl *fctrl, const char *kname);

s_sis_map_sdict *sis_map_sdict_create(int index, s_sis_dynamic_db *db);
void sis_map_sdict_destroy(void *);

///////////////////////////////////////////////////////
// s_sis_map_ksctrl
///////////////////////////////////////////////////////

s_sis_map_ksctrl *sis_map_ksctrl_create(s_sis_map_kdict *kdict, s_sis_map_sdict *sdict);
void sis_map_ksctrl_destroy(void *);

// 得到数据区的开始指针
static inline void *sis_map_ksctrl_get_fbvar(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, int blkno)
{
    return (fctrl->mapmem + (int64)blkno * SIS_MAP_MIN_SIZE + ksctrl->mindex_r.doffset);
}
// 此函数得到数据区的指定记录指针 只读 不需要加读锁 默认数据已经写好 不会再修改
static inline void *sis_map_ksctrl_get_var(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, int recno)
{
    if (recno < ksctrl->mindex_r.sumrecs)
    {
        int vbidx = recno / ksctrl->mindex_r.perrecs;
        // if (blkno > ksctrl->mindex_r.sumblks)
        // {
        //     return NULL;
        // }
        int curno = recno % ksctrl->mindex_r.perrecs;
        // if (curno > ksctrl->mindex_r.currecs)
        // {
        //     return NULL;
        // }
        int blkno = sis_int_list_get(ksctrl->varblks, vbidx);
        if (blkno > 0)
        {
            char *ptr = sis_map_ksctrl_get_fbvar(fctrl, ksctrl, blkno) + curno * ksctrl->mindex_r.recsize;;
            return ptr;
        }
    }
    return NULL;
}
// 此函数只读 不需要加读锁 默认数据已经写好 不会再修改
static inline int64 *sis_map_ksctrl_get_timefd(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, int recno)
{
    if (ksctrl->sdict->table->field_time && recno < ksctrl->mindex_r.sumrecs)
    {
        char *ptr = (char *)sis_map_ksctrl_get_var(fctrl, ksctrl, recno);
        return (int64 *)(ptr + ksctrl->sdict->table->field_time->offset);
    }
    return NULL;
}

// 此函数只读 不需要加读锁 默认数据已经写好 不会再修改
static inline int64 *sis_map_ksctrl_get_sno(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, int recno)
{
    if (fctrl->style == SIS_DISK_TYPE_MSN && recno < ksctrl->mindex_r.sumrecs)
    {
        int vbidx = recno / ksctrl->mindex_r.perrecs;
        int curno = recno % ksctrl->mindex_r.perrecs;
        int blkno = sis_int_list_get(ksctrl->varblks, vbidx);
        // printf("%d %d %d | %d\n", vbidx, curno, blkno, ksctrl->varblks->count);
        if (blkno > 0)
        {
            char *ptr = fctrl->mapmem + (int64)blkno * SIS_MAP_MIN_SIZE + sizeof(s_sis_map_block) + curno * 8;
            // sis_out_binary("sno", ptr, 16);
            return (int64 *)ptr;
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////
// s_sis_map_fctrl
///////////////////////////////////////////////////////

s_sis_map_fctrl *sis_map_fctrl_create(const char *fpath, const char *name, int style);
void sis_map_fctrl_destroy(void *);

// 得到索引区的开始指针
s_sis_map_index *sis_map_fctrl_get_mindex(s_sis_map_fctrl *fctrl, s_sis_map_sdict *sdict, int recno);

////////////////////
// pubic 
////////////////////
// 按块号获取头地址
s_sis_map_block *sis_map_block_head(s_sis_map_fctrl *fctrl, int blkno);

// 得到记录的索引号
int64 sis_disk_io_map_get_ksidx(int32 kidx, int64 sidx);

// 得到数据表的结构
s_sis_dynamic_db *sis_disk_io_map_get_dbinfo(s_sis_map_fctrl *fctrl, const char *sname_);

// 得到map的文件
s_sis_sds sis_disk_io_map_get_fname(const char *fpath_, const char *fname_, int style);

// 以下这两个函数要特别注意 仅仅修改map表 不干别的
int sis_disk_io_map_set_kdict(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_);
int sis_disk_io_map_set_sdict(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_);

// 得到keys
s_sis_sds sis_disk_io_map_as_keys(s_sis_map_list *map_keys_);
// 得到sdbs
s_sis_sds sis_disk_io_map_as_sdbs(s_sis_map_list *map_sdbs_);

// 删除文件 可能被锁定 需要有延时重复机制
int sis_disk_io_map_control_remove(const char *fpath_, const char *fname_, int style);

////////////////////
// write 
////////////////////

// 初始化信息 或检查文件合法性
// 如果以前有数据 需要先把以前的数据加载到 fctrl
// 如果以前没有数据 就什么也不做
// 返回 1 表示打开成功 0 表示文件不在或者文件有错误等
int sis_disk_io_map_w_open(s_sis_map_fctrl *fctrl, const char *fpath_, const char *fname_);

// 写一个新文件相关函数
void sis_disk_io_map_fw_mhead(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_fw_kdict(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_fw_sdict(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_fw_mindex(s_sis_map_fctrl *fctrl);

// 开始写文件 此时 key 和 sdb 已经有基础数据了
// 根据文件状态 新文件时就从头写数据 文件已经存在就增量写数据
// 0 表示正常 非0 表示失败
int sis_disk_writer_map_inited(s_sis_map_fctrl *fctrl, const char *keys_, size_t klen_, const char *sdbs_, size_t slen_);

// 关闭文件 做数据同步 释放mmap映射
int sis_disk_io_map_close(s_sis_map_fctrl *fctrl);

//////////
// 这里都是牵涉到加锁数据的函数

// 只更新和地址有关的信息
// 只需修改以下内容就可以重新定位数据了
// s_sis_map_ksctrl -> mindex_p 其他都不用修改
// 需要保存 以便unmmap时使用
void sis_disk_io_map_mmap_change(s_sis_map_fctrl *fctrl);

// 增加一个新数据块 进入前已加锁
int sis_disk_io_map_get_newblk(s_sis_map_fctrl *fctrl);

// 修改数据
void sis_disk_io_map_rw_ksctrl(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_rw_mindex(s_sis_map_fctrl *fctrl);

// 修改key sdb
void sis_disk_io_map_rw_kdict(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_rw_sdict(s_sis_map_fctrl *fctrl);

// 在文件打开的时候批量增加key 需要为每一个sdb增加 mindex 数据
void sis_disk_io_map_incr_key(s_sis_map_fctrl *fctrl, int kincr);
// 在文件打开的时候批量增加sdb 需要为每一个key增加 mindex 数据
void sis_disk_io_map_incr_sdb(s_sis_map_fctrl *fctrl, int sincr);
// 增加key和sdb后需要做的工作
int sis_disk_io_map_ks_change(s_sis_map_fctrl *fctrl, int kincr, int sincr);

// 增加一个key后需要做的工作
int sis_disk_io_map_kdict_change(s_sis_map_fctrl *fctrl, s_sis_map_kdict *kdict);

// 得到最后一块的数据头
s_sis_map_block *sis_map_ksctrl_incr_bhead(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl);
// 写入数据 需要判断写入的时间是否为最新 不是就插入 会影响速度
int sis_mdb_ksctrl_incr_data(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, s_sis_map_block *curblk, char *inmem);
// 写入数据 不判断顺序 按写入顺序直接写 msn文件不能插入 只能追加
int sis_msn_ksctrl_incr_data(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, s_sis_map_block *curblk, char *inmem);

// 写入数据
int sis_disk_io_map_w_data(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, void *in_, size_t ilen_);

int sis_disk_io_map_sync_data(s_sis_map_fctrl *fctrl);
// 删除指定数据 指定时间段的数据
// 对于无时序的表 直接删除其所有数据 覆盖写入
// 传入的必须有确定的值 不做匹配 
// idate_ == -1 代表删除所有
int sis_disk_io_map_del_data(s_sis_map_fctrl *fctrl, const char *keys_, const char *sdbs_, int idate_);
// 删除某个key的所有数据
int sis_disk_io_map_del_keys(s_sis_map_fctrl *fctrl, const char *keys_);
// 删除某个sdb的所有数据
int sis_disk_io_map_del_sdbs(s_sis_map_fctrl *fctrl, const char *sdbs_);

////////////////////
// read 
////////////////////
// 这里都是牵涉到加锁数据的函数

void sis_disk_io_map_read_ksctrl(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_read_kdict(s_sis_map_fctrl *fctrl);
void sis_disk_io_map_read_sdict(s_sis_map_fctrl *fctrl);

// 对打开的文件进行必要信息预加载
int sis_disk_io_map_load(s_sis_map_fctrl *fctrl);
// 以只读方式打开文件 0 打开成功 非0 表示失败
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
s_sis_memory *sis_disk_io_map_r_get_mem(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, s_sis_msec_pair *smsec_);
// 按记录位置获取数据 支持多键值获取 增加 kname 字段
s_sis_memory *sis_disk_io_map_r_get_range_mem(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, int offset, int count);
s_sis_memory *sis_disk_io_map_r_mget_range_mem(s_sis_map_fctrl *fctrl, const char *kname_, int klen, const char *sname_, int offset, int count);

// 找到第一条大于开始时间的记录 无论如何都会返回 >= 0的数据
// 返回的记录号 可能没有数据
int sis_map_ksctrl_find_start_cursor(s_sis_map_fctrl *fctrl, s_sis_map_ksctrl *ksctrl, s_sis_msec_pair *msec);
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
// 检查是否有相关新数据 0 没有 > 0 表示有新数据
int sis_map_subctrl_check(s_sis_map_subctrl *msubctrl, s_sis_map_fctrl *fctrl);

// 订阅方法 

#endif