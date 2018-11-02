/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zffs/path.h"
#include <string.h>

// int zffs_path_find_child_step(struct zffs_data *zffs,
// 			       struct zffs_area_pointer pointer,
// 			       const char **path,
// 			       struct zffs_node_data *node_data)
// {
// 	if (**path == '/') {
// 	}
// }

int zffs_path_step(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, const char **path,
		    struct zffs_node_data *node_data, sys_snode_t **snode)
{
	char name[ZFFS_CONFIG_NAME_MAX + 1];
	char *next_path = strchr(*path, '/');
	int rc;

	if (next_path) {
		int len = next_path - *path;
		strncpy(name, *path, len);
		name[len] = '\0';
		next_path ++;
	}
	else {
		strcpy(name, *path);
	}

	rc = zffs_dir_search_for_node_data(zffs, pointer, node_data, name,
					    snode);
	if (rc) {
		return rc;
	}

	*path = next_path;
	return 0;
}

// int zffs_path_find_step(struct zffs_data *zffs,
// 			 struct zffs_area_pointer *pointer, const char **path,
// 			 struct zffs_dir *dir)
// {
// 	int rc;
// 	char name[ZFFS_CONFIG_NAME_MAX + 1];
// 	struct zffs_dir parent;
// 	const char *path_next = *path;
// 	name[ZFFS_CONFIG_NAME_MAX] = '\0';

// 	if (**path == '/') {
// 		rc = zffs_dir_open(zffs, pointer, NULL, dir, NULL);
// 		if (rc) {
// 			return rc;
// 		}

// 		(*path)++;
// 		return 0;
// 	}

// 	for (int i = 0; i <= ZFFS_CONFIG_NAME_MAX; i++) {
// 		name[i] = *(path_next++);
// 		if (name[i] == '/') {
// 			name[i] = '\0';
// 			break;
// 		}
// 	}

// 	parent = *dir;
// 	rc = zffs_dir_open(zffs, pointer, &parent, dir, name);
// 	if (rc) {
// 		return rc;
// 	}

// 	rc = zffs_dir_close(zffs, pointer, &parent);
// 	if (rc) {
// 		return rc;
// 	}

// 	*path = path_next;

// 	return 0;
// }
