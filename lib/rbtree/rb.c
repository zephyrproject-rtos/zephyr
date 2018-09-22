/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* These assertions are very useful when debugging the tree code
 * itself, but produce significant performance degradation as they are
 * checked many times per operation.  Leave them off unless you're
 * working on the rbtree code itself
 */
#define CHECK(n) /**/
/* #define CHECK(n) __ASSERT_NO_MSG(n) */

#include <kernel.h>
#include <misc/rb.h>

enum rb_color { RED = 0, BLACK = 1 };

static struct rbnode *get_child(struct rbnode *n, int side)
{
	CHECK(n);
	if (side) {
		return n->children[1];
	}

	uintptr_t l = (uintptr_t) n->children[0];

	l &= ~1ul;
	return (struct rbnode *) l;
}

static void set_child(struct rbnode *n, int side, void *val)
{
	CHECK(n);
	if (side) {
		n->children[1] = val;
	} else {
		uintptr_t old = (uintptr_t) n->children[0];
		uintptr_t new = (uintptr_t) val;

		n->children[0] = (void *) (new | (old & 1ul));
	}
}

static enum rb_color get_color(struct rbnode *n)
{
	CHECK(n);
	return ((uintptr_t)n->children[0]) & 1ul;
}

static int is_black(struct rbnode *n)
{
	return get_color(n) == BLACK;
}

static int is_red(struct rbnode *n)
{
	return get_color(n) == RED;
}

static void set_color(struct rbnode *n, enum rb_color color)
{
	CHECK(n);

	uintptr_t *p = (void *) &n->children[0];

	*p = (*p & ~1ul) | color;
}

/* Searches the tree down to a node that is either identical with the
 * "node" argument or has an empty/leaf child pointer where "node"
 * should be, leaving all nodes found in the resulting stack.  Note
 * that tree must not be empty and that stack should be allocated to
 * contain at least tree->max_depth entries!  Returns the number of
 * entries pushed onto the stack.
 */
static int find_and_stack(struct rbtree *tree, struct rbnode *node,
			  struct rbnode **stack)
{
	int sz = 0;

	stack[sz++] = tree->root;

	while (stack[sz - 1] != node) {
		int side = tree->lessthan_fn(node, stack[sz - 1]) ? 0 : 1;
		struct rbnode *ch = get_child(stack[sz - 1], side);

		if (ch) {
			stack[sz++] = ch;
		} else {
			break;
		}
	}

	return sz;
}

struct rbnode *_rb_get_minmax(struct rbtree *tree, int side)
{
	struct rbnode *n;

	for (n = tree->root; n && get_child(n, side); n = get_child(n, side))
		;
	return n;
}

static int get_side(struct rbnode *parent, struct rbnode *child)
{
	CHECK(get_child(parent, 0) == child || get_child(parent, 1) == child);

	return get_child(parent, 1) == child ? 1 : 0;
}

/* Swaps the position of the two nodes at the top of the provided
 * stack, modifying the stack accordingly. Does not change the color
 * of either node.  That is, it effects the following transition (or
 * its mirror if N is on the other side of P, of course):
 *
 *    P          N
 *  N  c  -->  a   P
 * a b            b c
 *
 */
static void rotate(struct rbnode **stack, int stacksz)
{
	CHECK(stacksz >= 2);

	struct rbnode *parent = stack[stacksz - 2];
	struct rbnode *child = stack[stacksz - 1];
	int side = get_side(parent, child);
	struct rbnode *a = get_child(child, side);
	struct rbnode *b = get_child(child, !side);

	if (stacksz >= 3) {
		struct rbnode *grandparent = stack[stacksz - 3];

		set_child(grandparent, get_side(grandparent, parent), child);
	}

	set_child(child, side, a);
	set_child(child, !side, parent);
	set_child(parent, side, b);
	stack[stacksz - 2] = child;
	stack[stacksz - 1] = parent;
}

/* The node at the top of the provided stack is red, and its parent is
 * too.  Iteratively fix the tree so it becomes a valid red black tree
 * again
 */
