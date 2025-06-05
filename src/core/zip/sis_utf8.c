#include "sis_utf8.h"

#define SWAPWORD(x)   (((x & 0xff00)>>8) | ((x & 0x00ff)<<8))

int iconv_utf8_to_gbk(char *imem_, size_t ilen_, char *omem_, size_t olen_)
{
    iconv_t ic = iconv_open("GBK", "UTF-8");
    if (ic == (iconv_t)-1) {
        return 0;
    }
    size_t ilen = ilen_;
    char  *imem = imem_;
    size_t olen = olen_;
    char  *omem = omem_;
    // 必须重新赋值 执行iconv 会改变传入的变量
    if (iconv(ic, &imem, &ilen, &omem, &olen) == (size_t)-1) 
    {
        iconv_close(ic);
        return 0;
    }
    olen = olen > 0 ? (olen_ - olen) : olen_ - 1;
    omem_[olen] = 0;
    iconv_close(ic); 
    return olen;
}
int iconv_gbk_to_utf8(char *imem_, size_t ilen_, char *omem_, size_t olen_)
{
    iconv_t ic = iconv_open("UTF-8", "GBK"); 
    if (ic == (iconv_t)-1) 
    {
        return 0;
    }
    // 必须重新赋值 执行iconv 会改变传入的变量
    size_t ilen = ilen_;
    char  *imem = imem_;
    size_t olen = olen_;
    char  *omem = omem_;
    if (iconv(ic, &imem, &ilen, &omem, &olen) == (size_t)-1) 
    {
        iconv_close(ic);
        return 0;
    }
    olen = olen > 0 ? (olen_ - olen) : olen_ - 1;
    omem_[olen] = 0;
    iconv_close(ic); 
    return olen;
}
size_t sis_utf8_to_gbk(const char *in_, size_t ilen_, char *out_, size_t olen_)
{
    if (!in_ || !out_ || !ilen_ || !olen_)
    {
        return 0;
    }
    return iconv_utf8_to_gbk((char *)in_, ilen_, out_, olen_);
}

size_t sis_gbk_to_utf8(const char *in_, size_t ilen_, char *out_, size_t olen_)
{
    if (!in_ || !out_ || !ilen_ || !olen_)
    {
        return 0;
    }
    return iconv_gbk_to_utf8((char *)in_, ilen_, out_, olen_);
}
// 是不是utf格式字符串
int sis_chk_utf8(const char *imem_, size_t ilen_)
{
    if (ilen_ < 1)
    {
        return 0;
    }
    int isascii = 1;
    uint8*  ustrpos = (uint8*)imem_;
    for (int i = 0; i < ilen_; i++)
    {
        if (*ustrpos >= 0x80)
        {
            isascii = 0;
        }
        ustrpos++;
    }
    if (isascii)
    {
        return -2;
    }
    int isutf8 = 0; // ascii 字符
    int size = ilen_ * 2;
    char *uname = sis_malloc(size);
    char *gname = sis_malloc(size);
    sis_gbk_to_utf8(imem_, ilen_, uname, size);
    sis_utf8_to_gbk(uname, sis_strlen(uname), gname, size);
    if (!sis_strcasecmp(imem_, gname))
    {
        isutf8 = -1; // gbk格式
    }
    else
    {
        sis_utf8_to_gbk(imem_, ilen_, gname, size);
        sis_gbk_to_utf8(gname, sis_strlen(gname), uname, size);
        if (!sis_strcasecmp(imem_, uname))
        {
            isutf8 = 1; // utf8格式
        }
    }
    sis_free(uname);
    sis_free(gname);
    return isutf8;
}



#define SIS_BASE64_PAD '='
#define SIS_BASE64DE_FIRST '+'
#define SIS_BASE64DE_LAST 'z'

/* BASE 64 encode table */
static const char _map_base64_encode[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/',
};

