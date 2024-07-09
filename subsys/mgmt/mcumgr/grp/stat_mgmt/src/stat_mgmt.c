/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/stats/stats.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <mgmt/mcumgr/util/zcbor_bulk.h>

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/grp/stat_mgmt/stat_mgmt.h>

LOG_MODULE_REGISTER(mcumgr_stat_grp, CONFIG_MCUMGR_GRP_STAT_LOG_LEVEL);

static const struct mgmt_handler stat_mgmt_handlers[];

typedef int stat_mgmt_foreach_entry_fn(zcbor_state_t *zse, struct stat_mgmt_entry *entry);

static int
stats_mgmt_count_plus_one(struct stats_hdr *hdr, void *arg, const char *name, uint16_t off)
{
	size_t *counter = arg;

	(*counter)++;

	return 0;
}

static int
stat_mgmt_count(const char *group_name, size_t *counter)
{
	struct stats_hdr *hdr = stats_group_find(group_name);

	if (hdr == NULL) {
		return MGMT_ERR_ENOENT;
	}

	*counter = 0;

	return stats_walk(hdr, stats_mgmt_count_plus_one, &counter);
}

static int
stat_mgmt_walk_cb(struct stats_hdr *hdr, void *arg, const char *name, uint16_t off);

struct stat_mgmt_walk_arg {
	stat_mgmt_foreach_entry_fn *cb;
	zcbor_state_t *zse;
};

static int
stat_mgmt_walk_cb(struct stats_hdr *hdr, void *arg, const char *name, uint16_t off)
{
	struct stat_mgmt_walk_arg *walk_arg;
	struct stat_mgmt_entry entry;
	void *stat_val;

	walk_arg = arg;

	stat_val = (uint8_t *)hdr + off;
	switch (hdr->s_size) {
	case sizeof(uint16_t):
		entry.value = *(uint16_t *) stat_val;
		break;
	case sizeof(uint32_t):
		entry.value = *(uint32_t *) stat_val;
		break;
	case sizeof(uint64_t):
		entry.value = *(uint64_t *) stat_val;
		break;
	default:
		return STAT_MGMT_ERR_INVALID_STAT_SIZE;
	}

	entry.name = name;

	return walk_arg->cb(walk_arg->zse, &entry);
}

static int
stat_mgmt_foreach_entry(zcbor_state_t *zse, const char *group_name, stat_mgmt_foreach_entry_fn *cb)
{
	struct stat_mgmt_walk_arg walk_arg;
	struct stats_hdr *hdr;

	hdr = stats_group_find(group_name);
	if (hdr == NULL) {
		return STAT_MGMT_ERR_INVALID_GROUP;
	}

	walk_arg = (struct stat_mgmt_walk_arg) {
		.cb = cb,
		.zse = zse
	};

	return stats_walk(hdr, stat_mgmt_walk_cb, &walk_arg);
}

static int
stat_mgmt_cb_encode(zcbor_state_t *zse, struct stat_mgmt_entry *entry)
{
	bool ok = zcbor_tstr_put_term(zse, entry->name, CONFIG_MCUMGR_GRP_STAT_MAX_NAME_LEN) &&
		  zcbor_uint32_put(zse, entry->value);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}

/**
 * Command handler: stat show
 */