static void fix_extra_red(struct rbnode **stack, int stacksz)
{
	while (stacksz > 1) {
		struct rbnode *node = stack[stacksz - 1];
		struct rbnode *parent = stack[stacksz - 2];

		/* Correct child colors are a precondition of the loop */
		CHECK(!get_child(node, 0) || is_black(get_child(node, 0)));
		CHECK(!get_child(node, 1) || is_black(get_child(node, 1)));

		if (is_black(parent)) {
			return;
		}

		/* We are guaranteed to have a grandparent if our
		 * parent is red, as red nodes cannot be the root
		 */
		CHECK(stacksz >= 2);

		struct rbnode *grandparent = stack[stacksz - 3];
		int side = get_side(grandparent, parent);
		struct rbnode *aunt = get_child(grandparent, !side);

		if ((aunt != NULL) && is_red(aunt)) {
			set_color(grandparent, RED);
			set_color(parent, BLACK);
			set_color(aunt, BLACK);

			/* We colored the grandparent red, which might
			 * have a red parent, so continue iterating
			 * from there.
			 */
			stacksz -= 2;
			continue;
		}

		/* We can rotate locally to fix the whole tree.  First
		 * make sure that node is on the same side of parent
		 * as parent is of grandparent.
		 */
		int parent_side = get_side(parent, node);

		if (parent_side != side) {
			rotate(stack, stacksz);
			node = stack[stacksz - 1];
		}

		/* Rotate the grandparent with parent, swapping colors */
		rotate(stack, stacksz - 1);
		set_color(stack[stacksz - 3], BLACK);
		set_color(stack[stacksz - 2], RED);
		return;
	}

	/* If we exit the loop, it's because our node is now the root,
	 * which must be black.
	 */
	set_color(stack[0], BLACK);
}

void rb_insert(struct rbtree *tree, struct rbnode *node)
{
	set_child(node, 0, NULL);
	set_child(node, 1, NULL);

	if (!tree->root) {
		tree->root = node;
		tree->max_depth = 1;
		set_color(node, BLACK);
		return;
	}

	struct rbnode *stack[tree->max_depth + 1];

	int stacksz = find_and_stack(tree, node, stack);

	struct rbnode *parent = stack[stacksz - 1];

	int side = !tree->lessthan_fn(node, parent);

	set_child(parent, side, node);
	set_color(node, RED);

	stack[stacksz++] = node;
	fix_extra_red(stack, stacksz);

	if (stacksz > tree->max_depth) {
		tree->max_depth = stacksz;
	}

	/* We may have rotated up into the root! */
	tree->root = stack[0];
	CHECK(is_black(tree->root));
}

/* Called for a node N (at the top of the stack) which after a
 * deletion operation is "missing a black" in its subtree.  By
 * construction N must be black (because if it was red it would be
 * trivially fixed by recoloring and we wouldn't be here).  Fixes up
 * the tree to preserve red/black rules.  The "null_node" pointer is
 * for situations where we are removing a childless black node.  The
 * tree munging needs a real node for simplicity, so we use it and
 * then clean it up (replace it with a simple NULL child in the
 * parent) when finished.
 */
