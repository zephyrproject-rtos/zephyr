/*
 * Copyright (c) 2022 Laird Connectivity
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>

#include <zephyr/mgmt/mcumgr/grp/fs_mgmt/fs_mgmt_hash_checksum.h>

static sys_slist_t fs_mgmt_hash_checksum_group_list =
	SYS_SLIST_STATIC_INIT(&fs_mgmt_hash_checksum_group_list);

void fs_mgmt_hash_checksum_unregister_group(struct fs_mgmt_hash_checksum_group *group)
{
	(void)sys_slist_find_and_remove(&fs_mgmt_hash_checksum_group_list, &group->node);
}

void fs_mgmt_hash_checksum_register_group(struct fs_mgmt_hash_checksum_group *group)
{
	sys_slist_append(&fs_mgmt_hash_checksum_group_list, &group->node);
}

const struct fs_mgmt_hash_checksum_group *fs_mgmt_hash_checksum_find_handler(const char *name)
{
	/* Find the group with the specified group name */
	struct fs_mgmt_hash_checksum_group *group = NULL;
	sys_snode_t *snp, *sns;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&fs_mgmt_hash_checksum_group_list, snp, sns) {
		struct fs_mgmt_hash_checksum_group *loop_group =
			CONTAINER_OF(snp, struct fs_mgmt_hash_checksum_group, node);
		if (strcmp(loop_group->group_name, name) == 0) {
			group = loop_group;
			break;
		}
	}

	return group;
}

void fs_mgmt_hash_checksum_find_handlers(fs_mgmt_hash_checksum_list_cb cb, void *user_data)
{
	/* Run a callback with all supported hash/checksums */
	sys_snode_t *snp, *sns;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&fs_mgmt_hash_checksum_group_list, snp, sns) {
		struct fs_mgmt_hash_checksum_group *group =
			CONTAINER_OF(snp, struct fs_mgmt_hash_checksum_group, node);
		cb(group, user_data);
	}
}
