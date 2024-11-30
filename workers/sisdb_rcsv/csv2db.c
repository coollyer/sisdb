
#include <sis_obj.h>
#include "sis_utils.h"
#include "sis_list.h"
#include <csv2db.h>

/////////////////////////////////
//
/////////////////////////////////
s_csvdb_field *csvdb_field_create(int style, const char *fname)
{
    s_csvdb_field *o = SIS_MALLOC(s_csvdb_field, o);
    o->style = style;
    sis_strcpy(o->name, 128, fname);
    if (style == 1)
    {
        o->data = sis_int_list_create();
    }
    else if (style == 2)
    {
        o->data = sis_double_list_create();
    }
    else
    {
        o->data = sis_string_list_create();
    }
    return o;
}
void csvdb_field_destroy(void *fc)
{
    s_csvdb_field *o = fc;
    if (o->style == 1)
    {
        sis_int_list_destroy(o->data);
    }
    else if (o->style == 2)
    {
        sis_double_list_destroy(o->data);
    }
    else
    {
        sis_string_list_destroy(o->data);
    }
}
int csvdb_field_getsize(s_csvdb_field *fc)
{
    if (fc->style == 1)
    {
        s_sis_int_list *is = fc->data;
        return is->count;
    }
    else if (fc->style == 2)
    {
        return sis_double_list_getsize(fc->data);
    }
    else
    {
        return sis_string_list_getsize(fc->data);
    }
}
int csvdb_field_set_string(s_csvdb_field *fc, const char *in)
{
    if (fc->style == 0)
    {
        sis_string_list_push(fc->data, in, sis_strlen(in));
    }
    return csvdb_field_getsize(fc);
}
int csvdb_field_set_int(s_csvdb_field *fc, int64 in)
{
    if (fc->style == 1)
    {
        s_sis_int_list *is = fc->data;
        sis_int_list_push(is, in);
    }
    return csvdb_field_getsize(fc);
}
int csvdb_field_set_double(s_csvdb_field *fc, double in)
{
    if (fc->style == 2)
    {
        sis_double_list_push(fc->data, in);
    }
    return csvdb_field_getsize(fc);
}
const char *csvdb_field_get_string(s_csvdb_field *fc, int index)
{
    return sis_string_list_get(fc->data, index);
}
int64 csvdb_field_get_int(s_csvdb_field *fc, int index)
{
    s_sis_int_list *is = fc->data;
    return sis_int_list_get(is, index);
}
int csvdb_field_get_double(s_csvdb_field *fc, int index)
{
    return sis_double_list_get(fc->data, index);
}


///////////////////////////////////////////
//  callback define begin
///////////////////////////////////////////
static int cb_csv_read(void *source, void *argv)
{
    // s_csv2db_cxt *context = (s_csv2db_cxt *)source;
	// s_sis_file_csv_unit *unit = (s_sis_file_csv_unit *)argv;
	// printf("===== %5d : %3d \n", unit->index, unit->cols);
	// char str[100];
	// for (int i = 0; i < unit->cols; i++)
	// {
	// 	// sis_out_binary("--", unit->argv[i], unit->argsize[i]);
	// 	sis_strncpy(str, 100, unit->argv[i], unit->argsize[i]);
	// 	sis_trim(str);
    //     printf("%s\n", str);
	// 	// sis_out_binary("--", str, strlen(str));
	// }
	return 0;
}



///////////////////////////////////////////
//  callback define end.
///////////////////////////////////////////
void _csv2sno_w_factor(s_csv2db_cxt *context, int idate)
{
    // // 1. 提取 keys
    // // 2. 生成 sdbs
    // s_sis_dynamic_db *cdb = NULL;
    // // 3. 开始写入数据

    // char sdate[32];
    // sis_lldtoa(idate, sdate, 32, 10);
    // context->cb_wdb_sub_start(context->wsno, sdate);
    // context->cb_wdb_dict_keys(context->wsno, context->wkeys);
    // context->cb_wdb_dict_sdbs(context->wsno, context->wsdbs);

    // s_csvdb_field *tfield = sis_pointer_list_get(context->flist, 0);
    // if (tfield)
    // {
    //     int count = csvdb_field_getsize(tfield);
    //     for (int k = 0; k < count; k++)
    //     {
    //         s_sis_db_chars chars;
    //         chars.sname = "sis_factor";
    //         chars.size = cdb->size;
    //         chars.data = sis_malloc(chars.size);
    //         char *data =  chars.data;
            
    //         msec_t mt = csvdb_field_get_int(tfield, k); 
    //         memmove(data, &mt, sizeof(msec_t));
            
    //         s_csvdb_field *kfield = sis_pointer_list_get(context->flist, 1);
    //         chars.kname = csvdb_field_get_string(kfield, k); 
            
    //         for (int i = 2; i < context->flist->count; i++)
    //         {
    //             s_csvdb_field *cfield = sis_pointer_list_get(context->flist, i);
    //             double fv = csvdb_field_get_double(cfield, k);
    //             memmove(data + sizeof(msec_t) + (i - 2) * sizeof(double), &fv, sizeof(double)); 
    //         }
    //         context->cb_wdb_sub_chars(context->wsno, &chars);
    //     }
    // }
    // context->cb_wdb_sub_stop(context->wsno, sdate);
}
void _csv2sno_start_read(s_csv2db_cxt *context, int idate) 
{
    char fn[255];
    sis_sprintf(fn, 255, "%s/%s.%d.csv", context->rpath, context->rname, idate);
    LOG(5)("start read csv : %s\n", fn);

    sis_pointer_list_clear(context->flist);

    sis_file_csv_read_sub(fn, ',', context, cb_csv_read);
    // 这里读取数据完毕 开始提取数据

    LOG(5)("start write sno : %d\n", idate);
    if (context->work_mode == 1)
    {
        _csv2sno_w_factor(context, idate);
    }

}