static int
stat_mgmt_show(struct smp_streamer *ctxt)
{
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	char stat_name[CONFIG_MCUMGR_GRP_STAT_MAX_NAME_LEN];
	bool ok;
	size_t counter = 0;
	size_t decoded;
	struct zcbor_string name = { 0 };

	struct zcbor_map_decode_key_val stat_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("name", zcbor_tstr_decode, &name),
	};

	ok = zcbor_map_decode_bulk(zsd, stat_decode, ARRAY_SIZE(stat_decode), &decoded) == 0;

	if (!ok || name.len == 0 || name.len >= ARRAY_SIZE(stat_name)) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(stat_name, name.value, name.len);
	stat_name[name.len] = '\0';

	if (stat_mgmt_count(stat_name, &counter) != 0) {
		LOG_ERR("Invalid stat name: %s", stat_name);
		ok = smp_add_cmd_err(zse, ZEPHYR_MGMT_GRP_BASIC,
				     STAT_MGMT_ERR_INVALID_STAT_NAME);
		goto end;
	}

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR)) {
		ok = zcbor_tstr_put_lit(zse, "rc")		&&
		     zcbor_int32_put(zse, MGMT_ERR_EOK);
	}

	if (ok) {
		ok = zcbor_tstr_put_lit(zse, "name")		&&
		     zcbor_tstr_encode(zse, &name)		&&
		     zcbor_tstr_put_lit(zse, "fields")		&&
		     zcbor_map_start_encode(zse, counter);
	}

	if (ok) {
		int rc = stat_mgmt_foreach_entry(zse, stat_name,
						 stat_mgmt_cb_encode);

		if (rc != STAT_MGMT_ERR_OK) {
			if (rc != STAT_MGMT_ERR_INVALID_GROUP &&
			    rc != STAT_MGMT_ERR_INVALID_STAT_SIZE) {
				rc = STAT_MGMT_ERR_WALK_ABORTED;
			}

			ok = smp_add_cmd_err(zse, ZEPHYR_MGMT_GRP_BASIC, rc);
		}
	}

	ok = ok && zcbor_map_end_encode(zse, counter);

end:
	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}

/**
 * Command handler: stat list
 */
static int
stat_mgmt_list(struct smp_streamer *ctxt)
{
	const struct stats_hdr *cur = NULL;
	zcbor_state_t *zse = ctxt->writer->zs;
	bool ok;
	size_t counter = 0;

	do {
		cur = stats_group_get_next(cur);
		if (cur != NULL) {
			counter++;
		}
	} while (cur != NULL);

	ok = zcbor_tstr_put_lit(zse, "rc")		&&
	     zcbor_int32_put(zse, MGMT_ERR_EOK)		&&
	     zcbor_tstr_put_lit(zse, "stat_list")	&&
	     zcbor_list_start_encode(zse, counter);

	if (!ok) {
		return MGMT_ERR_EMSGSIZE;
	}
	/* Iterate the list of stat groups, encoding each group's name in the CBOR
	 * array.
	 */
	cur = NULL;
	do {
		cur = stats_group_get_next(cur);
		if (cur != NULL) {
			ok = zcbor_tstr_put_term(zse, cur->s_name,
						CONFIG_MCUMGR_GRP_STAT_MAX_NAME_LEN);
		}
	} while (ok && cur != NULL);

	if (!ok || !zcbor_list_end_encode(zse, counter)) {
		return MGMT_ERR_EMSGSIZE;
	}

	return 0;
}

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/*
 * @brief	Translate stat mgmt group error code into MCUmgr error code
 *
 * @param ret	#stat_mgmt_err_code_t error code
 *
 * @return	#mcumgr_err_t error code
 */
static int stat_mgmt_translate_error_code(uint16_t err)
{
	int rc;

	switch (err) {
	case STAT_MGMT_ERR_INVALID_GROUP:
	case STAT_MGMT_ERR_INVALID_STAT_NAME:
		rc = MGMT_ERR_ENOENT;
		break;

	case STAT_MGMT_ERR_INVALID_STAT_SIZE:
		rc = MGMT_ERR_EINVAL;
		break;

	case STAT_MGMT_ERR_WALK_ABORTED:
	default:
		rc = MGMT_ERR_EUNKNOWN;
	}

	return rc;
}
#endif

static const struct mgmt_handler stat_mgmt_handlers[] = {
	[STAT_MGMT_ID_SHOW] = { stat_mgmt_show, NULL },
	[STAT_MGMT_ID_LIST] = { stat_mgmt_list, NULL },
};

#define STAT_MGMT_HANDLER_CNT ARRAY_SIZE(stat_mgmt_handlers)

static struct mgmt_group stat_mgmt_group = {
	.mg_handlers = stat_mgmt_handlers,
	.mg_handlers_count = STAT_MGMT_HANDLER_CNT,
	.mg_group_id = MGMT_GROUP_ID_STAT,
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	.mg_translate_error = stat_mgmt_translate_error_code,
#endif
#ifdef CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME
	.mg_group_name = "stat mgmt",
#endif
};

static void stat_mgmt_register_group(void)
{
	mgmt_register_group(&stat_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(stat_mgmt, stat_mgmt_register_group);
