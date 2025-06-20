﻿
#include <os_file.h>
#include <dirent.h>
#include <fnmatch.h>
#include <os_time.h>

/* Replace \ to / in the path string */
void sis_file_fixpath(char *in_)
{
	int size = strlen(in_);
	for (int i = size - 1; i > 0; i--)
	{
		if (in_[i] == '\\')
		{
			in_[i] = SIS_PATH_SEPARATOR;
		}
	}
}

s_sis_handle sis_open(const char *fn_, int mode_, int access_)
{
	sis_file_fixpath((char *)fn_);
	return open(fn_, mode_, access_);
}

int sis_getpos(s_sis_handle fp_, size_t *offset_)
{
    long long size = sis_seek(fp_, 0, SEEK_CUR);
	if (size >= 0)
	{	
		*offset_ = size;
		return 0;
	}
	return -1;
}
int sis_setpos(s_sis_handle fp_, size_t *offset_)
{
    long long size = sis_seek(fp_, *offset_, SEEK_SET);
	if (size >= 0 )
	{	
		*offset_ = size;
		return 0;
	}
	return -1;
}
size_t sis_size(s_sis_handle fp_)
{
    return sis_seek(fp_, 0, SEEK_END);
}
size_t sis_read(s_sis_handle fp_, char *in_, size_t len_)
{
	return read(fp_, in_, len_);
}
size_t sis_write(s_sis_handle fp_, const char *in_, size_t len_)
{
	return write(fp_, in_, len_);
}

// s_sis_file_handle sis_file_open(const char *fn_, int mode_, int access_)
// {
// 	sis_file_fixpath((char *)fn_);
// 	s_sis_file_handle fp = NULL;
// 	char mode[5];
// 	int index = 0;
// 	if (mode_ & SIS_FILE_IO_TRUNC)
// 	{
// 		mode[index] = 'w';
// 		index++;
// 	}
// 	else
// 	{
// 		if (mode_ & SIS_FILE_IO_CREATE)
// 		{
// 			mode[index] = 'a';
// 			index++;
// 		}
// 	}
// 	if (index == 0)
// 	{
// 		mode[index] = 'r';
// 		index++;
// 	}
// 	// mode[index] = 'b';
// 	// index++;
// 	// if ((mode_ & SIS_FILE_IO_READ && mode_ & SIS_FILE_IO_WRITE) || mode_ & SIS_FILE_IO_RDWR)
// 	if (mode_ & SIS_FILE_IO_WRITE || mode_ & SIS_FILE_IO_RDWR)
// 	{
// 		mode[index] = '+';
// 		index++;
// 	}
// 	mode[index] = 0;

// 	fp = fopen(fn_, mode);

// 	// printf("[%p] %s %s \n ",fp,  fn_, mode);

// 	return fp;
// }

char *sis_mmap_open_r(const char *fn, size_t minsize, size_t *fsize)
{
	s_sis_handle fd = sis_open(fn, SIS_FILE_IO_RDWR, 0666);
    if (fd == -1)
    {
		printf("error opening file. %s\n", fn);
        return NULL;
    }
	struct stat stat;
	if (fstat(fd, &stat) == -1) {
        printf("error getting file status.\n");
        sis_close(fd);
        return NULL;
    }
    if (stat.st_size <= minsize)
    {
        printf("file format error. %s %lld\n", fn, (int64)stat.st_size);
        sis_close(fd);
        return NULL;
    }
    char *map = mmap(0, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) 
    {
        printf("mapp file fail. %s\n", fn);
        sis_close(fd);
        return NULL;
    }
	if (fsize)
	{
		*fsize = stat.st_size;
	}
	sis_close(fd);
	return map;
}
char *sis_mmap_open_w(const char *fn, size_t fsize)
{
    int mode = SIS_FILE_IO_RDWR;
    if (!sis_file_exists(fn))
    {
        mode |= SIS_FILE_IO_CREATE;
    }
	s_sis_handle fd = sis_open(fn, mode, 0666);
    if (fd == -1)
    {
        printf("error open the file.\n");
        return NULL;
    }
	if (fsize > 0)
	{
		if (sis_seek(fd, fsize - 1, SEEK_SET) == -1)
		{
			printf("error stretching the file.\n");
			sis_close(fd);
			return NULL;
		}
		// 在文件末尾写入一个空字节，扩大文件
		if (sis_write(fd, "", 1) != 1)
		{
			printf("error writing last byte of the file.\n");
			sis_close(fd);
			return NULL;
		}
	}
	else
	{
		struct stat stat;
		if (fstat(fd, &stat) == -1) 
        {
			printf("error getting file status.\n");
			sis_close(fd);
			return NULL;
		}
		fsize = stat.st_size;
	}
    char *map = mmap(0, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);;
    if (map == MAP_FAILED) 
    {
        printf("mapp file fail. %s\n", fn);
        sis_close(fd);
        return NULL;
    }
	sis_close(fd);
	return map;
}
char *sis_mmap_r(s_sis_handle fd, size_t isize)
{
	return mmap(0, isize, PROT_READ, MAP_SHARED, fd, 0);
}
char *sis_mmap_w(s_sis_handle fd, size_t isize)
{
	return mmap(0, isize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}
int  sis_mmap_sync(char *map, size_t isize)
{
	return msync(map, isize, MS_SYNC);
}
int  sis_unmmap(char *map, size_t isize)
{
	return munmap(map, isize);
}
s_sis_file_handle sis_file_open(const char *fn_, int mode_, int access_)
{
	sis_file_fixpath((char *)fn_);
	s_sis_file_handle fp = NULL;
	if (sis_file_exists(fn_))
	{
		if (mode_ & SIS_FILE_IO_TRUNC)
		{
			fp = fopen(fn_, "w");
			fclose(fp);
		}
		if (mode_ & SIS_FILE_IO_APPEND)
		{
			fp = fopen(fn_, "a+");
		}
		else if (mode_ & SIS_FILE_IO_WRITE || mode_ & SIS_FILE_IO_RDWR)
		{
			// fp = fopen(fn_, "a+");
			fp = fopen(fn_, "rb+");
		}
		else
		{
			fp = fopen(fn_, "r");
		}	
	}
	else
	{
		if (mode_ & SIS_FILE_IO_CREATE || mode_ & SIS_FILE_IO_TRUNC || mode_ & SIS_FILE_IO_WRITE || mode_ & SIS_FILE_IO_RDWR)
		{
			fp = fopen(fn_, "a+");
		//	fclose(fp);
		//	fp = fopen(fn_, "rb+");
		}
	}
	return fp;
}
long long sis_file_seek(s_sis_file_handle fp_, size_t offset_, int where_)
{
	long long o = fseek(fp_, offset_, where_);
	if ( o == 0)
	{
		return ftell(fp_);
	}
	return -1;
}
#ifdef __APPLE__
int sis_file_getpos(s_sis_file_handle fp_, size_t *offset_)
{
	fpos_t pos;
    int o = fgetpos(fp_, &pos);
	*offset_ = pos;
	return o;
}
int sis_file_setpos(s_sis_file_handle fp_, size_t *offset_)
{
	fpos_t pos;
	pos = *offset_;
    int o = fsetpos(fp_, &pos);
	*offset_ = pos;
	return o;
}
#else
int sis_file_getpos(s_sis_file_handle fp_, size_t *offset_)
{
	fpos_t pos;
    int o = fgetpos(fp_, &pos);
	*offset_ = pos.__pos;
	return o;
}
int sis_file_setpos(s_sis_file_handle fp_, size_t *offset_)
{
	fpos_t pos;
	pos.__pos = *offset_;
    int o = fsetpos(fp_, &pos);
	*offset_ = pos.__pos;
	return o;
}
#endif
size_t sis_file_size(s_sis_file_handle fp_)
{
	fseek(fp_, 0, SEEK_END);
	return ftell(fp_);
}
size_t sis_file_read(s_sis_file_handle fp_, char *in_, size_t len_)
{
	return fread((char *)in_, 1, len_, fp_);
}
size_t sis_file_write(s_sis_file_handle fp_, const char *in_, size_t len_)
{
	size_t size = fwrite((char *)in_, 1, len_, fp_);
	// printf("=======: %llx %lld\n", sis_file_seek(fp_, 0, SEEK_CUR),  sis_file_seek(fp_, 0, SEEK_CUR)); 
	// fflush(fp_);
	return size;
}
int sis_file_fsync(s_sis_file_handle fp_)
{
	return fsync(fileno(fp_));
}

void sis_file_getpath(const char *fn_, char *out_, int olen_)
{
	sis_file_fixpath((char *)fn_);
	out_[0] = 0;
	int i, len = (int)strlen(fn_);
	for (i = len - 1; i > 0; i--)
	{
		if (fn_[i] == SIS_PATH_SEPARATOR)
		{
			sis_strncpy(out_, olen_, fn_, i + 1);
			out_[i + 1] = 0;	
			return;
		}
	}
}
void sis_file_getname(const char *fn_, char *out_, int olen_)
{
	sis_file_fixpath((char *)fn_);

	int i, len = (int)strlen(fn_);
	for (i = len - 1; i > 0; i--)
	{
		if (fn_[i] == SIS_PATH_SEPARATOR)
		{
			sis_strncpy(out_, olen_, fn_ + i + 1, len - i - 1);	
			out_[len - i - 1] = 0;		
			return;
		}
	}
	sis_strncpy(out_, olen_, fn_, len);
	out_[len] = 0;
}
long long sis_get_file_size(const char *fn_) 
{
    struct stat file_info; // Unix/Linux 使用 struct stat
    // 获取文件元数据
    int ret = stat(fn_, &file_info); // Unix/Linux
    if (ret != 0) 
    {
        return -1;
    }
    return file_info.st_size; // 文件大小（字节）
}
const char *sis_get_file_create_time(const char *fn_)
{
    static char time_str[64] = {0};
    struct stat statbuf;
    
    if (stat(fn_, &statbuf) != 0) {
        return NULL;
    }
    
    // Linux/macOS使用st_ctime（文件状态改变时间）
    // Windows使用st_ctime（文件创建时间）
    struct tm* tm_info = localtime(&statbuf.st_ctime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    return time_str;
}

bool sis_file_exists(const char *fn_)
{
	sis_file_fixpath((char *)fn_);
	if (access(fn_, SIS_FILE_ACCESS_EXISTS) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}
bool sis_path_exists(const char *path_)
{
	char path[SIS_PATH_LEN];
	sis_file_getpath(path_, path, SIS_PATH_LEN);
	if (access(path, SIS_FILE_ACCESS_EXISTS) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}
bool sis_path_mkdir(const char *path_)
{
	int len = (int)strlen(path_);
	if (len == 0)
	{
		return false;
	}
	sis_file_fixpath((char *)path_);
	char dir[SIS_PATH_LEN];
	for (int i = 0; i < len; i++)
	{
		if (path_[i] == SIS_PATH_SEPARATOR)
		{
			sis_strncpy(dir, SIS_PATH_LEN, path_, i + 1);
			if (sis_path_exists(dir))
			{
				continue;
			}
			mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO);
		}
	}
	if (sis_path_exists(path_))
	{
		return true;
	}
	else
	{
		return false;
	}
}

int sis_file_rename(char *oldn_, char *newn_)
{
	sis_file_fixpath((char *)oldn_);
	sis_file_fixpath((char *)newn_);
	return rename(oldn_, newn_);
}

int sis_file_delete(const char *fn_)
{
	sis_file_fixpath((char *)fn_);
	return unlink(fn_);
	// remove(fn_);
}

void sis_path_complete(char *path_, int maxlen_)
{
	sis_file_fixpath(path_);
	size_t len = strlen(path_);
	int last = len - 1;
	if (path_[last] != SIS_PATH_SEPARATOR)
	{
		if (maxlen_ > (int)len)
		{
			path_[last + 1] = SIS_PATH_SEPARATOR;
		}
		else
		{
			path_[last] = SIS_PATH_SEPARATOR;
		}
	}
}
#ifndef __APPLE__
#define FNM_FILE_NAME (1 << 0)
#define FNM_CASEFOLD  (1 << 4)
#endif
char *sis_path_get_files(const char *path_, int mode_)
{
	char fname[255];
	strcpy(fname, path_);
	sis_file_fixpath(fname);

	char path[255];
	char findname[255];
	char filename[255];
	sis_file_getpath(fname, path, 255);
	sis_file_getname(fname, findname, 255);
	if (sis_strlen(findname) < 1)
	{
		findname[0] = '*';
		findname[1] = 0;
	}

	size_t size = 0;
	char *o = NULL;
	DIR *dirp = NULL;
	struct dirent *direntp = NULL;
	struct stat statbuf;
	if ((dirp = opendir(path)) != NULL)
	{
		while ((direntp = readdir(dirp)))
		{
			// printf("%s  %s\n",findname, direntp->d_name);
			if (!fnmatch(findname, direntp->d_name, FNM_FILE_NAME | FNM_CASEFOLD | FNM_PERIOD))
			{
				sprintf(filename, "%s%s", path, direntp->d_name);
				if (stat(filename, &statbuf) == -1)
				{	
					continue;
				}
				// printf("%s :: %x\n",filename, (int)statbuf.st_mode);
				if (((mode_ & SIS_FINDPATH) && S_ISDIR(statbuf.st_mode)) ||
					((mode_ & SIS_FINDFILE) && S_ISREG(statbuf.st_mode)) ||
					(mode_ == SIS_FINDALL))
				{
					if (o)
					{
						o = sis_strcat(o, &size, ":");
					}
					o = sis_strcat(o, &size, direntp->d_name);
				}
                if (mode_ & SIS_FINDONE)
                {
                    break;
                }
			}
		}
	}
	closedir(dirp);				
	return o;
}

void sis_path_del_files(char *path_)
{
	char fname[255];
	strcpy(fname, path_);
	sis_file_fixpath(fname);

	char path[255];
	char findname[255];
	char filename[255];
	sis_file_getpath(fname, path, 255);
	sis_file_getname(fname, findname, 255);
	if (sis_strlen(findname) < 1)
	{
		findname[0] = '*';
		findname[1] = 0;
	}
	// printf("%s\n",path_);
	DIR *dirp = NULL;
	struct dirent *direntp = NULL;
	struct stat statbuf;
	if ((dirp = opendir(path)) != NULL)
	{
		while ((direntp = readdir(dirp)))
		{
			printf("%s  %s\n",findname, direntp->d_name);
			if (!fnmatch(findname, direntp->d_name, FNM_FILE_NAME | FNM_CASEFOLD | FNM_PERIOD))
			{
				sprintf(filename, "%s%s", path, direntp->d_name);
				if (stat(filename, &statbuf) == -1)
				{	
					continue;
				}
				// printf("%s\n",filename);
				remove(filename);
			}
		}
	}
	closedir(dirp);
	if (findname[0] == '*' && findname[1] == 0)
	{
		remove(path_);
	}
}
#if 0
int main()
{
    char *paths = sis_path_get_files("../../data/former/femny/", SIS_FINDPATH);
    printf("--------path:\n%s\n--------\n", paths);
    char *files = sis_path_get_files("../../data/former/femny/20250602-121133-T5/worker.conf", SIS_FINDFILE | SIS_FINDONE);
    printf("--------file:\n%s\n", files);
    return 0;
}

#endif
// #include <fnmatch.h>

// int limit_path_filecount(char *FindName, int limit)
// {
// 	if (limit < 1) return 0;

// 	list_filetimeinfo fileinfo;

// 	char fname[255];
// 	strcpy(fname, FindName);
// 	translate_dir(fname);
// 	char path[255], findname[255], filename[255];
// 	get_file_path(fname, path);
// 	get_file_name(fname, findname);
// 	DIR *dirp = NULL;
// 	struct dirent *direntp = NULL;
// 	struct stat statbuf;
// 	if ((dirp = opendir(path)) != NULL)
// 	{
// 		while ((direntp = readdir(dirp)))
// 		{
// 			if (!fnmatch(findname, direntp->d_name, FNM_FILE_NAME | FNM_CASEFOLD | FNM_PERIOD))
// 			{
// 				sprintf(filename, "%s%s", path, direntp->d_name);
// 				if (stat(filename, &statbuf) == -1) continue;
// 				fileinfo.push_back(std::make_pair(statbuf.st_mtime, std::string(filename) ));
// 			}
// 		}
// 	}
// 	closedir(dirp);
// 	//if((int)fileinfo.size()<limit) return 0;
// 	time_t mintime;
// 	while ((int)fileinfo.size() > limit){
// 		int nIndex=-1;
// 		for(int i=0;i<(int)fileinfo.size();i++)
// 		{
// 			//LOG(1)("%d,%s\n",fileinfo[i].first,fileinfo[i].second.c_str());
// 			if(i==0)
// 			{
// 				mintime=fileinfo[i].first;
// 				nIndex=0;
// 				continue;
// 			}
// 			if(fileinfo[i].first<mintime)
// 			{
// 				mintime=fileinfo[i].first;
// 				nIndex=i;
// 			}
// 		}
// 		if(nIndex>=0)
// 		{
// 			//LOG(1)("remove %s\n",fileinfo[nIndex].second.c_str());
// 			remove(fileinfo[nIndex].second.c_str());
// 			fileinfo.erase(fileinfo.begin()+nIndex);
// 		}
// 	}
// 	return fileinfo.size();
// }


