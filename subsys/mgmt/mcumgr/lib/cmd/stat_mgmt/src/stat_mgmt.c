/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <string.h>
#include <stdio.h>

#include <stats/stats.h>
#include <mgmt/mgmt.h>
#include <stat_mgmt/stat_mgmt_config.h>
#include <stat_mgmt/stat_mgmt.h>
#include "cborattr/cborattr.h"

static struct mgmt_handler stat_mgmt_handlers[];

typedef int stat_mgmt_foreach_entry_fn(CborEncoder *enc, struct stat_mgmt_entry *entry);

static int
stat_mgmt_walk_cb(struct stats_hdr *hdr, void *arg, const char *name, uint16_t off);

struct stat_mgmt_walk_arg {
	stat_mgmt_foreach_entry_fn *cb;
	CborEncoder *enc;
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
		return MGMT_ERR_EUNKNOWN;
	}
	entry.name = name;

	return walk_arg->cb(walk_arg->enc, &entry);
}

static int
stat_mgmt_get_group(int idx, const char **out_name)
{
	const struct stats_hdr *cur;
	int i;

	cur = NULL;
	for (i = 0; i <= idx; i++) {
		cur = stats_group_get_next(cur);
		if (cur == NULL) {
			return MGMT_ERR_ENOENT;
		}
	}

	*out_name = cur->s_name;
	return 0;
}


static int
stat_mgmt_foreach_entry(const char *group_name, stat_mgmt_foreach_entry_fn *cb, CborEncoder *enc)
{
	struct stat_mgmt_walk_arg walk_arg;
	struct stats_hdr *hdr;

	hdr = stats_group_find(group_name);
	if (hdr == NULL) {
		return MGMT_ERR_ENOENT;
	}

	walk_arg = (struct stat_mgmt_walk_arg) {
		.cb = cb,
		.enc = enc
	};

	return stats_walk(hdr, stat_mgmt_walk_cb, &walk_arg);
}

static int
stat_mgmt_cb_encode(CborEncoder *enc, struct stat_mgmt_entry *entry)
{
	CborError err;

	err = 0;
	err |= cbor_encode_text_stringz(enc, entry->name);
	err |= cbor_encode_uint(enc, entry->value);

	if (err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	return 0;
}

/**
 * Command handler: stat show
 */
static int
stat_mgmt_show(struct mgmt_ctxt *ctxt)
{
	char stat_name[STAT_MGMT_MAX_NAME_LEN];
	CborEncoder map_enc;
	CborError err;
	int rc;

	struct cbor_attr_t attrs[] = {
		{
			.attribute = "name",
			.type = CborAttrTextStringType,
			.addr.string = stat_name,
			.len = sizeof(stat_name)
		},
		{ NULL },
	};

	err = cbor_read_object(&ctxt->it, attrs);
	if (err != 0) {
		return MGMT_ERR_EINVAL;
	}

	err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
	err |= cbor_encode_int(&ctxt->encoder, MGMT_ERR_EOK);

	err |= cbor_encode_text_stringz(&ctxt->encoder, "name");
	err |= cbor_encode_text_stringz(&ctxt->encoder, stat_name);

	err |= cbor_encode_text_stringz(&ctxt->encoder, "fields");
	err |= cbor_encoder_create_map(&ctxt->encoder, &map_enc, CborIndefiniteLength);

	rc = stat_mgmt_foreach_entry(stat_name, stat_mgmt_cb_encode, &map_enc);

	err |= cbor_encoder_close_container(&ctxt->encoder, &map_enc);
	if (err != 0) {
		rc = MGMT_ERR_ENOMEM;
	}

	return rc;
}

/**
 * Command handler: stat list
 */
static int
stat_mgmt_list(struct mgmt_ctxt *ctxt)
{
	const char *group_name;
	CborEncoder arr_enc;
	CborError err;
	int rc;
	int i;

	err = CborNoError;
	err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
	err |= cbor_encode_int(&ctxt->encoder, MGMT_ERR_EOK);
	err |= cbor_encode_text_stringz(&ctxt->encoder, "stat_list");
	err |= cbor_encoder_create_array(&ctxt->encoder, &arr_enc, CborIndefiniteLength);

	/* Iterate the list of stat groups, encoding each group's name in the CBOR
	 * array.
	 */
	for (i = 0; ; i++) {
		rc = stat_mgmt_get_group(i, &group_name);
		if (rc == MGMT_ERR_ENOENT) {
			/* No more stat groups. */
			break;
		} else if (rc != 0) {
			/* Error. */
			cbor_encoder_close_container(&ctxt->encoder, &arr_enc);
			return rc;
		}

		err |= cbor_encode_text_stringz(&ctxt->encoder, group_name);
	}
	err |= cbor_encoder_close_container(&ctxt->encoder, &arr_enc);

	if (err != 0) {
		return MGMT_ERR_ENOMEM;
	}
	return 0;
}

static struct mgmt_handler stat_mgmt_handlers[] = {
	[STAT_MGMT_ID_SHOW] = { stat_mgmt_show, NULL },
	[STAT_MGMT_ID_LIST] = { stat_mgmt_list, NULL },
};

#define STAT_MGMT_HANDLER_CNT ARRAY_SIZE(stat_mgmt_handlers)

static struct mgmt_group stat_mgmt_group = {
	.mg_handlers = stat_mgmt_handlers,
	.mg_handlers_count = STAT_MGMT_HANDLER_CNT,
	.mg_group_id = MGMT_GROUP_ID_STAT,
};

void
stat_mgmt_register_group(void)
{
	mgmt_register_group(&stat_mgmt_group);
}
