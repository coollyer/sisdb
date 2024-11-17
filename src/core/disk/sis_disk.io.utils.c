
#include "sis_disk.io.h"

int sis_disk_check_dtype(const char *rpath)
{
    int st = SIS_DTYPE_NONE;
    {
        // 先判断是否sno
        char fpath[255];
        sis_sprintf(fpath, 255, "%s/sno.*", rpath);
        char *files = sis_path_get_files(fpath, SIS_FINDPATH | SIS_FINDONE);
        if (files)
        {
            printf("%s \n %s\n", fpath, files);
            st = SIS_DTYPE_SNO;
            sis_free(files);
        }
    }
    if (st == SIS_DTYPE_NONE)
    {
        // 先判断是否msn
        char spath[255];
        sis_file_getpath(rpath, spath, 255);
        char sname[64];
        sis_file_getname(rpath, sname, 64);
        char fpath[255];
        sis_sprintf(fpath, 255, "%s/%s.msn", spath, sname);
        char *files = sis_path_get_files(fpath, SIS_FINDFILE | SIS_FINDONE);
        if (files)
        {
            printf("%s \n %s\n", fpath, files);
            st = SIS_DTYPE_MAP;
            sis_free(files);
        }
    }
    if (st == SIS_DTYPE_NONE)
    {
        // 先判断是否mdb
        char spath[255];
        sis_file_getpath(rpath, spath, 255);
        char sname[64];
        sis_file_getname(rpath, sname, 64);
        char fpath[255];
        sis_sprintf(fpath, 255, "%s/%s.mdb", spath, sname);
        char *files = sis_path_get_files(fpath, SIS_FINDFILE | SIS_FINDONE);
        if (files)
        {
            printf("%s \n %s\n", fpath, files);
            st = SIS_DTYPE_MAP;
            sis_free(files);
        }
    }
    if (st == SIS_DTYPE_NONE)
    {
        // 先判断是否csv
        char fpath[255];
        sis_sprintf(fpath, 255, "%s/*.csv", rpath);
        char *files = sis_path_get_files(fpath, SIS_FINDFILE | SIS_FINDONE);
        if (files)
        {
            printf("%s \n %s\n", fpath, files);
            st = SIS_DTYPE_CSV;
            sis_free(files);
        }
    }
    if (st == SIS_DTYPE_NONE)
    {
        // 先判断是否json
        char fpath[255];
        sis_sprintf(fpath, 255, "%s/*.json", rpath);
        char *files = sis_path_get_files(fpath, SIS_FINDFILE | SIS_FINDONE);
        if (files)
        {
            printf("%s \n %s\n", fpath, files);
            st = SIS_DTYPE_JSON;
            sis_free(files);
        }
    }
    return st;
}

