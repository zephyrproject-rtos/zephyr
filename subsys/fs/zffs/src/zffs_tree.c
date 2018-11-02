/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/tree.h"
#include "zffs/area.h"
#include "zffs/misc.h"
#include <string.h>

#define ZFFS_TREE_ADDR_WAIT_WRITE 0xffffffff

struct zffs_tree_node {
	u32_t disk_addr;
	u32_t key[2 * ZFFS_TREE_T - 1];
	u32_t value[2 * ZFFS_TREE_T - 1];
	ATOMIC_DEFINE(loadflag, 2 * ZFFS_TREE_T);
	union {
		u32_t disk_child[2 * ZFFS_TREE_T];
		struct zffs_tree_node *child[2 * ZFFS_TREE_T];
	};
	struct zffs_tree_node *parent;
	u8_t root : 1;
	u8_t leaf : 1;
	u8_t n : 6;
};

struct zffs_tree_disk_node {
	u8_t root : 1;
	u8_t leaf : 1;
	u8_t n : 6;
	u8_t key[(2 * ZFFS_TREE_T - 1) * sizeof(u32_t)];
	u8_t value[(2 * ZFFS_TREE_T - 1) * sizeof(u32_t)];
	u8_t child[2 * ZFFS_TREE_T * sizeof(u32_t)];
	u8_t crc[2];
};

K_MEM_SLAB_DEFINE(zffs_tree_node_pool, sizeof(struct zffs_tree_node),
		  ZFFS_CONFIG_TREE_CACHE_NODE_MAX, sizeof(u32_t));

#define tree_node_is_full(__node) ((__node)->n == 2 * ZFFS_TREE_T - 1)

static struct zffs_tree_node *tree_node_alloc(void)
{
	void *node;

	if (k_mem_slab_alloc(&zffs_tree_node_pool, &node, K_NO_WAIT)) {
		node = NULL;
	}

	return node;
}

static void tree_node_free(struct zffs_tree_node *node)
{
	k_mem_slab_free(&zffs_tree_node_pool, (void **)&node);
}

static int tree_write_disk_node(struct zffs_data *zffs,
				struct zffs_area_pointer *pointer,
				struct zffs_tree_disk_node *disk_node)
{
	sys_put_le16(crc16_ccitt(0, (const void *)disk_node,
				 offsetof(struct zffs_tree_disk_node, crc)),
		     disk_node->crc);
	return zffs_area_write(zffs, pointer, disk_node, sizeof(*disk_node));
}

static int tree_read_disk_node(struct zffs_data *zffs, off_t off,
			       struct zffs_tree_disk_node *disk_node)
{
	int rc = zffs_area_random_read(zffs, zffs->swap_area, off, disk_node,
				       sizeof(*disk_node));

	if (rc) {
		return rc;
	}

	return crc16_ccitt(0, (const void *)disk_node, sizeof(*disk_node)) != 0
	       ? -EIO
	       : 0;
}

static int tree_load_node(struct zffs_data *zffs, u32_t addr,
			  struct zffs_tree_node *node)
{
	struct zffs_tree_disk_node disk_node;
	int rc = tree_read_disk_node(zffs, addr, &disk_node);

	if (rc) {
		return rc;
	}

	node->n = disk_node.n;
	node->leaf = disk_node.leaf;
	node->root = disk_node.root;
	node->disk_addr = addr;

	memcpy(node->key, disk_node.key, node->n * sizeof(node->key[0]));
	memcpy(node->value, disk_node.value, node->n * sizeof(node->value[0]));
	memset(node->loadflag, 0, sizeof(node->loadflag));

	if (!node->leaf) {
		u8_t *buf = disk_node.child;
		for (int i = 0; i < node->n + 1; i++, buf += sizeof(u32_t)) {
			node->disk_child[i] = sys_get_le32(buf);
		}
	}
	return 0;
}

static int tree_load_child(struct zffs_data *zffs,
			   struct zffs_tree_node *node, int child)
{
	int rc;
	u32_t addr;

	addr = node->disk_child[child];
	node->child[child] = tree_node_alloc();
	if (node->child[child] == NULL) {
		node->disk_child[child] = addr;
		return -ENOMEM;
	}

	rc = tree_load_node(zffs, addr, node->child[child]);

	if (rc) {
		tree_node_free(node->child[child]);
		node->child[child] = NULL;
		node->disk_child[child] = addr;
		return rc;
	}

	node->child[child]->parent = node;
	return 0;
}

