#ifndef _csv2db_h
#define _csv2db_h

#include "sis_method.h"
#include "worker.h"

// 直接读取csv效率低 转成sno 通过mson合并数据再使用 效率和速度高
// 之所以不直接使用sno 是因为csv或json有更好的阅读性
// 对应还有 sno 转 csv 和 json 的插件

#define  SIS_CSV2SNO_NONE     0 // 订阅未开始
#define  SIS_CSV2SNO_WORK     1 // 自动运行模式
#define  SIS_CSV2SNO_EXIT     3 // 退出

// 数据结构解析 对一天数据全部加载到内存 对数据进行解析 
// 全部加载是为了判断字段类型 和 取值范围 加载的数据 按 longlong char[64] double 保存在内存中
// 可得到最大字符串长度 非0最大整数值 最后才生成数据结构 最高支持64长度字符串
// 这样既节省了存储空间 也尽量满足了数据结构的最精确的结构
// 首行为字段名 
// 找到第一个时间字段=time 找到第一个定长字符字段=key
// 其他字段 字符串 求最大长度为字符串长度
//  若为整数 且最大值未超过3000万(日期) 用int 否则用 long
//  若为浮点数 且最大值未超过10万(6位有效) 用float 否则用 double

// 主要为因子数据服务
//  仅有一个带毫秒的时间字段 一个代码字符字段 其余都定义为 float 类型
typedef struct s_csvdb_field
{
    char    name[128]; // 字段名
    int8    style;    // 0 char 1 long 2 double
    double  minv;     // 最小长度 最小值
    double  maxv;     // 最大长度 最大值
    void   *data;     // 保存数据  s_sis_string_list 
} s_csvdb_field;

typedef struct s_csv2db_cxt
{
    int                status;                  // 工作状态
         
    int                normals;                 // 所有日期累计的 正常的记录数
    int                abnormals;               // 所有日期累计的 不正常的记录数
    
    s_sis_date_pair                work_date;   // 工作日期
    int                            work_mode;   // 0 作为插件存在 1 因子模式 时间 + 代码 + 因子集合
    
    int                            rdate;
    s_sis_sds                      rkeys;
    s_sis_sds                      rpath;       // csv文件目录
    s_sis_sds                      rname;       // csv文件名称
    /////// 
    s_sis_sds                      wsdbs;       // 自动生成的 sno 数据结构信息
    s_sis_sds                      wkeys;       // 自动生成的 key 信息
    s_sis_worker                   *wsno;       // 输出工作者
    
    s_sis_pointer_list             *flist;      // s_csvdb_field

    // 
    

    void              *cb_source;      // 要将行情发往的目的地，一般是目标工作者的对象指针
    sis_method_define *cb_sub_start;    // 一日行情订阅开始时执行的回调函数，日期必须是字符格式的日期
    sis_method_define *cb_sub_stop;     // 一日行情订阅结束时执行的回调函数，日期必须是字符格式的日期
    sis_method_define *cb_dict_sdbs;    // 表结构 json字符串 
    sis_method_define *cb_dict_keys;    // 需要发送行情的股票列表，代码串 字符串
    sis_method_define *cb_sub_chars;    // s_sis_db_chars

} s_csv2db_cxt;


s_sis_memory *sis_csv2db_as_memory(const char *rpath, const char *rname, int idate);

typedef struct s_csvdb_reader
{
    const char     *fields; // 字段名
    int             offset; 
    int             count;
    s_sis_memory   *memory;
    s_sis_map_int  *mapfields;
    //
    s_sis_int_list *fds;
    int             currec;  // 当前读取的记录数
    int             curcol;  // 当前读取的记录数
    // 存储一个位置信息 可以支持按记录获取数据
    // 具体操作是记录每条记录的开始位置 数据读取完成后 移动 memory 中的数据并修改长度 就可以了
} s_csvdb_reader;
// 带参数
s_sis_memory *sis_csv2db_read_as_memory(const char *rpath, const char *rname, int idate, 
    const char *fields, int offset, int count);

#endif