/* ASCII order for BASE 64 decode, 255 in unused character */
static const unsigned char _map_base64_decode[] = {
	/* nul, soh, stx, etx, eot, enq, ack, bel, */
	   255, 255, 255, 255, 255, 255, 255, 255,
	/*  bs,  ht,  nl,  vt,  np,  cr,  so,  si, */
	   255, 255, 255, 255, 255, 255, 255, 255,
	/* dle, dc1, dc2, dc3, dc4, nak, syn, etb, */
	   255, 255, 255, 255, 255, 255, 255, 255,
	/* can,  em, sub, esc,  fs,  gs,  rs,  us, */
	   255, 255, 255, 255, 255, 255, 255, 255,
	/*  sp, '!', '"', '#', '$', '%', '&', ''', */
	   255, 255, 255, 255, 255, 255, 255, 255,
	/* '(', ')', '*', '+', ',', '-', '.', '/', */
	   255, 255, 255,  62, 255, 255, 255,  63,
	/* '0', '1', '2', '3', '4', '5', '6', '7', */
	    52,  53,  54,  55,  56,  57,  58,  59,
	/* '8', '9', ':', ';', '<', '=', '>', '?', */
	    60,  61, 255, 255, 255, 255, 255, 255,
	/* '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', */
	   255,   0,   1,  2,   3,   4,   5,    6,
	/* 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', */
	     7,   8,   9,  10,  11,  12,  13,  14,
	/* 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', */
	    15,  16,  17,  18,  19,  20,  21,  22,
	/* 'X', 'Y', 'Z', '[', '\', ']', '^', '_', */
	    23,  24,  25, 255, 255, 255, 255, 255,
	/* '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', */
	   255,  26,  27,  28,  29,  30,  31,  32,
	/* 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', */
	    33,  34,  35,  36,  37,  38,  39,  40,
	/* 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', */
	    41,  42,  43,  44,  45,  46,  47,  48,
	/* 'x', 'y', 'z', '{', '|', '}', '~', del, */
	    49,  50,  51, 255, 255, 255, 255, 255
};

size_t sis_base64_encode(const char *in_, size_t ilen_, char *out_, size_t olen_)
{
    if (!in_ || !out_ || !ilen_ || !olen_)
    {
        return 0;
    }
	size_t osize = 0;
	int sign = 0;
	unsigned char dot = 0;
	for (int i = 0; i < ilen_; i++) 
	{
		unsigned char ch = in_[i];

		switch (sign) {
		case 0:
			sign = 1;
			out_[osize++] =  _map_base64_encode[(ch >> 2) & 0x3F];
			break;
		case 1:
			sign = 2;
			out_[osize++] =  _map_base64_encode[((dot & 0x3) << 4) | ((ch >> 4) & 0xF)];
			break;
		case 2:
			sign = 0;
			out_[osize++] =  _map_base64_encode[((dot & 0xF) << 2) | ((ch >> 6) & 0x3)];
			out_[osize++] =  _map_base64_encode[ch & 0x3F];
			break;
		}
		dot = ch;
	}

	switch (sign) {
	case 1:
		out_[osize++] =  _map_base64_encode[(dot & 0x3) << 4];
		out_[osize++] = SIS_BASE64_PAD;
		out_[osize++] = SIS_BASE64_PAD;
		break;
	case 2:
		out_[osize++] =  _map_base64_encode[(dot & 0xF) << 2];
		out_[osize++] = SIS_BASE64_PAD;
		break;
	}
	out_[osize] = 0;
	return osize;
}
size_t sis_base64_decode(const char *in_, size_t ilen_, char *out_, size_t olen_)
{
    if (!in_ || !out_ || !ilen_ || (ilen_ & 0x3) || !olen_)
    {
        return 0;
    }
	unsigned char ch;
	size_t osize = 0;
	for (int i = 0; i < ilen_; i++) 
	{
		if (in_[i] == SIS_BASE64_PAD) {
			break;
		}
		if (in_[i] < SIS_BASE64DE_FIRST || in_[i] > SIS_BASE64DE_LAST) {
			return 0;
		}

		ch = _map_base64_decode[(unsigned char)in_[i]];
		if (ch == 255) 
		{
			return 0;
		}

		switch (i & 0x3) {
		case 0:
			out_[osize] = (ch << 2) & 0xFF;
			break;
		case 1:
			out_[osize++] |= (ch >> 4) & 0x3;
			out_[osize] = (ch & 0xF) << 4; 
			break;
		case 2:
			out_[osize++] |= (ch >> 2) & 0xF;
			out_[osize] = (ch & 0x3) << 6;
			break;
		case 3:
			out_[osize++] |= ch;
			break;
		}
	}
	return osize;
}
#if 0

