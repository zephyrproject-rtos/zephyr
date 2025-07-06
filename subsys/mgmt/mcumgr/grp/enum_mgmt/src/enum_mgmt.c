/**
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/grp/enum_mgmt/enum_mgmt.h>
#include <zephyr/logging/log.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>

#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>

#if defined(CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS)
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#endif

LOG_MODULE_REGISTER(mcumgr_enum_grp, CONFIG_MCUMGR_GRP_ENUM_LOG_LEVEL);

#define MAX_MCUMGR_GROUPS 65535

struct enum_mgmt_single_arg {
	uint32_t index;
	uint32_t current_index;
	uint32_t group;
	bool found;
	bool last;
};

struct enum_mgmt_list_arg {
	bool *ok;
	zcbor_state_t *zse;
};

struct enum_mgmt_details_arg {
	bool *ok;
	zcbor_state_t *zse;
	uint16_t *allowed_group_ids;
	uint16_t allowed_group_id_size;
#if defined(CONFIG_MCUMGR_GRP_ENUM_DETAILS_HOOK)
	enum mgmt_cb_return status;
	int32_t err_rc;
	uint16_t err_group;
#endif
};

static bool enum_mgmt_cb_count(const struct mgmt_group *group, void *user_data)
{
	uint16_t *count = (uint16_t *)user_data;

	++*count;

	return true;
}

static bool enum_mgmt_cb_single(const struct mgmt_group *group, void *user_data)
{
	struct enum_mgmt_single_arg *data = (struct enum_mgmt_single_arg *)user_data;

	if (data->index == data->current_index) {
		data->found = true;
		data->group = group->mg_group_id;
		++data->current_index;
		data->last = true;
	} else {
		if (data->found == true && data->current_index == (data->index + 1)) {
			data->last = false;
			return false;
		}

		++data->current_index;
	}

	return true;
}

static bool enum_mgmt_cb_list(const struct mgmt_group *group, void *user_data)
{
	struct enum_mgmt_list_arg *data = (struct enum_mgmt_list_arg *)user_data;

	*data->ok = zcbor_uint32_put(data->zse, group->mg_group_id);

	return *data->ok;
}

#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS
static bool enum_mgmt_cb_details(const struct mgmt_group *group, void *user_data)
{
	struct enum_mgmt_details_arg *data = (struct enum_mgmt_details_arg *)user_data;

#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME
	const char *group_name = group->mg_group_name;
	int group_name_size = 0;
#endif

#if defined(CONFIG_MCUMGR_GRP_ENUM_DETAILS_HOOK)
	struct enum_mgmt_detail_output group_detail_data = {
		.group = group,
		.zse = data->zse,
	};
#endif

	if (data->allowed_group_ids != NULL && data->allowed_group_id_size > 0) {
		/* Check if this entry is in the allow list */
		uint16_t i = 0;

		while (i < data->allowed_group_id_size) {
			if (data->allowed_group_ids[i] == group->mg_group_id) {
				break;
			}

			++i;
		}

		if (i == data->allowed_group_id_size) {
			*data->ok = true;
			goto finished;
		}
	}

#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME
	if (group_name != NULL) {
		group_name_size = strlen(group_name);
	}
#endif

	*data->ok = zcbor_map_start_encode(data->zse, CONFIG_MCUMGR_GRP_ENUM_DETAILS_STATES) &&
		    zcbor_tstr_put_lit(data->zse, "group") &&
		    zcbor_uint32_put(data->zse, group->mg_group_id) &&
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME
		    zcbor_tstr_put_lit(data->zse, "name") &&
		    zcbor_tstr_encode_ptr(data->zse, group_name, group_name_size) &&
#endif
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_HANDLERS
		    zcbor_tstr_put_lit(data->zse, "handlers") &&
		    zcbor_uint32_put(data->zse, group->mg_handlers_count) &&
#endif
		    1;

#if defined(CONFIG_MCUMGR_GRP_ENUM_DETAILS_HOOK)
	/* Send notification to application to optionally append more fields */
	data->status = mgmt_callback_notify(MGMT_EVT_OP_ENUM_MGMT_DETAILS, &group_detail_data,
				      sizeof(group_detail_data), &data->err_rc, &data->err_group);

	if (data->status != MGMT_CB_OK) {
		*data->ok = false;

		return false;
	}
#endif

	if (*data->ok) {
		*data->ok = zcbor_map_end_encode(data->zse, CONFIG_MCUMGR_GRP_ENUM_DETAILS_STATES);
	}

finished:
	return *data->ok;
}
#endif

