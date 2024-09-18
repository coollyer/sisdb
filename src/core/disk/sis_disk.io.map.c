
#include "sis_disk.io.map.h"
#include "sis_dynamic.h"
#include "sis_utils.h"

// 这里是关于map的读写函数
///////////////////////////////////////////////////////
// s_sis_map_kdict s_sis_map_sdict
///////////////////////////////////////////////////////

s_sis_map_kdict *sis_map_kdict_create(int index, const char *kname)
{
    s_sis_map_kdict *kdict = SIS_MALLOC(s_sis_map_kdict, kdict);
    kdict->index = index;
    kdict->kname = sis_sdsnew(kname);
    return kdict;
}
void sis_map_kdict_destroy(void *kdict_)
{
    s_sis_map_kdict *kdict = kdict_;
    sis_sdsfree((kdict->kname));
    sis_free(kdict);
}

s_sis_map_kdict *sis_disk_io_map_incr_kdict(s_sis_map_fctrl *fctrl, const char *kname)
{
    s_sis_map_kdict *kdict = sis_map_kdict_create(sis_map_list_getsize(fctrl->map_keys), kname);
    sis_map_list_set(fctrl->map_keys, kname, kdict);
    return kdict;  
}

s_sis_map_sdict *sis_map_sdict_create(int index, s_sis_dynamic_db *db)
{
    s_sis_map_sdict *sdict = SIS_MALLOC(s_sis_map_sdict, sdict);
    sdict->index   = index;
    sdict->table   = db;
    sdict->ksiblks  = sis_int_list_create();
    return sdict;    
}
void sis_map_sdict_destroy(void *sdict_)
{
    s_sis_map_sdict *sdict = sdict_;
    sis_dynamic_db_destroy(sdict->table);
    sis_int_list_destroy(sdict->ksiblks);
    sis_free(sdict);
}

///////////////////////////////////////////////////////
// s_sis_map_ksctrl
///////////////////////////////////////////////////////

s_sis_map_ksctrl *sis_map_ksctrl_create(s_sis_map_kdict *kdict, s_sis_map_sdict *sdict)
{
    s_sis_map_ksctrl *ksctrl = SIS_MALLOC(s_sis_map_ksctrl, ksctrl);
    ksctrl->ksidx = sis_disk_io_map_get_ksidx(kdict->index, sdict->index);
    ksctrl->kdict = kdict;
    ksctrl->sdict = sdict;
    
    ksctrl->varblks = sis_int_list_create();

    return ksctrl;  
}
void sis_map_ksctrl_destroy(void *ksctrl_)
{
    s_sis_map_ksctrl *ksctrl = ksctrl_;
    sis_int_list_destroy(ksctrl->varblks);
    sis_free(ksctrl);
}

///////////////////////////////////////////////////////
// s_sis_map_fctrl
///////////////////////////////////////////////////////

s_sis_map_fctrl *sis_map_fctrl_create(const char *fpath, const char *fname)
{
    s_sis_map_fctrl *fctrl = SIS_MALLOC(s_sis_map_fctrl, fctrl);
    
    fctrl->fname = sis_disk_io_map_get_fname(fpath, fname);
    fctrl->style = SIS_DISK_TYPE_MSN;

    fctrl->status = SIS_MAP_STATUS_CLOSED;  
    fctrl->rwmode = 0; // 读  

    fctrl->map_keys = sis_map_list_create(sis_map_kdict_destroy);
    fctrl->map_sdbs = sis_map_list_create(sis_map_sdict_destroy);   
    fctrl->map_kscs = sis_map_kints_create(sis_map_ksctrl_destroy);
    
    fctrl->rwlock   = sis_map_rwlock_create(fname);

    fctrl->ksirecs = SIS_MAP_MAYUSE_LEN / sizeof(s_sis_map_index);

    return fctrl;
}
void sis_map_fctrl_destroy(void *fctrl_)
{
    s_sis_map_fctrl *fctrl = fctrl_;
    sis_map_list_destroy(fctrl->map_keys);
    sis_map_list_destroy(fctrl->map_sdbs);
    sis_map_kints_destroy(fctrl->map_kscs);
    if (fctrl->mapmem)
    {
        sis_unmmap(fctrl->mapmem, fctrl->mhead_r.fsize);
    }
    sis_map_rwlock_destroy(fctrl->rwlock);

    sis_sdsfree(fctrl->fname);
    sis_sdsfree(fctrl->wkeys);
    sis_sdsfree(fctrl->wsdbs);
    sis_free(fctrl);
}   