static int tree_save_node(struct zffs_data *zffs,
			  struct zffs_area_pointer *pointer,
			  struct zffs_tree_node *node)
{
	struct zffs_tree_disk_node disk_node;
	int rc;

	if (node->disk_addr != ZFFS_TREE_ADDR_WAIT_WRITE) {
		return 0;
	}

	disk_node.n = node->n;
	disk_node.leaf = node->leaf;
	disk_node.root = node->root;
	memcpy(disk_node.key, node->key, node->n * sizeof(node->key[0]));
	memcpy(disk_node.value, node->value, node->n * sizeof(node->value[0]));
	if (!node->leaf) {
		u8_t *buf = disk_node.child;
		for (int i = 0; i < disk_node.n + 1;
		     i++, buf += sizeof(u32_t)) {
			if (atomic_test_bit(node->loadflag, i)) {
				if (node->child[i]->disk_addr ==
				    ZFFS_TREE_ADDR_WAIT_WRITE) {
					return -ESPIPE;
				}
				sys_put_le32(node->child[i]->disk_addr, buf);
			} else {
				sys_put_le32(node->disk_child[i], buf);
			}
		}
	}

	node->disk_addr = zffs_area_pointer_to_addr(zffs, pointer);
	rc = tree_write_disk_node(zffs, pointer, &disk_node);
	if (rc) {
		node->disk_addr = ZFFS_TREE_ADDR_WAIT_WRITE;
	} else {
		if (node->parent) {
			node->parent->disk_addr = ZFFS_TREE_ADDR_WAIT_WRITE;
		}
	}

	return rc;
}

static int tree_find_child(struct zffs_tree_node *parent,
			   struct zffs_tree_node *child)
{
	if (parent == NULL) {
		return 0;
	}

	for (int i = 0; i <= parent->n; i++) {
		if (atomic_test_bit(parent->loadflag, i)) {
			if (parent->child[i] == child) {
				return i;
			}
		}
	}

	return -1;
}

static int
tree_node_foreach(struct zffs_data *zffs, struct zffs_tree_node *top_node,
		  void *data, bool is_load, bool is_free,
		  int (*tree_node_cb)(struct zffs_data *zffs,
				      struct zffs_area_pointer *pointer,
				      struct zffs_tree_node *node, void *data))
{
	struct zffs_tree_node *node, *parent;
	int i, rc, c;
	struct zffs_area_pointer pointer;

	zffs_misc_lock(zffs);

	pointer.area_index = &zffs->swap_area;
	zffs_area_addr_to_pointer(zffs, zffs->swap_write_addr, &pointer);

	i = 0;
	node = top_node;
	parent = node->parent;
	c = tree_find_child(parent, node);
	if (c < 0) {
		return -ESPIPE;
	}

	while (1) {
		if (node->leaf || i > node->n) {
			if (tree_node_cb) {
				rc = tree_node_cb(zffs, &pointer, node, data);
				if (rc) {
					break;
				}
			}
		} else if (atomic_test_bit(node->loadflag, i)) {
foreach_in_child:
			parent = node;
			c = i;

			node = node->child[i];
			i = 0;
			continue;
		} else if (is_load) {
			rc = tree_load_child(zffs, node, i);
			if (rc) {
				break;
			}
			atomic_set_bit(node->loadflag, i);
			goto foreach_in_child;
		} else {
			i++;
			continue;
		}

		if (node == top_node) {
			rc = 0;
			break;
		}

		if (is_free) {
			if (node->disk_addr == ZFFS_TREE_ADDR_WAIT_WRITE) {
				rc = tree_save_node(zffs, &pointer, node);
				if (rc) {
					break;
				}
			}

			parent->disk_child[c] = node->disk_addr;
			atomic_clear_bit(parent->loadflag, c);
			tree_node_free(node);
		}

		i = c + 1;
		node = parent;
		parent = node->parent;
		c = tree_find_child(parent, node);
		if (c < 0) {
			rc = -ESPIPE;
			break;
		}
	}

	zffs->swap_write_addr = zffs_area_pointer_to_addr(zffs, &pointer);

	zffs_misc_unlock(zffs);
	return rc;
}

struct tree_key_foreach_data {
	void *data;
	int (*callback)(struct zffs_data *zffs, u32_t key, u32_t value,
			void *data);
};

static int tree_key_foreach_cb(struct zffs_data *zffs,
			       struct zffs_area_pointer *pointer,
			       struct zffs_tree_node *node,
			       struct tree_key_foreach_data *data)
{
	int rc;

