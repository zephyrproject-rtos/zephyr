/**
 * Copyright (c) 2024 Extreme Engineering Solutions
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/grp/enum_mgmt/enum_mgmt.h>

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>

struct enum_mgmt_list_arg {
	bool *ok;
	zcbor_state_t *zse;
};

static bool enum_mgmt_cb_count(const struct mgmt_group *group, void *user_data)
{
	uint16_t *count = (uint16_t *)user_data;

	++*count;

	return true;
}

static bool enum_mgmt_cb_list(const struct mgmt_group *group, void *user_data)
{
	struct enum_mgmt_list_arg *data = (struct enum_mgmt_list_arg *)user_data;

	*data->ok = zcbor_uint32_put(data->zse, group->mg_group_id);

	return *data->ok;
}

static int enum_mgmt_count(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	uint16_t count = 0;
	bool ok;

	mgmt_groups_foreach(enum_mgmt_cb_count, (void *)&count);

	ok = zcbor_tstr_put_lit(zse, "groups") &&
	     zcbor_uint32_put(zse, count);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ECORRUPT;
}

static int enum_mgmt_list(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	uint16_t count = 0;
	bool ok;
	struct enum_mgmt_list_arg arguments = {
		.ok = &ok,
		.zse = zse
	};

	mgmt_groups_foreach(enum_mgmt_cb_count, (void *)&count);

	ok = zcbor_tstr_put_lit(zse, "groups") &&
	     zcbor_list_start_encode(zse, count);

	if (!ok) {
		goto failure;
	}

	mgmt_groups_foreach(enum_mgmt_cb_list, (void *)&arguments);

	if (!ok) {
		goto failure;
	}

	ok = zcbor_list_end_encode(zse, count);

failure:
	return ok ? MGMT_ERR_EOK : MGMT_ERR_ECORRUPT;
}

static const struct mgmt_handler enum_mgmt_group_handlers[] = {
	[ENUM_MGMT_ID_COUNT] = { enum_mgmt_count, NULL },
	[ENUM_MGMT_ID_LIST] = { enum_mgmt_list, NULL },
};

static struct mgmt_group enum_mgmt_group = {
	.mg_handlers = enum_mgmt_group_handlers,
	.mg_handlers_count = ARRAY_SIZE(enum_mgmt_group_handlers),
	.mg_group_id = MGMT_GROUP_ID_ENUM,
};

static void enum_mgmt_register_group(void)
{
	mgmt_register_group(&enum_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(enum_mgmt, enum_mgmt_register_group);