////////////////////
// pubic 
////////////////////
s_sis_map_index *sis_map_fctrl_get_mindex(s_sis_map_fctrl *fctrl, s_sis_map_sdict *sdict, int recno)
{
    int index = recno / fctrl->ksirecs;
    int blkno = sis_int_list_get(sdict->ksiblks, index); 
    int curno = recno % fctrl->ksirecs;
    return (s_sis_map_index *)(fctrl->mapmem + blkno * SIS_MAP_MIN_SIZE + sizeof(s_sis_map_block) + curno * sizeof(s_sis_map_index));
}

s_sis_map_block *sis_map_block_head(s_sis_map_fctrl *fctrl, int blkno)
{
    return (s_sis_map_block *)(fctrl->mapmem + blkno * SIS_MAP_MIN_SIZE);
}

int64 sis_disk_io_map_get_ksidx(int32 kidx, int64 sidx)
{
    // return (sidx << 32) | kidx;
    return sidx * 100000000 + kidx;
}

s_sis_dynamic_db *sis_disk_io_map_get_dbinfo(s_sis_map_fctrl *fctrl, const char *sname_)
{
    s_sis_map_sdict *sdict = sis_map_list_get(fctrl->map_sdbs, sname_);
    if (sdict)
    {
        return sdict->table;
    }
    return NULL;
}

s_sis_sds sis_disk_io_map_get_fname(const char *fpath_, const char *fname_)
{
    s_sis_sds fname = sis_sdsempty();
    fname = sis_sdscatfmt(fname, "%s/%s.%s", fpath_, fname_, SIS_DISK_MAP_CHAR);
    return fname; 
}

int sis_disk_io_map_set_kdict(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_)
{
    s_sis_string_list *klist = sis_string_list_create();
    sis_string_list_load(klist, in_, ilen_, ",");
    int count = sis_string_list_getsize(klist);
    int news = 0;
    for (int i = 0; i < count; i++)
    {
        const char *key = sis_string_list_get(klist, i);
        s_sis_map_kdict *kdict = sis_map_list_get(fctrl->map_keys, key);
        if (!kdict)
        {
            sis_disk_io_map_incr_kdict(fctrl, key);
            news++;
        }
    }
    sis_string_list_destroy(klist);
    return news;  
}

int sis_disk_io_map_set_sdict(s_sis_map_fctrl *fctrl, const char *in_, size_t ilen_)
{
    s_sis_json_handle *injson = sis_json_load(in_, ilen_);
    if (!injson)
    {
        return 0;
    }
    int news = 0;
    s_sis_json_node *innode = sis_json_first_node(injson->node); 
    while (innode)
    {
        if (sis_map_list_getsize(fctrl->map_sdbs) < SIS_MAP_MAX_SDBNUM)
        {
            s_sis_dynamic_db *newdb = sis_dynamic_db_create(innode);
            if (newdb)
            {
                s_sis_map_sdict *sdict = sis_map_list_get(fctrl->map_sdbs, newdb->name);
                if (!sdict)
                {
                    sdict = sis_map_sdict_create(sis_map_list_getsize(fctrl->map_sdbs), newdb);
                    sis_map_list_set(fctrl->map_sdbs, newdb->name, sdict);
                }
                else
                {
                    sis_dynamic_db_destroy(newdb);
                }
            }  
        }
        innode = sis_json_next_node(innode);
    }
    sis_json_close(injson);
    // printf("sdbs = %d \n", sis_map_list_getsize(fctrl->map_sdbs));
    return news;  
}

