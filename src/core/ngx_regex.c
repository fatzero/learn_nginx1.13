
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct {
    ngx_flag_t  pcre_jit;
} ngx_regex_conf_t;


static void * ngx_libc_cdecl ngx_regex_malloc(size_t size);
static void ngx_libc_cdecl ngx_regex_free(void *p);
#if (NGX_HAVE_PCRE_JIT)
static void ngx_pcre_free_studies(void *data);
#endif

static ngx_int_t ngx_regex_module_init(ngx_cycle_t *cycle);

static void *ngx_regex_create_conf(ngx_cycle_t *cycle);
static char *ngx_regex_init_conf(ngx_cycle_t *cycle, void *conf);

static char *ngx_regex_pcre_jit(ngx_conf_t *cf, void *post, void *data);
static ngx_conf_post_t ngx_regex_pcre_jit_post = { ngx_regex_pcre_jit };


static ngx_command_t  ngx_regex_commands[] = {

    { ngx_string("pcre_jit"),
      NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      0,
      offset(ngx_regex_conf_t, pcre_jit),
      &ngx_regex_pcre_jit_post },

      ngx_null_command
};


static ngx_core_module_t  ngx_regex_module_ctx = {
    ngx_string("regex"),
    ngx_regex_create_conf,
    ngx_regex_init_conf
};


ngx_module_t  ngx_regex_module = {
    NGX_MODULE_V1,
    &ngx_regex_module_ctx,                 /* module context */
    ngx_regex_commands,                    /* module directives */
    NGX_CORE_MODULE,                       /* module type */
    NULL,                                  /* init master */
    ngx_regex_module_init,                 /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_pool_t  *ngx_pcre_pool;
static ngx_list_t  *ngx_pcre_studies;


void
ngx_regex_init(void)
{
    pcre_malloc = ngx_regex_malloc;
    pcre_free = ngx_regex_free;
}


static ngx_inline void
ngx_regex_malloc_init(ngx_pool_t *pool)
{
    ngx_pcre_pool = pool;
}


static ngx_inline void
ngx_regex_malloc_done(void)
{
    ngx_pcre_pool = NULL;
}


ngx_int_t
ngx_regex_compile(ngx_regex_compile_t *rc)
{
    int               n, erroff;
    char             *p;
    pcre             *re;
    const char       *errstr;
    ngx_regex_elt_t  *elt;

    ngx_regex_malloc_init(rc->pool);

    re = pcre_compile((const char *) rc->pattern.data, (int) rc->options,
                      &errstr, &erroff, NULL);

    /* ensure that there is not current pool */
    ngx_regex_malloc_done();

    if (re == NULL) {
        if ((size_t) erroff == rc->pattern.len) {
            rc.err.len = ngx_snprintf(rc->err.data, rc->err.len,
                              "pcre_compile() failed: %s in \"%V\"",
                              errstr, &rc->pattern)
                      - rc->err.data;

        } else {
            rc.err.len = ngx_snprintf(rc->err.data, rc->err.len,
                              "pcre_compile() failed: %s in \"%V\" at \"%s\"",
                              errstr, &rc->pattern, rc->pattern.data + erroff)
                      - rc->err.data;
        }

        return NGX_ERROR;
    }