static int enum_mgmt_count(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	uint32_t count = 0;
	bool ok;

	mgmt_groups_foreach(enum_mgmt_cb_count, (void *)&count);

	ok = zcbor_tstr_put_lit(zse, "count") &&
	     zcbor_uint32_put(zse, count);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ECORRUPT;
}

static int enum_mgmt_list(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	uint32_t count = 0;
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

static int enum_mgmt_single(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	size_t decoded = 0;
	bool ok;
	struct enum_mgmt_single_arg arguments = {
		.index = 0,
		.current_index = 0,
		.group = 0,
		.found = false,
		.last = false
	};

	struct zcbor_map_decode_key_val enum_single_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("index", zcbor_uint32_decode, &arguments.index),
	};

	ok = zcbor_map_decode_bulk(zsd, enum_single_decode, ARRAY_SIZE(enum_single_decode),
				   &decoded) == 0;

	if (!ok || arguments.index > MAX_MCUMGR_GROUPS) {
		return MGMT_ERR_EINVAL;
	}

	mgmt_groups_foreach(enum_mgmt_cb_single, &arguments);

	if (!arguments.found) {
		/* Out of range index supplied */
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_ENUM, ENUM_MGMT_ERR_INDEX_TOO_LARGE);
	} else {
		ok = zcbor_tstr_put_lit(zse, "group") &&
		     zcbor_uint32_put(zse, arguments.group);

		if (arguments.last) {
			ok &= zcbor_tstr_put_lit(zse, "end") &&
			      zcbor_bool_put(zse, true);
		}
	}

	return ok ? MGMT_ERR_EOK : MGMT_ERR_ECORRUPT;
}

#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS
static bool enum_mgmt_cb_count_entries(zcbor_state_t *state, void *user_data)
{
	size_t *entries = (size_t *)user_data;

	if (!zcbor_list_start_decode(state)) {
		return true;
	}

	while (!zcbor_array_at_end(state)) {
		++*entries;
		zcbor_any_skip(state, NULL);
	}

	(void)zcbor_list_end_decode(state);

	return true;
}

static bool enum_mgmt_cb_list_entries(zcbor_state_t *state, void *user_data)
{
	uint32_t temp = 0;
	uint16_t i = 0;
	uint16_t *list = (uint16_t *)user_data;

	if (!zcbor_list_start_decode(state)) {
		return true;
	}

	while (!zcbor_array_at_end(state)) {
		if (!zcbor_uint32_decode(state, &temp)) {
			return false;
		}

		list[i] = (uint16_t)temp;
		++i;
	}

	(void)zcbor_list_end_decode(state);

	return true;
}

static int enum_mgmt_details(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	uint32_t count = 0;
	bool ok;
	size_t backup_element_count_reader = zsd->elem_count;
	size_t entries = 0;
	size_t decoded = 0;
	struct enum_mgmt_details_arg arguments = {
		.ok = &ok,
		.zse = zse,
		.allowed_group_ids = NULL,
		.allowed_group_id_size = 0,
#if defined(CONFIG_MCUMGR_GRP_ENUM_DETAILS_HOOK)
		.status = MGMT_CB_OK,
#endif
	};

	if (!zcbor_new_backup(zsd, backup_element_count_reader)) {
		LOG_ERR("Failed to create zcbor backup");
		return MGMT_ERR_ENOMEM;
	}

	struct zcbor_map_decode_key_val enum_group_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("groups", enum_mgmt_cb_count_entries, &entries),
	};

	ok = zcbor_map_decode_bulk(zsd, enum_group_decode, ARRAY_SIZE(enum_group_decode),
				   &decoded) == 0;

	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

	if (!zcbor_process_backup(zsd, (ZCBOR_FLAG_RESTORE | ZCBOR_FLAG_CONSUME),
				  backup_element_count_reader)) {
		LOG_ERR("Failed to restore zcbor reader backup");
		return MGMT_ERR_ENOMEM;
	}

#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_BUFFER_TYPE_STACK
	if (entries > CONFIG_MCUMGR_GRP_ENUM_DETAILS_BUFFER_TYPE_STACK_ENTRIES) {
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_ENUM,
				     ENUM_MGMT_ERR_TOO_MANY_GROUP_ENTRIES);
		goto failure;
	}
#endif

	if (entries > 0) {
		/* Return details on selected groups only */
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_BUFFER_TYPE_HEAP
		uint16_t *entry_list = malloc(sizeof(uint16_t) * entries);
#else
		uint16_t entry_list[CONFIG_MCUMGR_GRP_ENUM_DETAILS_BUFFER_TYPE_STACK_ENTRIES];
#endif

#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_BUFFER_TYPE_HEAP
		if (entry_list == NULL) {
			ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_ENUM,
					     ENUM_MGMT_ERR_INSUFFICIENT_HEAP_FOR_ENTRIES);
			goto failure;
		}