	if (node->leaf) {
		for (int i = 0; i < node->n; i++) {
			rc = data->callback(zffs, node->key[i], node->value[i],
					    data->data);
			if (rc) {
				return rc;
			}
		}
	}

	if (node->parent) {
		int i = tree_find_child(node->parent, node);
		if (i < node->parent->n) {
			rc = data->callback(zffs, node->parent->key[i],
					    node->parent->value[i], data->data);
			if (rc) {
				return rc;
			}
		}
	}

	return 0;
}

static int
tree_key_foreach(struct zffs_data *zffs, struct zffs_tree_node *top_node,
		 void *data, bool is_load, bool is_free,
		 int (*tree_key_cb)(struct zffs_data *zffs, u32_t key,
				    u32_t value, void *data))
{
	struct tree_key_foreach_data inode_data = { .data = data,
						    .callback = tree_key_cb };

	return tree_node_foreach(zffs, top_node, &inode_data, is_load, is_free,
				 (void *)tree_key_foreach_cb);
}

static bool tree_node_is_in_path(struct zffs_tree_node *node,
				 struct zffs_tree_node *bottom_node)
{
	while (bottom_node) {
		if (bottom_node == node) {
			return true;
		}
		bottom_node = bottom_node->parent;
	}

	return false;
}

static int tree_node_free_other_path_cb(struct zffs_data *zffs,
					struct zffs_area_pointer *pointer,
					struct zffs_tree_node *node,
					void *bottom_node)
{
	struct zffs_tree_node *parent;
	int c, rc;

	if (tree_node_is_in_path(node, bottom_node)) {
		return 0;
	}

	if (node->disk_addr == ZFFS_TREE_ADDR_WAIT_WRITE) {
		parent = node->parent;

		if (parent == NULL) {
			return -ESPIPE;
		}

		c = tree_find_child(parent, node);
		if (c < 0) {
			return -ESPIPE;
		}

		rc = tree_save_node(zffs, pointer, node);
		if (rc) {
			return rc;
		}

		parent->disk_child[c] = node->disk_addr;
		atomic_clear_bit(parent->loadflag, c);
	}

	tree_node_free(node);

	return 0;
}

static int tree_node_free_other_path(struct zffs_data *zffs,
				     struct zffs_tree_node *top_node,
				     struct zffs_tree_node *node)
{
	return tree_node_foreach(zffs, top_node, node, false, false,
				 tree_node_free_other_path_cb);
}

static int tree_load_child_confirmation(struct zffs_data *zffs,
					struct zffs_tree_node *node, int child)
{
	int rc;
	bool is_retry = false;

retry:
	if (!atomic_test_and_set_bit(node->loadflag, child)) {
		rc = tree_load_child(zffs, node, child);
		if (rc) {
			atomic_clear_bit(node->loadflag, child);
			if (!is_retry && rc == -ENOMEM) {
				rc = tree_node_free_other_path(
					zffs, zffs->tree_root, node);
				if (rc) {
					goto err;
				}

				is_retry = true;
				goto retry;
			}

			goto err;
		}
	}

	rc = 0;
err:
	return rc;
}

static int tree_save_node_cb(struct zffs_data *zffs,
			     struct zffs_area_pointer *pointer,
			     struct zffs_tree_node *node, void *data)
{
	return tree_save_node(zffs, pointer, node);
}

static int tree_save_node_recursive(struct zffs_data *zffs,
				    struct zffs_tree_node *node)
{
	return tree_node_foreach(zffs, node, NULL, false, false,
				 tree_save_node_cb);
}

static bool tree_node_is_empty(struct zffs_data *zffs, u32_t addr)
{
	struct zffs_area_pointer pointer = zffs_swap_pointer(zffs);

	zffs_area_addr_to_pointer(zffs, addr, &pointer);

	return zffs_area_is_not_empty(zffs, &pointer,
				      sizeof(struct zffs_tree_disk_node)) !=
	       -ENOTEMPTY;
}

static int tree_load_root(struct zffs_data *zffs,
			  struct zffs_tree_node *node)
{
	int top, bottom;

	top = zffs_area_list_size(zffs->swap_area) /
	      sizeof(struct zffs_tree_disk_node);
	bottom = 0;
	node->parent = NULL;

	if (tree_node_is_empty(zffs, 0)) {
		zffs->swap_write_addr = 0;
		goto empty;
	}

	while (top - bottom > 1) {
		int n = (top + bottom) >> 1;

		if (tree_node_is_empty(
			    zffs, n * sizeof(struct zffs_tree_disk_node))) {
			top = n;
		} else {
			bottom = n;
		}
	}

