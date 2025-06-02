#ifndef _SIS_UTF8_H
#define _SIS_UTF8_H

#include "sis_bits.h"
#include <iconv.h>

#ifdef __cplusplus
extern "C" {
#endif

// olen 申请空间大于等于 ilen_
size_t sis_utf8_to_gbk(const char *in_, size_t ilen_, char *out_, size_t olen_);
// olen 申请空间大于等于 ilen_ * 3
size_t sis_gbk_to_utf8(const char *in_, size_t ilen_, char *out_, size_t olen_);

// 是不是utf格式字符串 1 = utf8 -1 gbk 0 未知 -2 表示ascii
int sis_chk_utf8(const char *in_, size_t ilen_);

// olen 申请空间大于等于 SIS_BASE64_ENCODE_OUT_SIZE
size_t sis_base64_encode(const char *in_, size_t ilen_, char *out_, size_t olen_);
// olen 申请空间大于等于 SIS_BASE64_DECODE_OUT_SIZE
size_t sis_base64_decode(const char *in_, size_t ilen_, char *out_, size_t olen_);

int sis_check_utf8_format(const char *rbuffer, size_t rsize);

#ifdef __cplusplus
}
#endif

#endif