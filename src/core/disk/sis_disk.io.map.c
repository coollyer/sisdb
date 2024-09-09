
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

s_sis_map_sdict *sis_map_sdict_create(int index, s_sis_dynamic_db *db)
{
    s_sis_map_sdict *sdict = SIS_MALLOC(s_sis_map_sdict, sdict);
    sdict->index = index;
    sdict->table = db;
    return sdict;    
}
void sis_map_sdict_destroy(void *sdict_)
{
    s_sis_map_sdict *sdict = sdict_;
    sis_dynamic_db_destroy(sdict->table);
    sis_free(sdict);
}
///////////////////////////////////////////////////////
// s_sis_map_bctrl
///////////////////////////////////////////////////////

s_sis_map_bctrl *sis_map_bctrl_create(int needsno, int recsize)
{
    s_sis_map_bctrl *bctrl = SIS_MALLOC(s_sis_map_bctrl, bctrl);
    bctrl->needsno = needsno;
    bctrl->recsize = recsize;
    int rsize = bctrl->needsno == 0 ? bctrl->recsize : bctrl->recsize + 8;
    bctrl->perrecs = SIS_MAP_MAYUSE_LEN / rsize;
    bctrl->sumrecs = 0;
    bctrl->currecs = 0;
    bctrl->nodes   = sis_node_create();
    return bctrl;
}
void sis_map_bctrl_destroy(void *bctrl_)
{
    s_sis_map_bctrl *bctrl = bctrl_;
    sis_node_destroy(bctrl->nodes);
    sis_free(bctrl);
}
void *sis_map_bctrl_get_var(s_sis_map_bctrl *bctrl, int recno_)
{
    int inode = recno_ / bctrl->perrecs;
    int recno = recno_ % bctrl->perrecs;
    int offset = sizeof(s_sis_map_block);
    if (bctrl->needsno)
    {
        offset += bctrl->perrecs * 8;
    }
    s_sis_node *next = sis_node_get(bctrl->nodes, inode);
    if (next)
    {
        char *ptr = next->value;
        return ptr + offset + recno * bctrl->recsize;
    }
    return NULL;
}
int64 *sis_map_bctrl_get_sno(s_sis_map_bctrl *bctrl, int recno_)
{
    if (bctrl->needsno && recno_ < bctrl->sumrecs)
    {
        int inode = recno_ / bctrl->perrecs;
        int recno = recno_ % bctrl->perrecs;
        s_sis_node *next = sis_node_get(bctrl->nodes, inode);
        if (next)
        {
            // char *ptr = next->value;
            // sis_out_binary("sno", ptr, 128);
            // printf(" getsno: %d %p %d\n", recno, ptr, (sizeof(s_sis_map_block) + recno * 8));
            // ptr = ptr + (sizeof(s_sis_map_block) + recno * 8);
            // printf(" getsno: %d %p\n", recno, ptr);
            return (int64 *)((char *)next->value + sizeof(s_sis_map_block) + recno * 8);
        }
    }
    return NULL;
}
void sis_map_bctrl_set_sno(s_sis_map_bctrl *bctrl, int recno_, int64 sno_)
{
    if (bctrl->needsno && recno_ < bctrl->sumrecs)
    {
        int inode = recno_ / bctrl->perrecs;
        int recno = recno_ % bctrl->perrecs;
        s_sis_node *next = sis_node_get(bctrl->nodes, inode);
        if (next)
        {
            int64 *var = (int64 *)((char *)next->value + sizeof(s_sis_map_block) + recno * 8);
            *var = sno_;
        }
    }
}

