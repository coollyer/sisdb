
#include <sisdb_msub.h>


///////////////////////////////////////////
//  s_sisdb_msub_reader 
///////////////////////////////////////////

s_sisdb_msub_reader *sisdb_msub_reader_create(void *father, s_sis_json_node *node)
{
    s_sisdb_msub_reader *o = SIS_MALLOC(s_sisdb_msub_reader, o);
    sis_strcpy(o->cname, 255, node->key);
    o->father  = father;
    o->valid   = 0;
    o->status  = SIS_READER_STOP; 
    o->nowmsec = 0;
    o->worker  = sis_worker_create(NULL, node);
    return o;
}
void sisdb_msub_reader_destroy(void *reader_)
{
    s_sisdb_msub_reader *reader = reader_;
    sis_worker_destroy(reader->worker);
    if (reader->datas)
    {
        sis_node_list_destroy(reader->datas);
    }
    sis_free(reader);
}

void sisdb_msub_reader_init(s_sisdb_msub_reader *reader, int maxsize, int pagesize)
{
    if (reader->datas)
    {
        if (reader->maxsize != maxsize)
        {
            sis_node_list_destroy(reader->datas);
            reader->datas = NULL;
        }
        else
        {
            sis_node_list_clear(reader->datas);
        }
    }
    reader->stoped = 0;
    reader->maxsize = maxsize;
    if (!reader->datas)
    {
        reader->datas = sis_node_list_create(pagesize < 16 ? 1024 : pagesize, sizeof(s_sis_msub_v) + reader->maxsize);
    }
}
int  sisdb_msub_reader_push(s_sisdb_msub_reader *reader, int kidx, int sidx, void *in, size_t isize)
{
    s_sis_msub_v *newv = sis_node_list_empty(reader->datas);
    newv->kidx = kidx;
    newv->sidx = sidx;
    newv->size = isize;
    memmove(newv->data, in, isize);
    return sis_node_list_getsize(reader->datas);
}

void *sisdb_msub_reader_pop(s_sisdb_msub_reader *reader)
{
    return sis_node_list_pop(reader->datas);
}
void *sisdb_msub_reader_first(s_sisdb_msub_reader *reader)
{
    return sis_node_list_get(reader->datas, 0);
}

///////////////////////////////////////////
//  s_sisdb_msub_db
///////////////////////////////////////////

s_sisdb_msub_db *sisdb_msub_db_create()
{
    s_sisdb_msub_db *o = SIS_MALLOC(s_sisdb_msub_db, o);
    return o;
}
void sisdb_msub_db_destroy(void *mdb_)
{
    s_sisdb_msub_db *mdb = mdb_;
    sis_dynamic_db_destroy(mdb->rdb);
    sis_free(mdb);
}

///////////////////////////////////////////
//  callback define begin
///////////////////////////////////////////

