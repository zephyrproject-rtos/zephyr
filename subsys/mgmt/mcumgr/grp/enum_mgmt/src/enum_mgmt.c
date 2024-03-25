/**
 * Copyright (c) - 2024 Extreme Engineering Solutions
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/object_tracing.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/grp/enum_mgmt/enum_mgmt.h>
#include <zephyr/logging/log.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>

static int enumeration_mgmt_list(struct smp_streamer *ctxt)
{
	int count;
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok;

	count = mgmt_get_num_groups();
	int ids[count];

	count = mgmt_get_group_ids(ids);

	if (count == 0) {
		return MGMT_ERR_ENOTSUP;
	}

	ok = zcbor_tstr_put_lit(zse, "groups") &&
		zcbor_list_start_encode(zse, count);

	for (int i = 0; i < count; ++i) {
		ok = zcbor_uint32_put(zse, ids[i]);
		if (!ok) {
			return MGMT_ERR_EBADSTATE;
		}
	}

	ok = zcbor_list_end_encode(zse, count);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ECORRUPT;
}

static int enumeration_mgmt_info(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs
	bool ok;

	ok = zcbor_tstr_put_lit(zse, "name") &&
		zcbor_tstr_put_lit(zse, ENUMERATION_MGMT_NAME) &&
		zcbor_tstr_put_lit(zse, "rev") &&
		zcbor_uint32_put(zse, ENUMERATION_MGMT_VER);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ECORRUPT;
}

static const struct mgmt_handler enumeration_mgmt_group_handlers[] = {
	[ENUMERATION_MGMT_LIST] = {enumeration_mgmt_list, NULL},
	[ENUMERATION_MGMT_INFO] = {enumeration_mgmt_info, NULL},
};

static struct mgmt_group enumeration_mgmt_group = {
	.mg_handlers = enumeration_mgmt_group_handlers,
	.mg_handlers_count = ENUMERATION_NUM_CMDS,
	.mg_group_id = ENUMERATION_MGMT_GP_ID,
};

void enumeration_mgmt_register_group(void)
{
	mgmt_register_group(&enumeration_mgmt_group);
}

void enumeration_mgmt_unregister_group(void)
{
	mgmt_unregister_group(&enumeration_mgmt_group);
}