#endif

		enum_group_decode[0].decoder = enum_mgmt_cb_list_entries;
		enum_group_decode[0].value_ptr = entry_list;
		enum_group_decode[0].found = false;
		arguments.allowed_group_ids = entry_list;
		arguments.allowed_group_id_size = (uint16_t)entries;

		ok = zcbor_map_decode_bulk(zsd, enum_group_decode, ARRAY_SIZE(enum_group_decode),
					   &decoded) == 0;

		if (!ok) {
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_BUFFER_TYPE_HEAP
			free(entry_list);
#endif

			return MGMT_ERR_EINVAL;
		}

		ok = zcbor_tstr_put_lit(zse, "groups") &&
		     zcbor_list_start_encode(zse, count);

		if (!ok) {
			goto cleanup;
		}

		mgmt_groups_foreach(enum_mgmt_cb_details, (void *)&arguments);

cleanup:
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_BUFFER_TYPE_HEAP
		free(entry_list);
#endif

		if (!ok) {
			goto failure;
		}
	} else {
		/* Return details on all groups */
		mgmt_groups_foreach(enum_mgmt_cb_count, (void *)&count);

		ok = zcbor_tstr_put_lit(zse, "groups") &&
		     zcbor_list_start_encode(zse, count);

		if (!ok) {
			goto failure;
		}

		mgmt_groups_foreach(enum_mgmt_cb_details, (void *)&arguments);

		if (!ok) {
			goto failure;
		}
	}

#if defined(CONFIG_MCUMGR_GRP_ENUM_DETAILS_HOOK)
	if (arguments.status == MGMT_CB_ERROR_RC) {
		return arguments.err_rc;
	} else if (arguments.status == MGMT_CB_ERROR_ERR) {
		/* Because there is already data in the buffer, it must be cleared first */
		net_buf_reset(ctxt->writer->nb);
		ctxt->writer->nb->len = sizeof(struct smp_hdr);
		zcbor_new_encode_state(zse, ARRAY_SIZE(ctxt->writer->zs),
				       ctxt->writer->nb->data + sizeof(struct smp_hdr),
				       net_buf_tailroom(ctxt->writer->nb), 0);
		ok = zcbor_map_start_encode(zse, CONFIG_MCUMGR_SMP_CBOR_MAX_MAIN_MAP_ENTRIES) &&
		     smp_add_cmd_err(zse, arguments.err_group, (uint16_t)arguments.err_rc);

		goto failure;
	}
#endif

	ok = zcbor_list_end_encode(zse, count);

failure:
	return ok ? MGMT_ERR_EOK : MGMT_ERR_ECORRUPT;
}
#endif

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/*
 * @brief       Translate enum mgmt group error code into MCUmgr error code
 *
 * @param ret   #enum_mgmt_err_code_t error code
 *
 * @return      #mcumgr_err_t error code
 */
static int enum_mgmt_translate_error_code(uint16_t err)
{
	int rc;

	switch (err) {
	case ENUM_MGMT_ERR_TOO_MANY_GROUP_ENTRIES:
		rc = MGMT_ERR_EINVAL;
		break;

	case ENUM_MGMT_ERR_UNKNOWN:
	default:
		rc = MGMT_ERR_EUNKNOWN;
	}

	return rc;
}
#endif

static const struct mgmt_handler enum_mgmt_group_handlers[] = {
	[ENUM_MGMT_ID_COUNT] = { enum_mgmt_count, NULL },
	[ENUM_MGMT_ID_LIST] = { enum_mgmt_list, NULL },
	[ENUM_MGMT_ID_SINGLE] = { enum_mgmt_single, NULL },
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS
	[ENUM_MGMT_ID_DETAILS] = { enum_mgmt_details, NULL },
#endif
};

static struct mgmt_group enum_mgmt_group = {
	.mg_handlers = enum_mgmt_group_handlers,
	.mg_handlers_count = ARRAY_SIZE(enum_mgmt_group_handlers),
	.mg_group_id = MGMT_GROUP_ID_ENUM,
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	.mg_translate_error = enum_mgmt_translate_error_code,
#endif
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME
	.mg_group_name = "enum mgmt",
#endif
};

static void enum_mgmt_register_group(void)
{
	mgmt_register_group(&enum_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(enum_mgmt, enum_mgmt_register_group);
