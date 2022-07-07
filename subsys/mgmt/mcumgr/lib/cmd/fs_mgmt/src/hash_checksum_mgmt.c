/*
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <string.h>
#include "mgmt/mgmt.h"
#include "fs_mgmt/hash_checksum_mgmt.h"

static sys_slist_t hash_checksum_mgmt_group_list =
	SYS_SLIST_STATIC_INIT(&hash_checksum_mgmt_group_list);

void hash_checksum_mgmt_unregister_group(struct hash_checksum_mgmt_group *group)
{
	(void)sys_slist_find_and_remove(&hash_checksum_mgmt_group_list, &group->node);
}

void hash_checksum_mgmt_register_group(struct hash_checksum_mgmt_group *group)
{
	sys_slist_append(&hash_checksum_mgmt_group_list, &group->node);
}

const struct hash_checksum_mgmt_group *hash_checksum_mgmt_find_handler(const char *name)
{
	/* Find the group with the specified group name */
	struct hash_checksum_mgmt_group *group = NULL;
	sys_snode_t *snp, *sns;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&hash_checksum_mgmt_group_list, snp, sns) {
		struct hash_checksum_mgmt_group *loop_group =
			CONTAINER_OF(snp, struct hash_checksum_mgmt_group, node);
		if (strcmp(loop_group->group_name, name) == 0) {
			group = loop_group;
			break;
		}
	}

	return group;
}