s_sis_sds sis_disk_io_map_as_keys(s_sis_map_list *map_keys_)
{
	int count = sis_map_list_getsize(map_keys_);
	s_sis_sds o = NULL;
	for (int i = 0; i < count; i++)
	{
		s_sis_map_kdict *kdict = sis_map_list_geti(map_keys_, i);
		if (o)
		{
			o = sis_sdscatfmt(o, ",%s", kdict->kname);
		}
		else
		{
			o = sis_sdsnew(kdict->kname);
		}
	}
	return o;
}

s_sis_sds sis_disk_io_map_as_sdbs(s_sis_map_list *map_sdbs_)
{
    s_sis_json_node *innode = sis_json_create_object();
    int count = sis_map_list_getsize(map_sdbs_);
    for (int i = 0; i < count; i++)
    {
        s_sis_map_sdict *sdict = sis_map_list_geti(map_sdbs_, i);
		sis_json_object_add_node(innode, sdict->table->name, sis_sdbinfo_to_json(sdict->table));
    }
    s_sis_sds o = sis_json_to_sds(innode, 1);
	sis_json_delete_node(innode);
	return o;
}

int sis_disk_io_map_control_remove(const char *fpath_, const char *fname_)
{
    s_sis_sds fname = sis_disk_io_map_get_fname(fpath_, fname_);
    if (sis_file_exists(fname))
    {
        sis_file_delete(fname);
        return 1;
    }
    return 0;
}

#if 0

#define FNAME  "mymap"

// 测试MAP文件读写
static int    __read_nums = 0;
static size_t __read_size = 0;
static msec_t __read_msec = 0;
static int    __write_nums = 1*1000*1000;
static size_t __write_size = 0;
static msec_t __write_msec = 0;

#pragma pack(push,1)
typedef struct s_info
{
    char name[10];
} s_info;

typedef struct s_tsec {
	msec_t time;
	int value;
} s_tsec;

typedef struct s_msec {
	msec_t time;
	int    value;
    int    value1;
} s_msec;
#pragma pack(pop)
// char *inkeys = "k1,k2,k3";
char *inkeys = "k1,k2,k3,k4,k5";
const char *insdbs = "{\"info\":{\"fields\":{\"name\":[\"C\",10]}},\
    \"stsec\":{\"fields\":{\"time\":[\"T\",8],\"value\":[\"I\",4]}},\
    \"smsec\":{\"fields\":{\"time\":[\"T\",8],\"value\":[\"I\",4],\"value1\":[\"I\",4]}}}";

static void cb_open(void *src, int tt)
{
    printf("%s : %d\n", __func__, tt);
    __read_msec = sis_time_get_now_msec();
}
static void cb_close(void *src, int tt)
{
    printf("%s : %d cost: %lld\n", __func__, tt, sis_time_get_now_msec() - __read_msec);
}
static void cb_start(void *src, int tt)
{
    printf("%s : %d cost: %lld\n", __func__, tt, sis_time_get_now_msec() - __read_msec);
}
static void cb_stop(void *src, int tt)
{
    printf("%s : %d cost: %lld\n", __func__, tt, sis_time_get_now_msec() - __read_msec);
}
static void cb_key(void *src, void *key_, size_t size) 
{
    __read_size += size;
    s_sis_sds info = sis_sdsnewlen((char *)key_, size);
    printf("%s %d : %s\n", __func__, (int)size, info);
    sis_sdsfree(info);
}
static void cb_sdb(void *src, void *sdb_, size_t size)  
{
    __read_size += size;
    s_sis_sds info = sis_sdsnewlen((char *)sdb_, size);
    printf("%s %d : %s\n", __func__, (int)size, info);
    sis_sdsfree(info);
}
static void cb_break(void *src, int tt)
{
    printf("%s : %d\n", __func__, tt);
}