s_sis_map_ksctrl *sis_map_ksctrl_create(s_sis_map_kdict *kdict, s_sis_map_sdict *sdict)
{
    s_sis_map_ksctrl *ksctrl = SIS_MALLOC(s_sis_map_ksctrl, ksctrl);
    ksctrl->kdict = kdict;
    ksctrl->sdict = sdict;
    ksctrl->ksidx = sis_disk_io_map_get_ksidx(kdict->index, sdict->index);
    ksctrl->mapbctrl_var = sis_map_bctrl_create(1, sdict->table->size);
    return ksctrl;  
}
void sis_map_ksctrl_destroy(void *ksctrl_)
{
    s_sis_map_ksctrl *ksctrl = ksctrl_;
    sis_map_bctrl_destroy(ksctrl->mapbctrl_var);
    sis_free(ksctrl);
}
int64 *sis_map_ksctrl_get_timefd(s_sis_map_ksctrl *ksctrl, int recno)
{
    if (recno < ksctrl->mapbctrl_var->sumrecs)
    {
        char *var = sis_map_bctrl_get_var(ksctrl->mapbctrl_var, recno);
        return (int64 *)(var + ksctrl->sdict->table->field_time->offset);
    }
    return NULL;
}
///////////////////////////////////////////////////////
// s_sis_map_fctrl
///////////////////////////////////////////////////////

s_sis_map_fctrl *sis_map_fctrl_create(const char *name)
{
    s_sis_map_fctrl *fctrl = SIS_MALLOC(s_sis_map_fctrl, fctrl);
    
    fctrl->status = SIS_DISK_STATUS_CLOSED;  
    fctrl->rwmode = 0; // 读  

    fctrl->map_keys = sis_map_list_create(sis_map_kdict_destroy);
    fctrl->map_sdbs = sis_map_list_create(sis_map_sdict_destroy);   
    fctrl->map_kscs = sis_map_kint_create(sis_map_ksctrl_destroy);
    
    fctrl->mapbctrl_idx = sis_pointer_list_create();
    fctrl->mapbctrl_idx->vfree = sis_map_bctrl_destroy;

    fctrl->rwlocks = sis_map_rwlocks_create(name, SIS_MAP_LOCK_MIDX);
    return fctrl;
}
void sis_map_fctrl_destroy(void *fctrl_)
{
    s_sis_map_fctrl *fctrl = fctrl_;
    sis_map_list_destroy(fctrl->map_keys);
    sis_map_list_destroy(fctrl->map_sdbs);
    sis_map_kint_destroy(fctrl->map_kscs);
    sis_pointer_list_destroy(fctrl->mapbctrl_idx);
    if (fctrl->mapmem)
    {
        sis_unmmap(fctrl->mapmem, fctrl->maphead->fsize);
    }
    sis_map_rwlocks_destroy(fctrl->rwlocks);
    sis_sdsfree(fctrl->fname);
    sis_sdsfree(fctrl->wkeys);
    sis_sdsfree(fctrl->wsdbs);
    sis_free(fctrl);
}   


////////////////////
// pubic 
////////////////////
s_sis_map_block *sis_map_block_head(s_sis_map_fctrl *fctrl, int blockno)
{
    return (s_sis_map_block *)(fctrl->mapmem + (blockno + 1) * SIS_DISK_MAXLEN_MAPPAGE);
}
char *sis_map_block_memory(s_sis_map_fctrl *fctrl, int blockno)
{
    return (fctrl->mapmem + (blockno + 1) * SIS_DISK_MAXLEN_MAPPAGE + sizeof(s_sis_map_block));
}
int64 sis_disk_io_map_get_ksidx(int32 kidx, int64 sidx)
{
    // return (sidx << 32) | kidx;
    return sidx * 100000000 + kidx;
}

s_sis_map_kdict *sis_disk_io_map_incr_kdict(s_sis_map_fctrl *fctrl, const char *kname)
{
    s_sis_map_kdict *kdict = sis_map_kdict_create(sis_map_list_getsize(fctrl->map_keys), kname);
    sis_map_list_set(fctrl->map_keys, kname, kdict);
    sis_disk_io_map_kdict_change(fctrl, kdict);
    return kdict;  
}
// 这两个填补key和sdb可以增补 并可能扩展大小
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
    printf("%s\n", in_);
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
                    sis_disk_io_map_sdict_change(fctrl, sdict);
                    news++;
                }
            }  
        }
        innode = sis_json_next_node(innode);
    }
    sis_json_close(injson);
    printf("sdbs = %d \n", sis_map_list_getsize(fctrl->map_sdbs));

    return news;  
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


