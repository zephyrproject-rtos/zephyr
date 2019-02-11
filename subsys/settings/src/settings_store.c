/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <errno.h>
#include <misc/__assert.h>

#include "settings/settings.h"
#include "settings_priv.h"

struct settings_dup_check_arg {
	const char *name;
	const char *val;
	size_t val_len;
	int is_dup;
};

sys_slist_t  settings_load_srcs;
struct settings_store *settings_save_dst;

void settings_src_register(struct settings_store *cs)
{
	sys_snode_t *prev, *cur;

	prev = NULL;

	SYS_SLIST_FOR_EACH_NODE(&settings_load_srcs, cur) {
		prev = cur;
	}

	sys_slist_insert(&settings_load_srcs, prev, &cs->cs_next);
}

void settings_dst_register(struct settings_store *cs)
{
	settings_save_dst = cs;
}

static void settings_load_cb(char *name, void *val_read_cb_ctx, off_t off,
			     void *cb_arg)
{
	 int rc = settings_set_value_priv(name, val_read_cb_ctx, off, 0);
	__ASSERT(rc == 0, "set-value operation failure\n");
	(void)rc;
}

int settings_load(void)
{
	struct settings_store *cs;

	/*
	 * for every config store
	 *    load config
	 *    apply config
	 *    commit all
	 */

	SYS_SLIST_FOR_EACH_CONTAINER(&settings_load_srcs, cs, cs_next) {
		cs->cs_itf->csi_load(cs, settings_load_cb, NULL);
	}
	return settings_commit(NULL);
}

/* val_off - offset of value-string within line entries */
static int settings_cmp(char const *val, size_t val_len, void *val_read_cb_ctx,
		 off_t val_off)
{
	size_t len_read, exp_len;
	size_t rem;
	char buf[16];
	int rc;
	off_t off = 0;

	for (rem = val_len; rem > 0; rem -= len_read) {
		len_read = exp_len = MIN(sizeof(buf), rem);
		rc = settings_line_val_read(val_off, off, buf, len_read,
			   &len_read, val_read_cb_ctx);
		if (rc) {
			break;
		}

		if (len_read != exp_len) {
			rc = 1;
			break;
		}

		rc = memcmp(val, buf, len_read);
		if (rc) {
			break;
		}
		val += len_read;
		off += len_read;
	}

	return rc;
}

static void settings_dup_check_cb(char *name, void *val_read_cb_ctx, off_t off,
				  void *cb_arg)
{
	struct settings_dup_check_arg *cdca = (struct settings_dup_check_arg *)
					      cb_arg;
	size_t len_read;

	if (strcmp(name, cdca->name)) {
		return;
	}

	len_read = settings_line_val_get_len(off, val_read_cb_ctx);
	if (len_read == 0) {
		/* treat as an empty entry */
		if (!cdca->val || cdca->val_len == 0) {
			cdca->is_dup = 1;
		} else {
			cdca->is_dup = 0;
		}
	} else {
		if (cdca->val && !settings_cmp(cdca->val, cdca->val_len,
					       val_read_cb_ctx, off)) {
			cdca->is_dup = 1;
		} else {
			cdca->is_dup = 0;
		}
	}
}

/*
 * Append a single value to persisted config. Don't store duplicate value.
 */
int settings_save_one(const char *name, void *value, size_t val_len)
{
	struct settings_store *cs;
	struct settings_dup_check_arg cdca;

	cs = settings_save_dst;
	if (!cs) {
		return -ENOENT;
	}

	/*
	 * Check if we're writing the same value again.
	 */
	cdca.name = name;
	cdca.val = (char *)value;
	cdca.is_dup = 0;
	cdca.val_len = val_len;
	cs->cs_itf->csi_load(cs, settings_dup_check_cb, &cdca);
	if (cdca.is_dup == 1) {
		return 0;
	}
	return cs->cs_itf->csi_save(cs, name, (char *)value, val_len);
}

int settings_delete(const char *name)
{
	return settings_save_one(name, NULL, 0);
}

int settings_save(void)
{
	struct settings_store *cs;
	struct settings_handler *ch;
	int rc;
	int rc2;

	cs = settings_save_dst;
	if (!cs) {
		return -ENOENT;
	}

	if (cs->cs_itf->csi_save_start) {
		cs->cs_itf->csi_save_start(cs);
	}
	rc = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
		if (ch->h_export) {
			rc2 = ch->h_export(settings_save_one);
			if (!rc) {
				rc = rc2;
			}
		}
	}
	if (cs->cs_itf->csi_save_end) {
		cs->cs_itf->csi_save_end(cs);
	}
	return rc;
}

void settings_store_init(void)
{
	sys_slist_init(&settings_load_srcs);
}
