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
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/settings/settings.h>
#include "settings_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

sys_slist_t settings_load_srcs;
struct settings_store *settings_save_dst;

void settings_src_register(struct settings_store *cs)
{
	sys_slist_append(&settings_load_srcs, &cs->cs_next);
}

void settings_dst_register(struct settings_store *cs)
{
	settings_save_dst = cs;
}

int settings_load(void)
{
	return settings_load_subtree(NULL);
}

int settings_load_subtree(const char *subtree)
{
	struct settings_store *cs;
	int rc;
	const struct settings_load_arg arg = {
		.subtree = subtree
	};

	/*
	 * for every config store
	 *    load config
	 *    apply config
	 *    commit all
	 */
	settings_lock_take();
	SYS_SLIST_FOR_EACH_CONTAINER(&settings_load_srcs, cs, cs_next) {
		cs->cs_itf->csi_load(cs, &arg);
	}
	rc = settings_commit_subtree(subtree);
	settings_lock_release();
	return rc;
}

int settings_load_subtree_direct(
	const char             *subtree,
	settings_load_direct_cb cb,
	void                   *param)
{
	struct settings_store *cs;

	const struct settings_load_arg arg = {
		.subtree = subtree,
		.cb = cb,
		.param = param
	};
	/*
	 * for every config store
	 *    load config
	 *    apply config
	 *    commit all
	 */
	settings_lock_take();
	SYS_SLIST_FOR_EACH_CONTAINER(&settings_load_srcs, cs, cs_next) {
		cs->cs_itf->csi_load(cs, &arg);
	}
	settings_lock_release();
	return 0;
}

struct default_param {
	void *buf;
	size_t buf_len;
	size_t *val_len;
};

/* Default callback to set a Key/Value pair */
static int settings_set_default_cb(const char *name, size_t len, settings_read_cb read_cb,
				   void *cb_arg, void *param)
{
	int rc = 0;
	const char *next;
	size_t name_len;
	struct default_param *dest = (struct default_param *)param;

	name_len = settings_name_next(name, &next);
	if (name_len == 0) {
		rc = read_cb(cb_arg, dest->buf, MIN(dest->buf_len, len));
		*dest->val_len = len;
	}

	return rc;
}

/* Default callback to get the value's length of the Key defined by name.
 * Returns the value's length in the provided `param` pointer
 */
static int settings_get_val_len_default_cb(const char *name, size_t len,
					   settings_read_cb read_cb __maybe_unused,
					   void *cb_arg __maybe_unused, void *param)
{
	const char *next;
	size_t name_len;
	size_t *val_len = (size_t *)param;

	name_len = settings_name_next(name, &next);
	if (name_len == 0) {
		*val_len = len;
	}

	return 0;
}

/* Gets the value's size if the Key defined by name is in the persistent storage,
 * Returns 0 if the Key doesn't exist.
 */
ssize_t settings_get_val_len(const char *name)
{
	struct settings_store *cs;
	int rc = 0;
	size_t val_len = 0;

	/*
	 * for every config store that supports this function
	 * get the value's length.
	 */
	settings_lock_take();
	SYS_SLIST_FOR_EACH_CONTAINER(&settings_load_srcs, cs, cs_next) {
		if (cs->cs_itf->csi_get_val_len) {
			val_len = cs->cs_itf->csi_get_val_len(cs, name);
		} else {
			const struct settings_load_arg arg = {
				.subtree = name,
				.cb = &settings_get_val_len_default_cb,
				.param = &val_len
			};
			rc = cs->cs_itf->csi_load(cs, &arg);
		}
	}
	settings_lock_release();

	if (rc >= 0) {
		return val_len;
	}

	return rc;
}

/* Load a single key/value from persistent storage */
ssize_t settings_load_one(const char *name, void *buf, size_t buf_len)
{
	struct settings_store *cs;
	size_t val_len = 0;
	int rc = 0;

	/*
	 * For every config store that defines csi_load_one() function use it.
	 * Otherwise, use the csi_load() function to load the key/value pair
	 */
	settings_lock_take();
	SYS_SLIST_FOR_EACH_CONTAINER(&settings_load_srcs, cs, cs_next) {
		if (cs->cs_itf->csi_load_one) {
			rc = cs->cs_itf->csi_load_one(cs, name, (char *)buf, buf_len);
			val_len = (rc >= 0) ? rc : 0;
		} else {
			struct default_param param = {
				.buf = buf,
				.buf_len = buf_len,
				.val_len = &val_len
			};
			const struct settings_load_arg arg = {
				.subtree = name,
				.cb = &settings_set_default_cb,
				.param = &param
			};
			rc = cs->cs_itf->csi_load(cs, &arg);
		}
	}
	settings_lock_release();

	if (rc >= 0) {
		return val_len;
	}
	return rc;
}