static void cb_chardata(void *src, const char *kname_, const char *sname_, void *out_, size_t olen_)
{
    __read_nums++;
    // if (__read_nums % sis_max((__write_nums / 10), 1) == 0 || __read_nums < 10)
    {
        printf("%s : %s.%s %zu %d\n", __func__, kname_, sname_, olen_, __read_nums);
        if (sname_[1] == 'n')
        {
            s_info *mmm = out_;
            printf("%s\n", mmm->name);
        }
        if (sname_[1] == 't')
        {
            s_tsec *mmm = out_;
            printf("%lld %d\n", mmm->time, mmm->value);
        }
        if (sname_[1] == 'm')
        {
            s_msec *mmm = out_;
            printf("%lld %d %d\n", mmm->time, mmm->value, mmm->value1);
        }
        // sis_out_binary("::", out_, olen_);
    }
    // if (__read_nums == 300000)
    // {
    //     sis_disk_reader_unsub((s_sis_disk_reader *)src);
    // }
}

void read_sdb(s_sis_disk_reader *cxt)
{
    // sis_disk_reader_open(cxt);
    // // s_sis_msec_pair pair = {1620793985999, 1620793985999};
    // s_sis_msec_pair pair = {1620793979000, 1620795500999};
    // s_sis_object *obj = sis_disk_reader_get_obj(cxt, "k2", "sminu", &pair);
    // // s_sis_object *obj = sis_disk_reader_get_obj(cxt, "k3", "smsec", &pair);
    // if (obj)
    // {
    //     sis_out_binary("get", SIS_OBJ_GET_CHAR(obj), SIS_OBJ_GET_SIZE(obj));
    //     sis_object_destroy(obj);
    // }
    // sis_disk_reader_close(cxt);

    // sis_disk_reader_open(cxt);
    // s_sis_object *obj = sis_disk_reader_get_one(cxt, "k5");
    // if (obj)
    // {
    //     sis_out_binary(".out.",SIS_OBJ_GET_CHAR(obj), SIS_OBJ_GET_SIZE(obj));
    //     sis_object_destroy(obj);
    // }
    // s_sis_msec_pair pair = {1511925510999, 1621315511000};
    // s_sis_msec_pair pair = {1511925510999, 1620795500999};
    // sis_disk_reader_sub_sdb(cxt, "*", "*", &pair);

    int submode = 1;
    // {
    //     s_sis_msec_pair pair = {1620793985999, 1711925510999};
    //     sis_disk_reader_sub_map(cxt, submode, "*", "*", &pair);
    //     printf("===============================\n");
    // }
    // {
    //     s_sis_msec_pair pair = {1620793985999, 1711925510999};
    //     sis_disk_reader_sub_map(cxt, submode, "k1,k2", "*", &pair);
    //     printf("===============================\n");
    // }
    // {
    //     s_sis_msec_pair pair = {1620793985999, 1711925510999};
    //     sis_disk_reader_sub_map(cxt, submode, "*", "info,smsec", &pair);
    //     printf("===============================\n");
    // }

    {
        // s_sis_msec_pair pair = {1620794000999, 1620795510999};
        // sis_disk_reader_sub_map(cxt, SIS_MAP_SUB_TSNO, "*", "*", &pair);
        // printf("===============================\n");
    }



    {
        s_sis_msec_pair pair = {1621315510999, 0};
        sis_disk_reader_sub_map(cxt, 1, "*", "*", &pair);
    }

    // sis_disk_reader_close(cxt);

}