static void fix_missing_black(struct rbnode **stack, int stacksz,
			      struct rbnode *null_node)
{
	/* Loop upward until we reach the root */
	while (stacksz > 1) {
		struct rbnode *c0, *c1, *inner, *outer;
		struct rbnode *n = stack[stacksz - 1];
		struct rbnode *parent = stack[stacksz - 2];
		int n_side = get_side(parent, n);
		struct rbnode *sib = get_child(parent, !n_side);

		CHECK(is_black(n));

		/* Guarantee the sibling is black, rotating N down a
		 * level if needed (after rotate() our parent is the
		 * child of our previous-sibling, so N is lower in the
		 * tree)
		 */
		if (!is_black(sib)) {
			stack[stacksz - 1] = sib;
			rotate(stack, stacksz);
			set_color(parent, RED);
			set_color(sib, BLACK);
			stack[stacksz++] = n;

			parent = stack[stacksz - 2];
			sib = get_child(parent, !n_side);
		}

		CHECK(sib);

		/* Cases where the sibling has only black children
		 * have simple resolutions
		 */
		c0 = get_child(sib, 0);
		c1 = get_child(sib, 1);
		if ((!c0 || is_black(c0)) && (!c1 || is_black(c1))) {
			if (n == null_node) {
				set_child(parent, n_side, NULL);
			}

			set_color(sib, RED);
			if (is_black(parent)) {
				/* Balance the sibling's subtree by
				 * coloring it red, then our parent
				 * has a missing black so iterate
				 * upward
				 */
				stacksz--;
				continue;
			} else {
				/* Recoloring makes the whole tree OK */
				set_color(parent, BLACK);
				return;
			}
		}

		CHECK((c0 && is_red(c0)) || (c1 && is_red(c1)));

		/* We know sibling has at least one red child.  Fix it
		 * so that the far/outer position (i.e. on the
		 * opposite side from N) is definitely red.
		 */
		outer = get_child(sib, !n_side);
		if (!(outer && is_red(outer))) {
			inner = get_child(sib, n_side);

			stack[stacksz - 1] = sib;
			stack[stacksz++] = inner;
			rotate(stack, stacksz);
			set_color(sib, RED);
			set_color(inner, BLACK);

			/* Restore stack state to have N on the top
			 * and make sib reflect the new sibling
			 */
			sib = stack[stacksz - 2];
			outer = get_child(sib, !n_side);
			stack[stacksz - 2] = n;
			stacksz--;
		}

		/* Finally, the sibling must have a red child in the
		 * far/outer slot.  We can rotate sib with our parent
		 * and recolor to produce a valid tree.
		 */
		CHECK(is_red(outer));
		set_color(sib, get_color(parent));
		set_color(parent, BLACK);
		set_color(outer, BLACK);
		stack[stacksz - 1] = sib;
		rotate(stack, stacksz);
		if (n == null_node) {
			set_child(parent, n_side, NULL);
		}
		return;
	}
}

void rb_remove(struct rbtree *tree, struct rbnode *node)
{
	struct rbnode *stack[tree->max_depth + 1], *tmp;

	int stacksz = find_and_stack(tree, node, stack);

	if (node != stack[stacksz - 1]) {
		return;
	}

	/* We can only remove a node with zero or one child, if we
	 * have two then pick the "biggest" child of side 0 (smallest
	 * of 1 would work too) and swap our spot in the tree with
	 * that one
	 */
	if (get_child(node, 0) && get_child(node, 1)) {
		int stacksz0 = stacksz;
		struct rbnode *hiparent, *loparent;
		struct rbnode *node2 = get_child(node, 0);

		hiparent = stacksz > 1 ? stack[stacksz - 2] : NULL;
		stack[stacksz++] = node2;
		while (get_child(node2, 1)) {
			node2 = get_child(node2, 1);
			stack[stacksz++] = node2;
		}

		loparent = stack[stacksz - 2];

		/* Now swap the position of node/node2 in the tree.
		 * Design note: this is a spot where being an
		 * intrusive data structure hurts us fairly badly.
		 * The trees you see in textbooks do this by swapping
		 * the "data" pointers between the two nodes, but we
		 * have a few special cases to check.  In principle
		 * this works by swapping the child pointers between
		 * the nodes and retargetting the nodes pointing to
		 * them from their parents, but: (1) the upper node
		 * may be the root of the tree and not have a parent,
		 * and (2) the lower node may be a direct child of the
		 * upper node.  Remember to swap the color bits of the
		 * two nodes also.  And of course we don't have parent
		 * pointers, so the stack tracking this structure
		 * needs to be swapped too!
		 */
		if (hiparent) {
			set_child(hiparent, get_side(hiparent, node), node2);
		} else {
			tree->root = node2;
		}

		if (loparent == node) {
			set_child(node, 0, get_child(node2, 0));
			set_child(node2, 0, node);
		} else {
			set_child(loparent, get_side(loparent, node2), node);
			tmp = get_child(node, 0);
			set_child(node, 0, get_child(node2, 0));
			set_child(node2, 0, tmp);
		}

		set_child(node2, 1, get_child(node, 1));
		set_child(node, 1, NULL);

		tmp = stack[stacksz0 - 1];
		stack[stacksz0 - 1] = stack[stacksz - 1];
		stack[stacksz - 1] = tmp;

		int ctmp = get_color(node);

		set_color(node, get_color(node2));
		set_color(node2, ctmp);
	}

	CHECK(!get_child(node, 0) || !get_child(node, 1));

	struct rbnode *child = get_child(node, 0);

	if (child == NULL) {
		child = get_child(node, 1);
	}

	/* Removing the root */
	if (stacksz < 2) {
		tree->root = child;
		if (child) {
			set_color(child, BLACK);
		} else {
			tree->max_depth = 0;
		}
		return;
	}

	struct rbnode *parent = stack[stacksz - 2];

	/* Special case: if the node to be removed is childless, then
	 * we leave it in place while we do the missing black
	 * rotations, which will replace it with a proper NULL when
	 * they isolate it.
	 */
	if (!child) {
		if (is_black(node)) {
			fix_missing_black(stack, stacksz, node);
		} else {
			/* Red childless nodes can just be dropped */
			set_child(parent, get_side(parent, node), NULL);
		}
	} else {
		set_child(parent, get_side(parent, node), child);

		/* Check colors, if one was red (at least one must have been
		 * black in a valid tree), then we're done.  Otherwise we have
		 * a missing black we need to fix
		 */
		if (is_red(node) || is_red(child)) {
			set_color(child, BLACK);
		} else {
			stack[stacksz - 1] = child;
			fix_missing_black(stack, stacksz, NULL);
		}
	}

	/* We may have rotated up into the root! */
	tree->root = stack[0];
}

