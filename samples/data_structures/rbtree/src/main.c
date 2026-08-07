/*
 * Copyright (c) 2025 James Roy <rruuaanng@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/rb.h>
#include <zephyr/sys/printk.h>

struct user_data {
	int n;
	struct rbnode rbnode;
} data_list[5] = {{.n = 1}, {.n = 3}, {.n = 2}, {.n = 5}, {.n = 4}};

bool user_data_lessthan_func(struct rbnode *a, struct rbnode *b)
{
	struct user_data *n1 = CONTAINER_OF(a, struct user_data, rbnode);
	struct user_data *n2 = CONTAINER_OF(b, struct user_data, rbnode);

	return n1->n < n2->n;
}

int main(void)
{
	struct rbnode *n;
	struct rbnode *max, *min;

	struct rbtree tree = { .lessthan_fn = user_data_lessthan_func };

	for (int i = 0; i < ARRAY_SIZE(data_list); i++) {
		printk("insert n=%d rb=%p\n", data_list[i].n, &data_list[i].rbnode);
		rb_insert(&tree, &(data_list[i].rbnode));
	}

	max = rb_get_max(&tree);
	min = rb_get_min(&tree);
	struct user_data *max_node = CONTAINER_OF(max, struct user_data, rbnode);
	struct user_data *min_node = CONTAINER_OF(min, struct user_data, rbnode);

	printk("max=%d min=%d\n", max_node->n, min_node->n);

	printk("rbtree elements:\n");
	RB_FOR_EACH(&tree, n) {
		struct user_data *ent = CONTAINER_OF(n, struct user_data, rbnode);

		printk("n=%d\n", ent->n);
	}

	printk("removed max=%d\n", max_node->n);
	printk("removed min=%d\n", min_node->n);
	rb_remove(&tree, max);
	rb_remove(&tree, min);

	printk("rbtree after removal:\n");
	RB_FOR_EACH(&tree, n) {
		struct user_data *ent = CONTAINER_OF(n, struct user_data, rbnode);

		printk("n=%d\n", ent->n);
	}

	printk("Done\n");
	return 0;
}
