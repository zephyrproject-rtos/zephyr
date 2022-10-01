/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "mgmt/mgmt.h"
#include <zephyr/mgmt/mcumgr/buf.h>
#include <zcbor_encode.h>
#include <zcbor_common.h>

static mgmt_on_evt_cb evt_cb;
static sys_slist_t mgmt_group_list =
	SYS_SLIST_STATIC_INIT(&mgmt_group_list);

void
mgmt_unregister_group(struct mgmt_group *group)
{
	(void)sys_slist_find_and_remove(&mgmt_group_list, &group->node);
}

const struct mgmt_handler *
mgmt_find_handler(uint16_t group_id, uint16_t command_id)
{
	struct mgmt_group *group = NULL;
	sys_snode_t *snp, *sns;

	/*
	 * Find the group with the specified group id, if one exists
	 * check the handler for the command id and make sure
	 * that is not NULL. If that is not set, look for the group
	 * with a command id that is set
	 */
	SYS_SLIST_FOR_EACH_NODE_SAFE(&mgmt_group_list, snp, sns) {
		struct mgmt_group *loop_group =
			CONTAINER_OF(snp, struct mgmt_group, node);
		if (loop_group->mg_group_id == group_id) {
			if (command_id >= loop_group->mg_handlers_count) {
				break;
			}

			if (!loop_group->mg_handlers[command_id].mh_read &&
				!loop_group->mg_handlers[command_id].mh_write) {
				continue;
			}

			group = loop_group;
			break;
		}
	}

	if (group == NULL) {
		return NULL;
	}

	return &group->mg_handlers[command_id];
}

void
mgmt_register_group(struct mgmt_group *group)
{
	sys_slist_append(&mgmt_group_list, &group->node);
}

void
mgmt_ntoh_hdr(struct mgmt_hdr *hdr)
{
	hdr->nh_len = sys_be16_to_cpu(hdr->nh_len);
	hdr->nh_group = sys_be16_to_cpu(hdr->nh_group);
}

void
mgmt_hton_hdr(struct mgmt_hdr *hdr)
{
	hdr->nh_len = sys_cpu_to_be16(hdr->nh_len);
	hdr->nh_group = sys_cpu_to_be16(hdr->nh_group);
}

void
mgmt_register_evt_cb(mgmt_on_evt_cb cb)
{
	evt_cb = cb;
}

void
mgmt_evt(uint8_t opcode, uint16_t group, uint8_t id, void *arg)
{
	if (evt_cb) {
		evt_cb(opcode, group, id, arg);
	}
}
