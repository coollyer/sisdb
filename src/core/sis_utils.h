﻿
#ifndef _SIS_UTILS_H
#define _SIS_UTILS_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sis_os.h"
#include "sis_dynamic.h"
#include "sis_map.h"
#include "sis_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////
// 对 s_sis_dynamic_db 信息的提取和转换
/////////////////////////////////////////////////
// 从字符串加载结构
s_sis_dynamic_db *sis_sdbinfo_load(const char *dbname, const char *fdbstr);
// 从字符串加载指定表结构
s_sis_dynamic_db *sis_sdbinfo_parse(const char *dbname, const char *fdbstr);
// 表字段转 conf
s_sis_sds sis_sdbinfo_to_conf(s_sis_dynamic_db *db_, s_sis_sds in_);
// 克隆一个表
s_sis_dynamic_db *sis_dynamic_db_clone(s_sis_dynamic_db *db_);

// 表字段转 json 
s_sis_json_node *sis_sdbinfo_to_json(s_sis_dynamic_db *db_);
// 数据转换为array
s_sis_sds sis_sdb_to_array_sds(s_sis_dynamic_db *db_, const char *key_, void *in_, size_t ilen_); 
// 数据转换为csv
s_sis_sds sis_sdb_to_csv_sds(s_sis_dynamic_db *db_, void *in_, size_t ilen_);
// 表字段转 csv 标头
s_sis_sds sis_sdbinfo_to_csv_sds(s_sis_dynamic_db *db_);
// 数据转换为csv 单条数据 对时间特殊处理
s_sis_sds sis_fhead_to_csv_sds(s_sis_dynamic_db *db_, void *in_, size_t ilen_);
// 数据转换json 单条数据 对时间特殊处理
s_sis_sds sis_fhead_to_json_sds(s_sis_dynamic_db *db_, void *in_, size_t ilen_);
// 数据转换json node 
s_sis_json_node *sis_fhead_to_json_node(s_sis_dynamic_db *db_, void *in_, size_t ilen_);

// 直接通过配置转数据格式
s_sis_sds sis_sdb_to_array_of_conf_sds(const char *confstr_, void *in_, size_t ilen_); 

// 得到指定 字段的json格式
s_sis_json_node *sis_sdb_get_fields_of_json(s_sis_dynamic_db *sdb_, s_sis_string_list *fields_);
// 得到指定 字段的csv格式
s_sis_sds sis_sdb_get_fields_of_csv(s_sis_dynamic_db *db_, s_sis_string_list *fields_);

// 按fields_中的约定字段获取二进制数据
s_sis_sds sisdb_sdb_struct_to_sds(s_sis_dynamic_db *db_, const char *in_, size_t ilen_, s_sis_string_list *fields_);
// 数据按字段转换为 csv
s_sis_sds sis_sdb_fields_to_csv_sds(s_sis_dynamic_db *db_, void *in_, size_t ilen_, s_sis_string_list *fields_, bool isfields_);
// 数据按字段转换为 array
s_sis_sds sis_sdb_fields_to_array_sds(s_sis_dynamic_db *db_, void *in_, size_t ilen_, s_sis_string_list *fields_, bool iszip_);
// 数据按字段转换为 json
s_sis_sds sis_sdb_fields_to_json_sds(s_sis_dynamic_db *db_, void *in_, size_t ilen_, const char *key_, s_sis_string_list *fields_, bool isfields_, bool iszip_);
// 
s_sis_sds sis_sdb_fields_offset_json_sds(s_sis_dynamic_db *db_, void *in_, size_t ilen_, const char* fields, int offset_, int count_);

// [[x,y,z],....]字符串转二进制数据
s_sis_sds sis_array_to_struct_sds(s_sis_dynamic_db *db_, s_sis_sds in_);

// [[x,y,z],....]字符串转二进制数据
s_sis_sds sis_json_to_struct_sds(s_sis_dynamic_db *db_, s_sis_sds in_, s_sis_sds ago_);

// json 转字符串
s_sis_sds sis_json_to_sds(s_sis_json_node *node_, bool iszip_);

// 合并根目录到当前节点
int sis_json_merge_rpath(s_sis_json_node *node_, const char *rkey, const char *rpath);

int sis_json_replace_int(s_sis_json_node *node_, const char *rkey, int rval);
int sis_json_replace_string(s_sis_json_node *node_, const char *rkey, const char *rval);

int sis_json_supply_int(s_sis_json_node *node_, const char *rkey, int rval);
int sis_json_supply_string(s_sis_json_node *node_, const char *rkey, const char *rval);

// match_keys : * --> whole_keys
// match_keys : k,m1 | whole_keys : k1,k2,m1,m2 --> k1,k2,m1
s_sis_sds sis_match_key(s_sis_sds match_keys, s_sis_sds whole_keys);
// 对匹配的字符长度限制
s_sis_sds sis_match_key_and_len(s_sis_sds match_keys, int len, s_sis_sds whole_keys);
// 得到匹配的sdbs
// match_sdbs : * | whole_sdbs : {s1:{},s2:{},k1:{}} --> s1,s2,k1
// match_sdbs : s1,s2,s4 | whole_sdbs : {s1:{},s2:{},k1:{}} --> s1,s2
s_sis_sds sis_match_sdb(s_sis_sds match_keys, s_sis_sds whole_keys);
// 得到匹配的sdbs
// match_sdbs : * --> whole_sdbs
// match_sdbs : s1,s2 | whole_sdbs : {s1:{},s2:{},k1:{}} --> {s1:{},s2:{}}
s_sis_sds sis_match_sdb_of_sds(s_sis_sds match_sdbs, s_sis_sds  whole_sdbs);
// 得到匹配的sdbs
// whole_sdbs 是s_sis_dynamic_db结构的map表
// match_sdbs : * --> whole_sdbs of sis_sds
// match_sdbs : s1,s2 | whole_sdbs : {s1:{},s2:{},k1:{}} --> {s1:{},s2:{}}
s_sis_sds sis_match_sdb_of_map(s_sis_sds match_sdbs, s_sis_map_list *whole_sdbs);


int sis_get_map_keys(s_sis_sds keys_, s_sis_map_list *map_keys_);

// 解析 sdbs_ 并 match 后转到 map_sdbs_ 中
int sis_match_map_sdbs(const char *sdbs_, s_sis_sds match_sdbs, s_sis_map_list *map_sdbs_);
// 解析 sdbs_ 并转到 map_sdbs_ 中
int sis_get_map_sdbs(s_sis_sds sdbs_, s_sis_map_list *map_sdbs_);

s_sis_sds sis_map_as_keys(s_sis_map_list *map_keys_);

s_sis_sds sis_map_as_sdbs(s_sis_map_list *map_sdbs_);

#ifdef __cplusplus
}
#endif

#endif /* _SIS_DICT_H */