	zffs->swap_write_addr = top * sizeof(struct zffs_tree_disk_node);

	while (top--) {
		if (tree_load_node(zffs,
				   top * sizeof(struct zffs_tree_disk_node),
				   node) == 0 &&
		    node->root) {
			return 0;
		}
	}

empty:
	node->disk_addr = ZFFS_TREE_ADDR_WAIT_WRITE;
	node->n = 0;
	node->leaf = 1;
	node->root = 1;
	memset(node->loadflag, 0x00, sizeof(node->loadflag));
	return 0;
}

static void tree_node_copy_key(struct zffs_tree_node *from, off_t from_off,
			       struct zffs_tree_node *to, off_t to_off)
{
	to->key[to_off] = from->key[from_off];
	to->value[to_off] = from->value[from_off];
	if (!from->leaf) {
		if (atomic_test_and_clear_bit(from->loadflag, from_off + 1)) {
			to->child[to_off + 1] = from->child[from_off + 1];
			atomic_set_bit(to->loadflag, to_off + 1);
			to->child[to_off + 1]->parent = to;
		} else {
			to->disk_child[to_off + 1] =
				from->disk_child[from_off + 1];
		}
	}
}

int zffs_tree_init(struct zffs_data *zffs)
{
	int rc;

	zffs_misc_lock(zffs);
	k_mem_slab_init(&zffs_tree_node_pool,
			_k_mem_slab_buf_zffs_tree_node_pool,
			sizeof(struct zffs_tree_node),
			ZFFS_CONFIG_TREE_CACHE_NODE_MAX);
	zffs->swap_write_addr = 0;
	zffs->tree_root = tree_node_alloc();

	if (zffs->tree_root == NULL) {
		rc = -ENOMEM;
		goto err;
	}

	rc = tree_load_root(zffs, zffs->tree_root);

err:
	zffs_misc_unlock(zffs);
	return rc;
}

int zffs_tree_search(struct zffs_data *zffs, u32_t key, u32_t *value)
{
	int i;
	int rc;
	struct zffs_tree_node *node = zffs->tree_root;

	while (true) {
		for (i = 0; i < node->n; i++) {
			if (node->key[i] == key) {
				*value = node->value[i];
				return 0;
			} else if (node->key[i] > key) {
				break;
			}
		}

		if (node->leaf) {
			return -ENOENT;
		}

		rc = tree_load_child_confirmation(zffs, node, i);
		if (rc) {
			return rc;
		}

		node = node->child[i];
	}
}

int zffs_tree_update(struct zffs_data *zffs, u32_t key, u32_t value)
{
	int i;
	int rc;
	struct zffs_tree_node *node = zffs->tree_root;

	while (true) {
		for (i = 0; i < node->n; i++) {
			if (node->key[i] == key) {
				node->value[i] = value;
				node->disk_addr = ZFFS_TREE_ADDR_WAIT_WRITE;
				return 0;
			} else if (node->key[i] > key) {
				break;
			}
		}

		if (node->leaf) {
			return -EBADF;
		}

		rc = tree_load_child_confirmation(zffs, node, i);
		if (rc) {
			return rc;
		}

		node = node->child[i];
	}
}

static int tree_node_insert(struct zffs_tree_node *node, u32_t key,
			    u32_t value)
{
	int i;
	int j;

	if (tree_node_is_full(node)) {
		return -ESPIPE;
	}

	i = 0;
	while (i < node->n && node->key[i] < key) {
		i++;
	}

	for (j = node->n; j > i; j--) {
		tree_node_copy_key(node, j - 1, node, j);
	}

	node->n += 1;
	node->key[i] = key;
	node->value[i] = value;
	node->disk_addr = ZFFS_TREE_ADDR_WAIT_WRITE;

	return i;
}

static int tree_split_child(struct zffs_data *zffs,
			    struct zffs_tree_node *node)
{
	struct zffs_tree_node *parent = node->parent;
	struct zffs_tree_node *brothers;
	int rc;
	bool is_retry = false;

	if (!tree_node_is_full(node)) {
		return -ESPIPE;
	}

retry:
	if (parent == NULL) {
		parent = tree_node_alloc();
		if (parent == NULL) {
			rc = -ENOMEM;
			goto done;
		}

		parent->disk_addr = ZFFS_TREE_ADDR_WAIT_WRITE;
		parent->parent = NULL;
		parent->n = 0;
		parent->leaf = 0;
		parent->root = 1;
		memset(parent->loadflag, 0x00, sizeof(parent->loadflag));
		atomic_set_bit(parent->loadflag, 0);
		parent->child[0] = node;
		node->root = 0;
		zffs->tree_root = parent;
		node->parent = parent;
	}