int main() {
    const char gbk_str[] = {0xC4, 0xE3, 0xBA, 0xC3, 0x00};
    char ustr[24];
    size_t o = sis_gbk_to_utf8(gbk_str, 5, ustr, 24);
    if (ustr != NULL) {
        printf("转换后的 UTF-8 字符串: %s %d\n", ustr, o);
    }
    return 0;
}
#endif
#if 0
 

int main()
{
    {
        const char *gname = "万 科Ａ";
        char uname[128];
        sis_out_binary("--", gname, sis_strlen(gname));
        sis_utf8_to_gbk(gname, sis_strlen(gname), uname, 128);
        printf("%s | %s\n", gname, uname);
        sis_out_binary("--", uname, 16);
        char oname[128];
        // cdfe20bfc6a3a1
        sis_gbk_to_utf8(uname, sis_strlen(uname), oname, 128);
        sis_out_binary("--", oname, sis_strlen(oname));
        printf("%s | %s\n", gname, oname);
        printf("---------------------\n");
    }
    {
        const char *gname = "美丽生态";
        char uname[128];
        sis_out_binary("--", gname, sis_strlen(gname));
        sis_utf8_to_gbk(gname, sis_strlen(gname), uname, 128);
        printf("%s | %s\n", gname, uname);
        sis_out_binary("--", uname, 16);
        char oname[128];
        // c3c0c0f6c9faccac
        sis_gbk_to_utf8(uname, sis_strlen(uname), oname, 128);
        sis_out_binary("--", oname, sis_strlen(oname));
        printf("%s | %s\n", gname, oname);
        printf("---------------------\n");
    }
    {
        const char *gname = "南 玻Ａ";
        char uname[128];
        sis_out_binary("--", gname, sis_strlen(gname));
        sis_utf8_to_gbk(gname, sis_strlen(gname), uname, 128);
        printf("%s | %s\n", gname, uname);
        sis_out_binary("--", uname, 16);
        char oname[128];
        // c4cf20b2a5a3a1
        uname[0] = 0xc4; uname[1] = 0xcf; uname[2] = 0x20; uname[3] = 0xb2;
        uname[4] = 0xa5; uname[5] = 0xa3; uname[6] = 0xa1; uname[7] = 0x0;
        sis_out_binary("--", uname, sis_strlen(uname));
        sis_gbk_to_utf8(uname, sis_strlen(uname), oname, 128);
        sis_out_binary("--", oname, sis_strlen(oname));
        printf("%s | %s\n", gname, oname);
        printf("---------------------\n");
    }
    return 0;
}
#endif

#if 0
 

