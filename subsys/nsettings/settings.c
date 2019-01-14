/*
 * Copyright (c) 2018 Laczen
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

#include <nsettings/settings.h>
#include "settings_priv.h"
#include <zephyr/types.h>

sys_slist_t settings_handlers;

static u8_t settings_cmd_inited;

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
	sys_slist_prepend(&settings_handlers, &handler->node);
	return 0;
}

/*
 * Find settings_handler based on name.
 */
struct settings_handler *settings_handler_lookup(char *name)
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

struct settings_handler *settings_parse_and_lookup(char *name,
	int *name_argc, char *name_argv[])
{
	int rc;

	rc = settings_parse_name(name, name_argc, name_argv);
	if (rc) {
		return NULL;
	}
	return settings_handler_lookup(name_argv[0]);
}

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