///////////////////////////////////////////
//  method define
/////////////////////////////////////////
#define SIS_DT_STRING  0
#define SIS_DT_NUMBER  1
#define SIS_DT_DOUBLE  2

int sis_str_gettype(const char *in_, size_t ilen_)
{
    // strchr("-.0123456789e"
    int dt = SIS_DT_NUMBER;
    int fs = 0;
    int es = 0;
    int ds = 0;
    for (int i = 0; i < ilen_; i++)
    {
        if (!strchr("-.0123456789e", in_[i]))
        {
            dt = SIS_DT_STRING;
            break;
        }
        if (in_[i] == '-')
        {
            fs ++;
            if (fs > 2)
            {
                dt = SIS_DT_STRING;
                break;
            }
        }
        else if (in_[i] == '.')
        {
            ds ++;
            dt = SIS_DT_DOUBLE;
            if (ds > 1)
            {
                dt = SIS_DT_STRING;
                break;
            }
        }
        else if (in_[i] == 'e')
        {
            es ++;
            if (es > 1)
            {
                dt = SIS_DT_STRING;
                break;
            }
        }
    }
    return dt;
}
static int cb_csv2db_read(void *source, void *argv)
{
    s_sis_memory *omemory = (s_sis_memory *)source;
	s_sis_file_csv_unit *unit = (s_sis_file_csv_unit *)argv;
    if (!unit)
    {
        sis_memory_cat(omemory, "]}", 2);
        return 0;
    }
	// printf("===== %5d : %3d \n", unit->index, unit->cols);
    if (unit->index == 0)
    {
        // 处理标题
        sis_memory_cat(omemory, "{\"fields\":{", 11);
        char str[128];
        char fstr[128];
        for (int i = 0; i < unit->cols; i++)
        {
            sis_strncpy(str, 128, unit->argv[i], unit->argsize[i]);
            sis_trim(str);
            if (i == unit->cols - 1)
            {
                sis_sprintf(fstr, 128, "\"%s\":%d", str, i);
            }
            else
            {
                sis_sprintf(fstr, 128, "\"%s\":%d,", str, i);
            }
            // int len = sis_memory_cat(omemory, fstr, sis_strlen(fstr));
            // printf("-1-%s | %d\n", sis_memory(omemory), len);
        }
        sis_memory_cat(omemory, "},\"datas\":[", 11);
        // printf("-2-%s \n", sis_memory(omemory));
    }
    else
    {
        // 处理数据
        if (unit->index > 1)
        {
            sis_memory_cat(omemory, ",", 1);
        }
        sis_memory_cat(omemory, "[", 1);
        char str[128];
        char fstr[128];
        for (int i = 0; i < unit->cols; i++)
        {
            // sis_out_binary("--", unit->argv[i], unit->argsize[i]);
            sis_strncpy(str, 128, unit->argv[i], unit->argsize[i]);
            sis_trim(str);
            int dt = sis_str_gettype(str, sis_strlen(str));
            if (dt == SIS_DT_STRING)
            {
                if (i == unit->cols - 1)
                {
                    sis_sprintf(fstr, 128, "\"%s\"", str);
                }
                else
                {
                    sis_sprintf(fstr, 128, "\"%s\",", str);
                }
            }
            else
            {
                if (i == unit->cols - 1)
                {
                    sis_sprintf(fstr, 128, "%s", str);
                }
                else
                {
                    sis_sprintf(fstr, 128, "%s,", str);
                }
            }
            sis_memory_cat(omemory, fstr, sis_strlen(fstr));
            // sis_out_binary("--", str, strlen(str));
        }
        sis_memory_cat(omemory, "]", 1);
    }
    // if (unit->index < 3 || unit->index > 5095)
    // printf("--%s\n", sis_memory(omemory));
	return 0;
}
s_sis_memory *sis_csv2db_as_memory(const char *rpath, const char *rname, int idate)
{
    char fn[255];
    if (idate > 0)
    {
        sis_sprintf(fn, 255, "%s/%s.%d.csv", rpath, rname, idate);
    }
    else
    {
        sis_sprintf(fn, 255, "%s/%s.csv", rpath, rname);
    }
    LOG(5)("start read csv : %s\n", fn);
    
    s_sis_memory *omemory = sis_memory_create_size(1024 * 1024);
    if (sis_file_csv_read_sub(fn, ',', omemory, cb_csv2db_read) == -1)
    {
        sis_memory_destroy(omemory);
        return NULL;
    }
    // printf("--%s\n", sis_memory(omemory));
    LOG(5)("csv omemory size : %zu\n", sis_memory_get_size(omemory));
    if (sis_memory_get_size(omemory) > 0)
    {
        return omemory;
    }
    sis_memory_destroy(omemory);
    return NULL;
}