int main()
{
// 天
// Unicode	5929
// UTF-8 E5A4A9
// GBK CCEC
// GB2312 CCEC

	// 检查 sis_utf8_to_gbk 是否会改变输入内容
	printf("====\n");
	char uname[128];
	char gname[128]; gname[0] = 0;
	sis_sprintf(uname, 128, "%s", "1|\"天下无*双AT");
	printf("%s %s\n", uname, gname);
	sis_utf8_to_gbk(uname, sis_strlen(uname), gname, 128);
	printf("%s %s %zu %zu\n", uname, gname, sis_strlen(uname), sis_strlen(gname));
	sis_out_binary("--", uname, 16);
	sis_out_binary("--", gname, 16);
	
	char oname[128];
	sis_gbk_to_utf8(gname, sis_strlen(gname), oname, 128);
	
	sis_out_binary("--", gname, 16);
	sis_out_binary("--", oname, 16);
	printf("%s %s %zu %zu\n", gname, oname, sis_strlen(gname), sis_strlen(oname));

	sis_gbk_to_utf8(uname, sis_strlen(uname), gname, 128);
	sis_utf8_to_gbk(gname, sis_strlen(gname), oname, 128);
	sis_out_binary("--", gname, 16);
	sis_out_binary("--", oname, 16);
	printf("%s %s %zu %zu\n", gname, oname, sis_strlen(gname), sis_strlen(oname));

	return 0;
}
#endif
#if 0
#include <sis_zint.h>
int main()
{
	// float f0 = 0.00123456789;
	float f0  = -12312;
	float f1  = 123.45679;
	double f2 = 2606327.956;
	double d0 = 0.000123456789;
	double d1 = 1234.456789;
	double d2 = -12345678.456789;

	printf("zint32 : %zu zint64 %zu\n", sizeof(zint32), sizeof(zint64));
	printf("2.07 - 1 = %f %x \n", (2.07 - 1.0), -12312);

	printf("f0 = %10x %10x d0 = %10llx %10llx\n", sis_double_to_int32(f0, 3, 0), sis_double_to_int32(f0, 3, 1),sis_double_to_int64(d0, 3, 0), sis_double_to_int64(d0, 3, 1));
	printf("f1 = %10x %10x d1 = %10llx %10llx\n", sis_double_to_int32(f1, 3, 0), sis_double_to_int32(f1, 3, 1),sis_double_to_int64(d1, 3, 0), sis_double_to_int64(d1, 3, 1));
	printf("f2 = %10x %10x d2 = %10llx %10llx\n", sis_double_to_int32(f2, 3, 0), sis_double_to_int32(f2, 2, 1),sis_double_to_int64(d2, 3, 0), sis_double_to_int64(d2, 3, 1));

	zint32 z0  = sis_double_to_zint32(f0, 0, 1);
	zint64 z80 = sis_double_to_zint64(d0, 3, 1);
	printf("z0 = %d %15.5f z80 = %d %15.5f\n", 
		sis_zint32_valid(z0), 
		sis_zint32_to_double(z0), 
		sis_zint64_valid(z80), 
		sis_zint64_to_double(z80));
	zint32 z1  = sis_double_to_zint32(f1, 3, 1);
	zint64 z81 = sis_double_to_zint64(d1, 3, 1);
	printf("z1 = %d %15.5f z81 = %d %15.5f\n", 
		sis_zint32_valid(z1), 
		sis_zint32_to_double(z1), 
		sis_zint64_valid(z81), 
		sis_zint64_to_double(z81));
	zint32 z2  = sis_double_to_zint32(f2, 2, 1);
	zint64 z82 = sis_double_to_zint64(d2, 3, 1);
	printf("z2 = %d %15.5f z82 = %d %15.5f\n", 
		sis_zint32_valid(z2), 
		sis_zint32_to_double(z2), 
		sis_zint64_valid(z82), 
		sis_zint64_to_double(z82));

	int32 i1  = sis_double_to_int32(f1, 3, 1);
	int64 i81 = sis_double_to_int64(d1, 3, 1);
	printf("i1 = %d %15.5f i81 = %10lld %15.5f\n", 
		sis_zint32_i(i1),
		sis_int32_to_double(i1), 
		sis_zint64(i81),
		sis_int64_to_double(i81));

	return 0;
}


#endif


// #include <stdio.h>
// #include <stdint.h>
// #include <stdbool.h>
// #include <errno.h>

// // 枚举UTF-16子类型
// typedef enum {
//     UTF16_NONE,     // 非UTF-16
//     UTF16_LE,       // UTF-16小端
//     UTF16_BE        // UTF-16大端
// } UTF16Type;

