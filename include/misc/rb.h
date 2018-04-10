/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Red/Black balanced tree data structure
 *
 * This implements an intrusive balanced tree that guarantees
 * O(log2(N)) runtime for all operations and amortized O(1) behavior
 * for creation and destruction of whole trees.  The algorithms and
 * naming are conventional per existing academic and didactic
 * implementations, c.f.:
 *
 * https://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 *
 * The implementation is size-optimized to prioritize runtime memory
 * usage.  The data structure is intrusive, which is to say the struct
 * rbnode handle is intended to be placed in a separate struct the
 * same way other such structures (e.g. Zephyr's dlist list) and
 * requires no data pointer to be stored in the node.  The color bit
 * is unioned with a pointer (fairly common for such libraries).  Most
 * notably, there is no "parent" pointer stored in the node, the upper
 * structure of the tree being generated dynamically via a stack as
 * the tree is recursed.  So the overall memory overhead of a node is
 * just two pointers, identical with a doubly-linked list.
 */

#ifndef _RB_H
#define _RB_H

struct rbnode {
	struct rbnode *children[2];
};

/**
 * @typedef rb_lessthan_t
 * @brief Red/black tree comparison predicate
 *
 * Compares the two nodes and returns 1 if node A is strictly less
 * than B according to the tree's sorting criteria, 0 otherwise.
 */
typedef int (*rb_lessthan_t)(struct rbnode *a, struct rbnode *b);

struct rbtree {
	struct rbnode *root;
	rb_lessthan_t lessthan_fn;
	int max_depth;
};

typedef void (*rb_visit_t)(struct rbnode *node, void *cookie);

struct rbnode *_rb_child(struct rbnode *node, int side);
int _rb_is_black(struct rbnode *node);
void _rb_walk(struct rbnode *node, rb_visit_t visit_fn, void *cookie);
struct rbnode *_rb_get_minmax(struct rbtree *tree, int side);

/**
 * @brief Insert node into tree
 */
void rb_insert(struct rbtree *tree, struct rbnode *node);

/**
 * @brief Remove node from tree
 */
void rb_remove(struct rbtree *tree, struct rbnode *node);

/**
 * @brief Returns the lowest-sorted member of the tree
 */
static inline struct rbnode *rb_get_min(struct rbtree *tree)
{
	return _rb_get_minmax(tree, 0);
}

/**
 * @brief Returns the highest-sorted member of the tree
 */
static inline struct rbnode *rb_get_max(struct rbtree *tree)
{
	return _rb_get_minmax(tree, 1);
}

/**
 * @brief Returns true if the given node is part of the tree
 *
 * Note that this does not internally dereference the node pointer
 * (though the tree's lessthan callback might!), it just tests it for
 * equality with items in the tree.  So it's feasible to use this to
 * implement a "set" construct by simply testing the pointer value
 * itself.
 */
int rb_contains(struct rbtree *tree, struct rbnode *node);

/**
 * @brief Walk/enumerate a rbtree
 *
 * Very simple recursive enumeration implementation.  A rather more
 * subtle (have to alloca() a stack to manage manually) iterative one
 * is possible that uses a FOREACH-style macro API, but this is good
 * enough for many purposes and much smaller.
 */
static inline void rb_walk(struct rbtree *tree, rb_visit_t visit_fn,
			   void *cookie)
{
	_rb_walk(tree->root, visit_fn, cookie);
}

#endif /* _RB_H */
