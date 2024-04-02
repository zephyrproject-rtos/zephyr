/**
 * Copyright (c) - 2024 Extreme Engineering Solutions
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/logging/log.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>

#define ENUMERATION_MGMT_LIST   0
#define ENUMERATION_MGMT_GP_ID  10

static bool encode_id(sys_snode_t *group_node, zcbor_state_t *zse) {
	struct mgmt_group *group = CONTAINER_OF(group_node, struct mgmt_group, node);
	return zcbor_uint32_put(group->mg_group_id);
}

static int enumeration_mgmt_list(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	sys_slist_t *groups;
	sys_snode_t *snp, *sns;
	int count;

	groups = mgmt_get_group_list();
	count = sys_slist_len(groups);

	ok = zcbor_tstr_put_lit(zse, "groups") &&
		zcbor_list_start_encode(zse, count);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&groups, snp, sns) {
		ok = encode_id(sns, zse);
		if (!ok) {
			return MGMT_ERR_EBADSTATE;
		}
	}

	ok = zcbor_list_end_encode(zse, count);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ECORRUPT;
}

static const struct mgmt_handler enumeration_mgmt_group_handlers[] = {
	[ENUMERATION_MGMT_LIST] = {enumeration_mgmt_list, NULL},
};

static struct mgmt_group enumeration_mgmt_group = {
	.mg_handlers = enumeration_mgmt_group_handlers,
	.mg_handlers_count = ARRAY_SIZE(enumeration_mgmt_group_handlers),
	.mg_group_id = ENUMERATION_MGMT_GP_ID,
};

static void enumeration_mgmt_register_group(void)
{
	mgmt_register_group(&enumeration_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(enumerationg_mgmt, enumeration_mgmt_register_group);