// // 检测BOM并返回UTF-16类型（若不是则返回UTF16_NONE）
// UTF16Type _utf16_bom_exists(s_sis_file_handle fp)
// {
//     uint8_t bom[2] = {0};
//     // 读取前2字节（BOM至少需要2字节）
//     if (fread(bom, 1, 2, fp) != 2)
//     {
//         return UTF16_NONE; // 文件太小，无法判断
//     }

//     if (bom[0] == 0xFF && bom[1] == 0xFE)
//     {
//         return UTF16_LE; // 小端BOM
//     }
//     else if (bom[0] == 0xFE && bom[1] == 0xFF)
//     {
//         return UTF16_BE; // 大端BOM
//     }
//     else
//     {
//         return UTF16_NONE; // 无UTF-16 BOM
//     }
// }

// // 验证UTF-16编码规则（根据字节序）
// bool _utf16_validate(s_sis_file_handle fp, UTF16Type type)
// {
//     uint8_t buffer[2]; // 每次读取2字节（一个代码单元）
//     uint16_t code_unit;

//     while (1)
//     {
//         // 读取一个代码单元（2字节）
//         size_t read_bytes = fread(buffer, 1, 2, fp);
//         if (read_bytes == 0)
//             break; // 文件结束
//         if (read_bytes != 2)
//         { // 剩余字节不足2，无效
//             return false;
//         }

//         // 根据字节序组合为16位值
//         if (type == UTF16_LE)
//         {
//             code_unit = (buffer[1] << 8) | buffer[0]; // 小端：低位在前
//         }
//         else
//         {
//             code_unit = (buffer[0] << 8) | buffer[1]; // 大端：高位在前
//         }

//         // 验证代码单元是否符合UTF-16规则
//         if (code_unit >= 0xD800 && code_unit <= 0xDBFF)
//         {
//             // 高代理项，需检查下一个代码单元是否为低代理项（0xDC00-0xDFFF）
//             size_t next_read = fread(buffer, 1, 2, fp);
//             if (next_read != 2)
//             { // 无后续字节，高代理无低代理配对
//                 return false;
//             }
//             uint16_t next_code;
//             if (type == UTF16_LE)
//             {
//                 next_code = (buffer[1] << 8) | buffer[0];
//             }
//             else
//             {
//                 next_code = (buffer[0] << 8) | buffer[1];
//             }
//             if (next_code < 0xDC00 || next_code > 0xDFFF)
//             {
//                 return false; // 低代理项无效
//             }
//         }
//         else if (code_unit >= 0xDC00 && code_unit <= 0xDFFF)
//         {
//             return false; // 单独的低代理项无效
//         }
//         else if (code_unit > 0x10FFFF)
//         {
//             return false; // 超过Unicode最大码点（0x10FFFF）
//         }
//     }
//     return true; // 所有代码单元有效
// }

// int sis_check_utf16_format(s_sis_file_handle fp)
// {
//     // 检测BOM确定UTF-16类型
//     UTF16Type utf16_type = _utf16_bom_exists(fp);
//     if (utf16_type == UTF16_NONE)
//     {
//         return -1;
//     }

//     // 验证文件内容是否符合UTF-16规则
//     bool is_valid = _utf16_validate(fp, utf16_type);
//     if (!is_valid)
//     {
//         return -2;
//     }
//     return 0;
// }

