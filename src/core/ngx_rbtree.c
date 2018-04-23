
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * The red-black tree code is based on the algorithm described in
 * the "Introduction to Algorithms" by Cormen, Leiserson and Rivest.
 */


static ngx_inline void ngx_rbtree_left_rotate(ngx_rbtree_node_t *root,
    ngx_rbtree_node_t *sentinel, ngx_rbtree_node_t *node);
static ngx_inline void ngx_rbtree_right_rotate(ngx_rbtree_node_t *root,
    ngx_rbtree_node_t *sentinel, ngx_rbtree_node_t *node);


void
ngx_rbtree_insert(ngx_rbtree_t *tree, ngx_rbtree_node_t *node)
{
    ngx_rbtree_node_t  **root, *temp, *sentinel;

    /* a binary tree insert */


