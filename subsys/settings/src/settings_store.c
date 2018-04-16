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

static void settings_load_cb(char *name, char *val, void *cb_arg)
{
	int rc = settings_set_value(name, val);
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

static void settings_dup_check_cb(char *name, char *val, void *cb_arg)
{
	struct settings_dup_check_arg *cdca = (struct settings_dup_check_arg *)
					      cb_arg;

	if (strcmp(name, cdca->name)) {
		return;
	}
	if (!val) {
		if (!cdca->val || cdca->val[0] == '\0') {
			cdca->is_dup = 1;
		} else {
			cdca->is_dup = 0;
		}
	} else {
		if (cdca->val && !strcmp(val, cdca->val)) {
			cdca->is_dup = 1;
		} else {
			cdca->is_dup = 0;
		}
	}
}

/*
 * Append a single value to persisted config. Don't store duplicate value.
 */
int settings_save_one(const char *name, char *value)
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
	cdca.val = value;
	cdca.is_dup = 0;
	cs->cs_itf->csi_load(cs, settings_dup_check_cb, &cdca);
	if (cdca.is_dup == 1) {
		return 0;
	}
	return cs->cs_itf->csi_save(cs, name, value);
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
			rc2 = ch->h_export(settings_save_one,
					   SETTINGS_EXPORT_PERSIST);
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