// 检测文件是否以 UTF-8 BOM 开头（可选）
bool _utf8_bom_exists(const char *rbuffer, size_t rsize)
{
    // 读取前3字节（BOM需要3字节）
    if (rsize < 3)
    {
        return false; // 文件太小，无BOM
    }
    return ((uint8)rbuffer[0] == 0xEF && (uint8)rbuffer[1] == 0xBB && (uint8)rbuffer[2] == 0xBF);
}
// 验证文件内容是否符合 UTF-8 编码规则
bool _utf8_validate(const char *rbuffer, size_t rsize, int iswhole)
{
    int nowpos = 0;
    if (_utf8_bom_exists(rbuffer, rsize))
    {
        nowpos = 3;   // 跳过BOM的3字节
    }
    uint8 *rbyte = (uint8 *)rbuffer;
    int remaining = 0; // 剩余需要检查的续字节数
    while (nowpos < rsize)
    {
        if (remaining == 0)
        {
            // 处理首字节
            if ((rbyte[nowpos] & 0x80) == 0)
            {
                // 1字节字符（0x00-0x7F）
                nowpos++;
                continue;
            }
            else if ((rbyte[nowpos] & 0xE0) == 0xC0)
            {
                // 2字节字符：110xxxxx（0xC2-0xDF）
                if (rbyte[nowpos] < 0xC2 || rbyte[nowpos] > 0xDF)
                {
                    return false; // 无效的2字节首字节
                }
                remaining = 1;
            }
            else if ((rbyte[nowpos] & 0xF0) == 0xE0)
            {
                // 3字节字符：1110xxxx（0xE0-0xEF）
                if (rbyte[nowpos] == 0xE0)
                {
                    // 0xE0的后续4位不能小于0x02（排除0xE0 0x80-0x9F）
                    if ((rbyte[nowpos] & 0x0F) < 0x02)
                    {
                        return false;
                    }    
                }
                else if (rbyte[nowpos] == 0xED)
                {
                    // 0xED的后续4位不能大于0x0F（排除代理对0xED 0xA0-0xBF）
                    if ((rbyte[nowpos] & 0x0F) > 0x0F)
                        return false;
                }
                remaining = 2;
            }
            else if ((rbyte[nowpos] & 0xF8) == 0xF0)
            {
                // 4字节字符：11110xxx（0xF0-0xF4）
                if (rbyte[nowpos] < 0xF0 || rbyte[nowpos] > 0xF4)
                {
                    return false; // 超出UTF-8范围（0xF0-0xF4）
                }
                if (rbyte[nowpos] == 0xF0)
                {
                    // 0xF0的后续3位不能小于0x01（排除0xF0 0x80-0x8F）
                    if ((rbyte[nowpos] & 0x07) < 0x01)
                    {
                        return false;
                    }    
                }
                else if (rbyte[nowpos] == 0xF4)
                {
                    // 0xF4的后续3位不能大于0x04（排除0xF4 0x90-0xBF，超过0x10FFFF）
                    if ((rbyte[nowpos] & 0x07) > 0x04)
                    {
                        return false;
                    }
                }
                remaining = 3;
            }
            else
            {
                // 无效的首字节（如11111xxx或10xxxxxx）
                return false;
            }
        }
        else
        {
            // 处理续字节（必须是10xxxxxx）
            if ((rbyte[nowpos] & 0xC0) != 0x80)
            {
                return false; // 续字节格式错误
            }
            remaining--;
        }
        nowpos++;
    }

    // 所有多字节字符必须完整（无残留未处理的续字节）
    return remaining == 0;
}

int sis_check_utf8_format(const char *rbuffer, size_t rsize)
{
    // 验证UTF-8编码
    bool valid = _utf8_validate(rbuffer, rsize, 1);
    if (!valid)
    {
        return -1;
    }
    // printf("文件是有效的UTF-8格式%s\n", skipbom ? "（含BOM）" : "（无BOM）");
    return 0;
}

#if 0
int main() {
    const char *filename = "test_utf16le.txt";
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("无法打开文件");
        return 1;
    }

    // 检测BOM确定UTF-16类型
    UTF16Type utf16_type = detect_utf16_bom(file);
    if (utf16_type == UTF16_NONE) {
        printf("文件不是UTF-16格式\n");
        fclose(file);
        return 0;
    }

    // 验证文件内容是否符合UTF-16规则
    bool is_valid = _utf8_validate(file, utf16_type);
    if (is_valid) {
        printf("文件是有效的UTF-16格式（%s）\n", 
              utf16_type == UTF16_LE ? "小端LE" : "大端BE");
    } else {
        printf("文件是UTF-16格式但内容无效（如代理对不完整）\n");
    }

    fclose(file);
    return 0;
}

#endif