void write_sdb(s_sis_disk_writer *cxt)
{
    s_info info_data[3] = { 
        { "k10000" },
        { "k20000" },
        { "k30000" },
    };
    s_msec snap_data[10] = { 
        { 1620793979000,  1 , 3},
        { 1620793979500,  2 , 3},
        { 1620793979999,  3 , 3},
        { 1620793985999,  4 , 3},
        { 1620794000999,  5 , 3},
        { 1620795500999,  6 , 3},
        { 1620795510999,  7 , 3},
        { 1620905510999,  8 , 3},
        { 1621315510999,  9 , 3},
        { 1711925510999,  10, 3},
    };

    if (sis_disk_writer_open(cxt, 0) == 0)
    {
        return ;
    }

    sis_disk_writer_inited(cxt, inkeys, sis_strlen(inkeys), insdbs, sis_strlen(insdbs));

    int count = __write_nums;
    __write_msec = sis_time_get_now_msec();

    for (int i = 0; i < 10; i++)
    {
        s_tsec data;
        data.time = 1711925520999;
        data.value = i;
        __write_size += sis_disk_writer_data(cxt, "k4", "stsec", &data, sizeof(s_tsec));
    }

    // __write_size += sis_disk_writer_data(cxt, "k1", "info", &info_data[0], sizeof(s_info));
    // __write_size += sis_disk_writer_data(cxt, "k2", "info", &info_data[1], sizeof(s_info));
    // __write_size += sis_disk_writer_data(cxt, "k3", "info", &info_data[2], sizeof(s_info));
    // // for (int k = 0; k < count; k++)
    // {
    //     for (int i = 0; i < 10; i++) // 4
    //     {
    //         s_tsec data;
    //         data.time = snap_data[i].time;
    //         for (int k = 0; k < count; k++)
    //         {
    //             data.value = k * 10 + i;
    //             if (i > 5) {
    //                 __write_size += sis_disk_writer_data(cxt, "k4", "stsec", &data, sizeof(s_tsec));
    //             }
    //             if (i < 8) 
    //             {
    //                 __write_size += sis_disk_writer_data(cxt, "k2", "stsec", &data, sizeof(s_tsec));
    //             }
    //             if (i < 5) 
    //             {
    //                 __write_size += sis_disk_writer_data(cxt, "k3", "smsec", &snap_data[i], sizeof(s_msec));
    //             }
    //         }
    //     }
    // }
    sis_disk_writer_close(cxt);
    printf("write end %d %zu | cost: %lld.\n", __write_nums, __write_size, sis_time_get_now_msec() - __write_msec);
}
int main(int argc, char **argv)
{
    if (argc > 1 && argv[1][0] == 'f')
    {
        sis_map_rwlock_clear(".mymap.sem", 0);
        return 0;
    }
    safe_memory_start();
    // s_sis_map_fctrl *fctrl = sis_map_fctrl_create("./", FNAME);
    
    if (argc < 1)
    {
        exit(0);
    }
    if (argv[1][0] == 'r')
    {
        s_sis_disk_reader_cb cb = {0};
        cb.cb_source = NULL;
        cb.cb_open = cb_open;
        cb.cb_close = cb_close;
        cb.cb_start = cb_start;
        cb.cb_stop = cb_stop;
        cb.cb_dict_keys = cb_key;
        cb.cb_dict_sdbs = cb_sdb;
        cb.cb_break = cb_break;
        cb.cb_chardata  = cb_chardata;
        s_sis_disk_reader *rcxt = sis_disk_reader_create(".", FNAME, SIS_DISK_TYPE_MSN, &cb);
        cb.cb_source = rcxt;
        read_sdb(rcxt);
        sis_disk_reader_destroy(rcxt);
    }
    if (argv[1][0] == 'a')
    {
        // 新增数据
    }
    if (argv[1][0] == 'w')
    {
        // 新写数据
        s_sis_disk_writer *wcxt = sis_disk_writer_create(".", FNAME, SIS_DISK_TYPE_MSN);
        write_sdb(wcxt);
        // write_sdb_msec(wcxt);  // 按时间写入
        sis_disk_writer_destroy(wcxt);
    }
    // sis_map_fctrl_destroy(fctrl);
    safe_memory_stop();
    return 0;
}
#endif