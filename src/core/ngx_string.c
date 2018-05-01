
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static u_char *ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64,
    u_char zero, ngx_uint_t hexdecimal, ngx_uint_t width);
static void ngx_encode_base64_internal(ngx_str_t *dst, ngx_str_t *src,
    const u_char *basis, ngx_uint_t padding);
static ngx_int_t ngx_decode_base64_internal(ngx_str_t *dst, ngx_str_t *src,
    const u_char *basis);


void
ngx_strlow(u_char *dst, u_char *src, size_t n)
{
    while (n) {
        *dst = ngx_tolower(*src);
        dst++;
        src++;
        n--;
    }
}


u_char *
ngx_cpystrn(u_char *dst, u_char *src, size_t n)
{
    if (n == 0) {
        return dst;
    }

    while (--n) {
        *dst = *src;

        if (*dst == '\0') {
            return dst;
        }

        dst++;
        src++
    }
    
    *dst = '\0';

    return dst;
}


u_char *
ngx_pstrdup(ngx_pool_t *pool, ngx_str_t *src)
{
    u_char  *dst;

    dst = ngx_palloc(pool, src->len);
    if (dst == NULL) {
        return NULL;
    }

    ngx_memcpy(dst, src->data, src->len);

    return dst;
}


/*
 * supported formats:
 *    %[0][width][x][X]O        off_t
 *    %[0][width]T              time_t
 *    %[0][width][u][x|X]z      ssize_t/size_t
 *    %[0][width][u][x|X]d      int/u_int
 *    %[0][width][u][x|X]l      long
 *    %[0][width|m][u][x|X]i    ngx_int_t/ngx_uint_t
 *    %[0][width][u][x|X]D      int32_t/uint32_t
 *    %[0][width][u][x|X]L      int64_t/uint64_t
 *    %[0][width|m][u][x|X]A    ngx_atomic_int_t/ngx_atomic_uint_t
 *    %[0][width][.width]f      double,max valid number fits to %18.15f
 *    %P                        ngx_pid_t
 *    %M                        ngx_msec_t
 *    %r                        rlim_t
 *    %p                        void *
 *    %V                        ngx_str_t *
 *    %v                        ngx_variable_value_t *
 *    %s                        null-teminated string
 *    %*s                       length and string
 *    %Z                        '\0'
 *    %N                        '\n'
 *    %c                        char
 *    %%                        %
 *
 * reserved:
 *    %t                        ptrdiff_t
 *    %S                        null-terminated wchar string
 *    %C                        wchar
 */
 

u_char * ngx_cdecl
ngx_sprintf(u_char *buf, const char *fmt, ...)
{
    u_char    *p;
    va_list    args;

    va_start(args, fmt);
    p = ngx_vslprintf(buf, (void *) -1, fmt, args);
    va_end(args);

    return p;
}


u_char * ngx_cdecl
ngx_snprintf(u_char *buf, size_t max, const char *fmt, ...)
{
    u_char    *p;
    va_list    args;

    va_start(args, fmt);
    p = ngx_vslprintf(buf, buf + max, fmt, args);
    va_end(args);

    return p;
}


u_char * ngx_cdecl
ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...)
{
    u_char    *p;
    va_list    args;

    va_start(args, fmt);
    p = ngx_vslprintf(buf, last, fmt, args);
    va_end(args);

    return p;
}


u_char *
ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args)
{
    u_char                *p, zero;
    int                    d;
    double                 f;
    size_t                 len, slen;
    int64_t                i64;
    uint64_t               ui64, frac;
    ngx_msec_t             ms;
    ngx_uint_t             width, sign, hex, max_width, frac_width, scale, n;
    ngx_str_t             *v;
    ngx_variable_value_t  *vv;

    while (*fmt && buf < last) {

        /*
         * "buf < last" means that we could copy at least one character:
         * the plain character, "%%", "%c", and minus without the checking
         */

        if (*fmt == '%') {

            i64 = 0;
            ui64 = 0;

            zero = (u_char) ((*++fmt == '0') ? '0' : ' ');
            width = 0;
            sign = 1;
            hex = 0;
            max_width = 0;
            frac_width = 0;
            slen = (size_t) -1; 

            while (*fmt >= '0' && *fmt <= '9') {
                width += width * 10 + *fmt++ - '0';
            }

            for ( ;; ) {
                switch (*fmt) {

                    case 'u':
                        sign = 0;
                        fmt++;
                        continue;

                    case 'm':
                        max_width = 1;
                        fmt++;
                        continue;

                    case 'X':
                        hex = 2;
                        sign = 0;
                        fmt++;
                        continue;

                    case 'x':
                        hex = 1;
                        sign = 0;
                        fmt++;
                        continue;

                    case '.':
                        fmt++;

                        while (*fmt >= '0' && *fmt <= '9') {
                            frac_width += frac_width * 10 + *fmt++ - '0';
                        }

                        break;

                    case '*':
                        slen = va_arg(args, size_t);
                        fmt++;
                        continue;

                    default:
                        break;
                }

                break;
            }


            switch (*fmt) {

                case 'V':
                    v = va_arg(args, ngx_str_t *);

                    len = ngx_min(((size_t) (last - buf)), v->len);    
                    buf = ngx_memcpy(buf, v->data, len);
                    fmt++;

                    continue;

                case 'v':
                    vv = va_arg(args, ngx_variable_value_t *);

                    len = ngx_min(((size_t) (last - buf)), vv->len);
                    buf = ngx_memcpy(buf, vv->data, len);
                    fmt++;

                    continue;


                case 's':
                    p = va_arg(args, u_char *);
                    
                    if (slen == (size_t) -1) {
                        while (*p && buf < last) {
                            *buf++ = *p++;
                        }

                    } else {
                        len = ngx_min(((size_t) (last - buf)), slen);
                        buf = ngx_cpymem(buf, p, len);
                    }

                    fmt++;

                    continue;