	brothers = tree_node_alloc();
	if (brothers == NULL) {
		rc = -ENOMEM;
		goto done;
	}

	rc = tree_node_insert(parent, node->key[ZFFS_TREE_T - 1],
			      node->value[ZFFS_TREE_T - 1]);
	if (rc < 0) {
		goto err;
	}

	brothers->disk_addr = ZFFS_TREE_ADDR_WAIT_WRITE;
	brothers->parent = parent;
	brothers->leaf = node->leaf;
	brothers->root = 0;
	memset(brothers->loadflag, 0x00, sizeof(parent->loadflag));

	atomic_set_bit(parent->loadflag, rc + 1);
	parent->child[rc + 1] = brothers;

	if (!node->leaf) {
		if (atomic_test_and_clear_bit(node->loadflag, ZFFS_TREE_T)) {
			brothers->child[0] = node->child[ZFFS_TREE_T];
			atomic_set_bit(brothers->loadflag, 0);
			brothers->child[0]->parent = brothers;
		} else {
			brothers->disk_child[0] = node->disk_child[ZFFS_TREE_T];
		}
	}

	for (int i = 0; i < ZFFS_TREE_T - 1; i++) {
		tree_node_copy_key(node, i + ZFFS_TREE_T, brothers, i);
	}

	node->n = ZFFS_TREE_T - 1;
	brothers->n = ZFFS_TREE_T - 1;

	rc = 0;

done:
	if (rc == -ENOMEM && !is_retry) {
		rc = tree_node_free_other_path(zffs, zffs->tree_root, node);

		if (!rc) {
			is_retry = true;
			goto retry;
		}
	}

	return rc;

err:
	tree_node_free(brothers);
	return rc;
}

int zffs_tree_insert(struct zffs_data *zffs, u32_t key, u32_t value)
{
	int i;
	int rc;
	struct zffs_tree_node *node = zffs->tree_root;

	while (true) {
		for (i = 0; i < node->n; i++) {
			if (node->key[i] == key) {
				return -EEXIST;
			} else if (node->key[i] > key) {
				break;
			}
		}

		if (node->leaf) {
			rc = tree_node_insert(node, key, value);
			if (rc < 0) {
				break;
			}
			rc = 0;

			while (node != NULL && tree_node_is_full(node)) {
				rc = tree_split_child(zffs, node);
				if (rc) {
					break;
				}
				node = node->parent;
			}

			break;
		}

		rc = tree_load_child_confirmation(zffs, node, i);
		if (rc) {
			break;
		}

		node = node->child[i];
	}

	return rc;
}

int zffs_tree_sync(struct zffs_data *zffs)
{
	return tree_save_node_recursive(zffs, zffs->tree_root);
}

struct tree_info {
	u32_t key_count;
	u32_t key_max;
	u32_t value_min;
	u32_t value_max;
};

static int tree_info_cb(struct zffs_data *zffs, u32_t key, u32_t value,
			struct tree_info *info)
{
	if (key > info->key_max) {
		info->key_max = key;
	}

	if (value < info->value_min) {
		info->value_min = value;
	}

	if (value > info->value_max) {
		info->value_max = value;
	}

	info->key_count++;
	return 0;
}

int zffs_tree_info(struct zffs_data *zffs, u32_t *key_count, u32_t *key_max,
		   u32_t *value_min, u32_t *value_max)
{
	struct tree_info info = {
		.key_count = 0,
		.key_max = 0,
		.value_min = 0xffffffff,
		.value_max = 0,
	};
	int rc;

	rc = tree_key_foreach(zffs, zffs->tree_root, &info, true, true,
			      (void *)tree_info_cb);
	if (rc) {
		return rc;
	}

	if (key_count) {
		*key_count = info.key_count;
	}

	if (key_max) {
		*key_max = info.key_max;
	}

	if (value_min) {
		*value_min = info.value_min;
	}

	if (value_max) {
		*value_max = info.value_max;
	}

	return 0;
}

int zffs_tree_foreach(struct zffs_data *zffs, void *data,
		      int (*tree_cb)(struct zffs_data *zffs, u32_t key,
				     u32_t value, void *data))
{
	return tree_key_foreach(zffs, zffs->tree_root, data, true, true,
				tree_cb);
}
