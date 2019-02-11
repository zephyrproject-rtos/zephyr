/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include "settings/settings.h"
#include "settings_priv.h"
#include <zephyr/types.h>

sys_slist_t settings_handlers;

static u8_t settings_cmd_inited;

static struct settings_handler *settings_handler_lookup(char *name);
void settings_store_init(void);

void settings_init(void)
{
	if (!settings_cmd_inited) {
		sys_slist_init(&settings_handlers);
		settings_store_init();

		settings_cmd_inited = 1U;
	}
}

int settings_register(struct settings_handler *handler)
{
	if (settings_handler_lookup(handler->name)) {
		return -EEXIST;
	}
	sys_slist_prepend(&settings_handlers, &handler->node);

	return 0;
}

/*
 * Find settings_handler based on name.
 */
static struct settings_handler *settings_handler_lookup(char *name)
{
	struct settings_handler *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
		if (!strcmp(name, ch->name)) {
			return ch;
		}
	}
	return NULL;
}

/*
 * Separate string into argv array.
 */
int settings_parse_name(char *name, int *name_argc, char *name_argv[])
{
	int i = 0;

	while (name) {
		name_argv[i++] = name;

		while (1) {
			if (*name == '\0') {
				name = NULL;
				break;
			}

			if (*name == *SETTINGS_NAME_SEPARATOR) {
				*name = '\0';
				name++;
				break;
			}
			name++;
		}
	}

	*name_argc = i;

	return 0;
}

static struct settings_handler *settings_parse_and_lookup(char *name,
							  int *name_argc,
							  char *name_argv[])
{
	int rc;

	rc = settings_parse_name(name, name_argc, name_argv);
	if (rc) {
		return NULL;
	}
	return settings_handler_lookup(name_argv[0]);
}

int settings_set_value_priv(char *name, void *val_read_cb_ctx, off_t off,
		       u8_t is_runtime)
{
	int name_argc;
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct settings_handler *ch;
	struct read_value_cb_ctx value_ctx;

	ch = settings_parse_and_lookup(name, &name_argc, name_argv);
	if (!ch) {
		return -EINVAL;
	}

	value_ctx.read_cb_ctx = val_read_cb_ctx;
	value_ctx.off = off;
	value_ctx.runtime = is_runtime;

	return ch->h_set(name_argc - 1, &name_argv[1], (void *)&value_ctx);
}

/* API */
int settings_set_value(char *name, void *val_str, size_t len)
{
	struct runtime_value_ctx rt_ctx;

	rt_ctx.p_value = val_str;
	rt_ctx.size = len;

	return settings_set_value_priv(name, &rt_ctx, 0, 1);
}

/* API */
int settings_val_read_cb(void *value_ctx, void *buf, size_t len)
{
	struct read_value_cb_ctx *value_context = value_ctx;
	size_t len_read;
	int rc;
	struct runtime_value_ctx *rt_ctx;

	if (value_context->runtime) {
		rt_ctx = value_context->read_cb_ctx;
		len_read = MIN(len, rt_ctx->size);
		memcpy(buf, rt_ctx->p_value, len_read);
		return len_read;
	} else {
		rc = settings_line_val_read(value_context->off, 0, buf, len,
					    &len_read,
					    value_context->read_cb_ctx);
	}

	if (rc == 0) {
		return len_read;
	}

	return rc;
}

/* API */
size_t settings_val_get_len_cb(void *value_ctx)
{
	struct read_value_cb_ctx *value_context = value_ctx;
	struct runtime_value_ctx *rt_ctx;

	if (value_context->runtime) {
		rt_ctx = value_context->read_cb_ctx;
		return rt_ctx->size;
	} else {
		return settings_line_val_get_len(value_context->off,
						 value_context->read_cb_ctx);
	}
}

/*
 * Get value in printable string form. If value is not string, the value
 * will be filled in *buf.
 * Return value will be pointer to beginning of that buffer,
 * except for string it will pointer to beginning of string.
 */
int settings_get_value(char *name, char *buf, int buf_len)
{
	int name_argc;
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct settings_handler *ch;

	ch = settings_parse_and_lookup(name, &name_argc, name_argv);
	if (!ch) {
		return -EINVAL;
	}

	if (!ch->h_get) {
		return -EINVAL;
	}
	return ch->h_get(name_argc - 1, &name_argv[1], buf, buf_len);
}

int settings_commit(char *name)
{
	int name_argc;
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct settings_handler *ch;
	int rc;
	int rc2;

	if (name) {
		ch = settings_parse_and_lookup(name, &name_argc, name_argv);
		if (!ch) {
			return -EINVAL;
		}
		if (ch->h_commit) {
			return ch->h_commit();
		} else {
			return 0;
		}
	} else {
		rc = 0;
		SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
			if (ch->h_commit) {
				rc2 = ch->h_commit();
				if (!rc) {
					rc = rc2;
				}
			}
		}
		return rc;
	}
}