/*
 * Append a single value to persisted config. Don't store duplicate value.
 */
int settings_save_one(const char *name, const void *value, size_t val_len)
{
	int rc;
	struct settings_store *cs;

	cs = settings_save_dst;
	if (!cs) {
		return -ENOENT;
	}

	settings_lock_take();

	rc = cs->cs_itf->csi_save(cs, name, (char *)value, val_len);

	settings_lock_release();

	return rc;
}

int settings_delete(const char *name)
{
	return settings_save_one(name, NULL, 0);
}

int settings_save(void)
{
	return settings_save_subtree(NULL);
}

int settings_save_subtree(const char *subtree)
{
	struct settings_store *cs;
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

	STRUCT_SECTION_FOREACH(settings_handler_static, ch) {
		if (subtree && !settings_name_steq(ch->name, subtree, NULL)) {
			continue;
		}
		if (ch->h_export) {
			rc2 = ch->h_export(settings_save_one);
			if (!rc) {
				rc = rc2;
			}
		}
	}

#if defined(CONFIG_SETTINGS_DYNAMIC_HANDLERS)
	struct settings_handler *ch;
	SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
		if (subtree && !settings_name_steq(ch->name, subtree, NULL)) {
			continue;
		}
		if (ch->h_export) {
			rc2 = ch->h_export(settings_save_one);
			if (!rc) {
				rc = rc2;
			}
		}
	}
#endif /* CONFIG_SETTINGS_DYNAMIC_HANDLERS */

	if (cs->cs_itf->csi_save_end) {
		cs->cs_itf->csi_save_end(cs);
	}
	return rc;
}

int settings_storage_get(void **storage)
{
	struct settings_store *cs = settings_save_dst;

	if (!cs) {
		return -ENOENT;
	}

	if (cs->cs_itf->csi_storage_get) {
		*storage = cs->cs_itf->csi_storage_get(cs);
	}

	return 0;
}

void settings_store_init(void)
{
	sys_slist_init(&settings_load_srcs);
}

#ifdef CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION
int settings_save_subtree_or_single_without_modification(const char *name,
							 bool save_if_subtree,
							 bool save_if_single_setting)
{
	int rc;
	int value_size;
	uint8_t read_buffer[CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION_VALUE_SIZE];
	const char *next = NULL;
	struct settings_handler_static *handler;

	if (save_if_subtree == false && save_if_single_setting == false) {
		return -EINVAL;
	}

	handler = settings_parse_and_lookup(name, &next);

	if (next == NULL) {
		/* This is a subtree of settings, bail if user did not request saving it. */
		if (save_if_subtree == false) {
			return -EPERM;
		}

		return settings_save_subtree(name);
	} else if (save_if_single_setting == false) {
		return -EPERM;
	}

	/*
	 * For single settings, we need to be able to retrieve the value of the setting, if this
	 * is not supported then single saving cannot be done with this key.
	 */
	if (handler->h_get == NULL) {
		return -ENOSYS;
	}

	settings_lock_take();

	/*
	 * Settings does not support getting the size of a setting, therefore attempt to read the
	 * full buffer size, if that returns that amount of data then we must abort as we cannot
	 * get the full data with this buffer and would save a truncated value.
	 */
	value_size = handler->h_get(next, read_buffer, sizeof(read_buffer));

	if (value_size < 0) {
		rc = value_size;
		goto exit;
	} else if (value_size == sizeof(read_buffer)) {
		rc = -EDOM;
		goto exit;
	}

	rc = settings_save_one(name, read_buffer, value_size);

	/*
	 * Caller just needs to know that it was successful, not the length of the data that was
	 * saved.
	 */
	if (rc >= 0) {
		rc = 0;
	} else if (rc < 0) {
		LOG_ERR("Saving single setting '%s' of length %d failed: %d", name, value_size,
			rc);
	}

exit:
	settings_lock_release();

	return rc;
}
#endif /* CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION */
