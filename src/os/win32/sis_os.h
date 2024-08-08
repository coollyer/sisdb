
#ifndef _SIS_OS_H
#define _SIS_OS_H

#ifdef __MSVC__
#pragma warning(disable: 4200 4267 4047 4996 4819 4244)
#endif

#define random rand
#define inline __inline
#define __func__  __FUNCTION__

#ifndef __WIN32__
#define __WIN32__
#endif
// ***  注意顺序不能变  *** //
// #ifdef WINDOWS
#include <winsock2.h>
#include <windows.h>
#ifdef __MSVC__
#pragma comment (lib, "ws2_32.lib")
#endif
// ***  注意顺序不能变  *** //
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <io.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <stdint.h>

#include <os_malloc.h>
#include <os_types.h>

// #include <os_file.h>
// #include <os_fork.h>
// #include <os_net.h>
// #include <os_str.h>
// #include <os_thread.h>
// #include <os_time.h>

#define CLR_RED    ""
#define CLR_GREEN  ""
#define CLR_BLUE   ""
#define CLR_GOLD   ""

#define CLR_LRED   ""
#define CLR_LGREEN ""
#define CLR_LBLUE  ""
#define CLR_LHOT   ""
#define CLR_YELLOW ""
#define RESET      ""

// #endif 
#endif //_SIS_OS_H