int sis_disk_io_map_kdict_change(s_sis_map_fctrl *fctrl, s_sis_map_kdict *kdict)
{
    if (fctrl->rwmode == 0)
    {
        return 0;
    }
    if (fctrl->status & SIS_DISK_STATUS_OPENED)
    {
        // 如果文件已经开始写入 需要增加对应信息
        sis_sdsfree(fctrl->wkeys);
        fctrl->wkeys = sis_disk_io_map_as_keys(fctrl->map_keys);
        sis_disk_io_map_write_kdict(fctrl);
        // 这里需要在每个sdb中增加索引和数据块
        sis_disk_io_map_ksctrl_incr_key(fctrl, kdict);
        return 1;
    }
    return 0;
}
int sis_disk_io_map_sdict_change(s_sis_map_fctrl *fctrl, s_sis_map_sdict *sdict)
{
    if (fctrl->rwmode == 0)
    {
        return 0;
    }
    if (fctrl->status & SIS_DISK_STATUS_OPENED)
    {
        // 如果文件已经开始写入 需要增加对应信息
        sis_sdsfree(fctrl->wsdbs);
        fctrl->wsdbs = sis_disk_io_map_as_sdbs(fctrl->map_sdbs);
        sis_disk_io_map_write_sdict(fctrl);
        // 这里需要增加所有股票的sdb中索引和数据块
        sis_disk_io_map_ksctrl_incr_sdb(fctrl, sdict);
        return 1;
    }
    return 0;
}

int sis_disk_io_map_open(s_sis_map_fctrl *fctrl)
{
    // 加载头和索引数据
    s_sis_handle fd = -1;
    if (fctrl->rwmode == 1)
    {
        fd = sis_open(fctrl->fname, SIS_FILE_IO_RDWR | SIS_FILE_IO_CREATE, 0666);
    }
    else
    {
        fd = sis_open(fctrl->fname, SIS_FILE_IO_RDWR, 0666);
    }
    if (fd == -1)
    {
        LOG(5)("error opening file. %s\n", fctrl->fname);
        return SIS_DISK_CMD_NO_OPEN;
    }
    size_t fsize = sis_size(fd);
    if (fsize <= SIS_DISK_MAXLEN_MAPPAGE)
    {
        LOG(5)("file format error. %s %zu\n", fctrl->fname, fsize);
        sis_close(fd);
        return SIS_DISK_CMD_SIZENO;
    }
    sis_seek(fd, 16, SEEK_SET);
    s_sis_map_head headctrl;
    if (sis_read(fd, (char *)&headctrl, sizeof(s_sis_map_head)) != sizeof(s_sis_map_head))
    {
        LOG(5)("read file error. %s\n", fctrl->fname);
        sis_close(fd);
        return SIS_DISK_CMD_RWFAIL;
    }
    char *map = NULL;
    if (fctrl->rwmode == 1)
    {
        map = sis_mmap_w(fd, headctrl.fsize);
    }
    else
    {
        map = sis_mmap_r(fd, headctrl.fsize);
    }
    if (map == MAP_FAILED) 
    {
        LOG(5)("mapp file fail. %s\n", fctrl->fname);
        sis_close(fd);
        return SIS_DISK_CMD_MAPERR;
    }
    fctrl->mapmem = map;
    sis_close(fd);
    // 文件打开并映射好 下面开始读取索引数据
    return SIS_DISK_CMD_OK;
}

