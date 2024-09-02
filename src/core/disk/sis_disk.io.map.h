#ifndef _sis_disk_io_map_h
#define _sis_disk_io_map_h

#include "sis_disk.h"

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

////////////////////////////////////////////////
// map 文件的控制信息 SIS_DISK_HID_MAP_CTRL
typedef struct s_sis_map_ctrl {
    int32        size;          // 数据大小
    int64        cursno;        // 当前最新序列号
    int64        boffset;       // 固定块起始偏移
    int32        bsize;         // 块的尺寸
    int32        bnums;         // 当前块的数量 boffset + bnums * bsize = fsize文件长度
    int32        sdbnums;         // sdb数量
    int32        keynums;         // key数量
    // sdbnums keynums 增加时所有读者需要读取增加的数据
} s_sis_map_ctrl;

////////////////////////////////////////////////
//// -- 下面就是固定块的信息 /////
////////////////////////////////////////////////

// map 文件的块头信息 放在索引块和数据块头部
typedef struct s_sis_map_block {
    int32        size;       // 数据大小
    int32        next;       // 下一个块 编号
} s_sis_map_block;

// SIS_DISK_HID_DICT_KEY 按块保存数据 新增**不改变顺序**修改后存储
// SIS_DISK_HID_DICT_SDB 按块保存数据 新增**不改变顺序**修改后存储

// 索引信息 SIS_DISK_HID_MAP_INDEX
// key索引 sdb索引 开始块号 每页记录数 总记录数 当前块记录数 最新一条sno偏移位置 数据开始偏移 最新一条数据偏移位置 块数
typedef struct s_sis_map_index {
    int32        kidx;        // key索引
    int16        sidx;        // sdb索引
    int32        blockno;     // SIS_DISK_HID_MAP_BLOCK 的开始编号
    int32        perrecs;     // 每页记录数
    int32        sumrecs;     // 总记录数  sumrecs / perrecs >= blocks
    // ----    
    int32        currecs;     // 当前块记录数
    int64        soffset;     // 最新一条sno偏移位置 起始位置为 0
    int64        doffset;     // 数据开始位置 soffset + perrecs * 8
    int64        noffset;     // 最新一条数据偏移位置 doffset + （currecs - 1） * dsize
    int32        blocks;      // 块数 
} s_sis_map_index;

// 块信息 SIS_DISK_HID_MAP_BLOCK
typedef struct s_sis_map_binfo {
    int32        blockno;       // 块号
    int32        curdate;       // 块数据最新日期
    // 通常比较如果查询日期小于 curdate 就必须从该块开始读数据了 直到数据日期大于结束日期
    // int32        startdate;     // 块起始日期
    // int32        stopdate;      // 块最后日期
} s_sis_map_binfo;

// 数据块信息 SIS_DISK_HID_MAP_DATA
// 一条记录一条记录顺序写入

// 数据区 前部为序列号 后部为数据区 全部用指针操作
// 增加数据时需要加读写锁 

// map文件读写的控制结构
typedef struct s_sis_map_fctrl {
    int32        size;          // 数据大小
    int64        cursno;        // 当前最新序列号
    int64        boffset;       // 固定块起始偏移
    int32        bsize;         // 块的尺寸
    int32        bnums;         // 当前块的数量 boffset + bnums * bsize = fsize文件长度
    int32        sdbnums;         // sdb数量
    int32        keynums;         // key数量
    // sdbnums keynums 增加时所有读者需要读取增加的数据
} s_sis_map_fctrl;


#pragma pack(pop)

s_sis_map_fctrl *sis_map_fctrl_create();
void sis_map_fctrl_destroy(void *);

int sis_disk_io_map_w_open(s_sis_map_fctrl *fctrl, const char *fpath_, const char *fname_, int wdate_);

// s_sis_disk_kdict *sis_disk_io_map_incr_kdict(s_sis_disk_ctrl *cls_, const char *kname_);

// 开始 此时 key 和 sdb 已经有基础数据了
int sis_disk_io_map_w_start(s_sis_map_fctrl *fctrl);

// 这两个填补key和sdb可以增补 并可能扩展大小
int sis_disk_io_map_set_kinfo(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_);
int sis_disk_io_map_set_sinfo(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_);

s_sis_dynamic_db *sis_disk_io_map_get_sinfo(s_sis_map_fctrl *fctrl, const char *sname_);

int sis_disk_io_map_w_stop(s_sis_map_fctrl *fctrl);

int sis_disk_io_map_w_close(s_sis_map_fctrl *fctrl);

int sis_disk_io_map_control_remove(const char *path_, const char *name_, int idate_);

int sis_disk_io_map_w_data(s_sis_map_fctrl *cls_, const char *kname_, const char *sname_, void *in_, size_t ilen_);

void is_disk_io_map_r_unsub(s_sis_map_fctrl *fctrl);

int sis_disk_io_map_r_sub(s_sis_map_fctrl *fctrl, const char *keys_, const char *sdbs_, int startdate_, int stopdate_);

s_sis_object *sis_disk_io_map_r_get_obj(s_sis_map_fctrl *fctrl, const char *kname_, const char *sname_, s_sis_msec_pair *smsec_);
#endif