void _rb_walk(struct rbnode *node, rb_visit_t visit_fn, void *cookie)
{
	if (node != NULL) {
		_rb_walk(get_child(node, 0), visit_fn, cookie);
		visit_fn(node, cookie);
		_rb_walk(get_child(node, 1), visit_fn, cookie);
	}
}

struct rbnode *_rb_child(struct rbnode *node, int side)
{
	return get_child(node, side);
}

int _rb_is_black(struct rbnode *node)
{
	return is_black(node);
}

int rb_contains(struct rbtree *tree, struct rbnode *node)
{
	struct rbnode *n = tree->root;

	while (n && n != node) {
		n = get_child(n, tree->lessthan_fn(n, node));
	}

	return n == node;
}

/* Pushes the node and its chain of left-side children onto the stack
 * in the foreach struct, returning the last node, which is the next
 * node to iterate.  By construction node will always be a right child
 * or the root, so is_left must be false.
 */
static inline struct rbnode *stack_left_limb(struct rbnode *n,
					     struct _rb_foreach *f)
{
	f->top++;
	f->stack[f->top] = n;
	f->is_left[f->top] = 0;

	while ((n = get_child(n, 0))) {
		f->top++;
		f->stack[f->top] = n;
		f->is_left[f->top] = 1;
	}

	return f->stack[f->top];
}

/* The foreach tracking works via a dynamic stack allocated via
 * alloca().  The current node is found in stack[top] (and its parent
 * is thus stack[top-1]).  The side of each stacked node from its
 * parent is stored in is_left[] (i.e. if is_left[top] is true, then
 * node/stack[top] is the left child of stack[top-1]).  The special
 * case of top == -1 indicates that the stack is uninitialized and we
 * need to push an initial stack starting at the root.
 */
struct rbnode *_rb_foreach_next(struct rbtree *tree, struct _rb_foreach *f)
{
	struct rbnode *n;

	if (!tree->root) {
		return NULL;
	}

	/* Initialization condition, pick the leftmost child of the
	 * root as our first node, initializing the stack on the way.
	 */
	if (f->top == -1) {
		return stack_left_limb(tree->root, f);
	}

	/* The next child from a given node is the leftmost child of
	 * it's right subtree if it has a right child
	 */
	n = get_child(f->stack[f->top], 1);
	if (n != NULL) {
		return stack_left_limb(n, f);
	}

	/* Otherwise if the node is a left child of its parent, the
	 * next node is the parent (note that the root is stacked
	 * above with is_left set to 0, so this condition still works
	 * even if node has no parent).
	 */
	if (f->is_left[f->top]) {
		return f->stack[--f->top];
	}

	/* If we had no left tree and are a right child then our
	 * parent was already walked, so walk up the stack looking for
	 * a left child (whose parent is unwalked, and thus next).
	 */
	while (f->top > 0 && !f->is_left[f->top]) {
		f->top--;
	}

	f->top--;
	return f->top >= 0 ? f->stack[f->top] : NULL;
}