s_sis_sds sis_disk_io_map_get_fname(const char *fpath_, const char *fname_)
{
    s_sis_sds fname = sis_sdsempty();
    fname = sis_sdscatfmt(fname, "%s/%s.%s", fpath_, fname_, SIS_DISK_MAP_CHAR);
    return fname; 
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

#if 0

#define FNAME  "mymap"

// 测试MAP文件读写
static int    __read_nums = 0;
static size_t __read_size = 0;
static msec_t __read_msec = 0;
static int    __write_nums = 1;//*1000*1000;
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
    {
        s_sis_msec_pair pair = {1620793985999, 1711925510999};
        sis_disk_reader_sub_map(cxt, submode, "*", "*", &pair);
        printf("===============================\n");
    }
    {
        s_sis_msec_pair pair = {1620793985999, 1711925510999};
        sis_disk_reader_sub_map(cxt, submode, "k1,k2", "*", &pair);
        printf("===============================\n");
    }
    {
        s_sis_msec_pair pair = {1620793985999, 1711925510999};
        sis_disk_reader_sub_map(cxt, submode, "*", "info,smsec", &pair);
        printf("===============================\n");
    }

    {
        s_sis_msec_pair pair = {0, 1621315510999};
        sis_disk_reader_sub_map(cxt, submode, "*", "*", &pair);
        printf("===============================\n");
    }



    {
        s_sis_msec_pair pair = {1620793979999, 0};
        sis_disk_reader_sub_map(cxt, submode, "*", "*", &pair);
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

    sis_disk_writer_open(cxt, 0);
    sis_disk_writer_set_kdict(cxt, inkeys, sis_strlen(inkeys));
    sis_disk_writer_set_sdict(cxt, insdbs, sis_strlen(insdbs));
    sis_disk_writer_start(cxt);

    int count = __write_nums;
    __write_msec = sis_time_get_now_msec();

    __write_size += sis_disk_writer_map(cxt, "k1", "info", &info_data[0], sizeof(s_info));
    __write_size += sis_disk_writer_map(cxt, "k2", "info", &info_data[1], sizeof(s_info));
    __write_size += sis_disk_writer_map(cxt, "k3", "info", &info_data[2], sizeof(s_info));
    for (int k = 0; k < count; k++)
    {
        for (int i = 6; i < 10; i++) // 4
        {
            s_tsec data;
            data.time = snap_data[i].time;
            data.value = k * 10 + i;
            __write_size += sis_disk_writer_map(cxt, "k4", "stsec", &data, sizeof(s_tsec));
        }
        for (int i = 0; i < 8; i++) // 5
        {
            s_tsec data;
            data.time = snap_data[i].time;
            data.value = k * 10 + i;
            __write_size += sis_disk_writer_map(cxt, "k2", "stsec", &data, sizeof(s_tsec));
        }
        for (int i = 0; i < 5; i++) // 5
        {
            __write_size += sis_disk_writer_map(cxt, "k3", "smsec", &snap_data[i], sizeof(s_msec));
        }
        for (int i = 4; i >= 0; i--) // 5
        {
            __write_size += sis_disk_writer_map(cxt, "k1", "smsec", &snap_data[i], sizeof(s_msec));
        }
    }
    sis_disk_writer_stop(cxt);
    sis_disk_writer_close(cxt);
    printf("write end %d %zu | cost: %lld.\n", __write_nums, __write_size, sis_time_get_now_msec() - __write_msec);
}
int main(int argc, char **argv)
{
    safe_memory_start();
    s_sis_map_fctrl *fctrl = sis_map_fctrl_create(FNAME);
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
        s_sis_disk_reader *rcxt = sis_disk_reader_create(".", FNAME, SIS_DISK_TYPE_MAP, &cb);
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
        s_sis_disk_writer *wcxt = sis_disk_writer_create(".", FNAME, SIS_DISK_TYPE_MAP);
        write_sdb(wcxt);
        // write_sdb_msec(wcxt);  // 按时间写入
        sis_disk_writer_destroy(wcxt);
    }
    sis_map_fctrl_destroy(fctrl);
    safe_memory_stop();
    return 0;
}
#endif