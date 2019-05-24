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

#include <logging/log.h>
LOG_MODULE_REGISTER(settings, CONFIG_SETTINGS_LOG_LEVEL);

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

struct settings_handler *settings_parse_and_lookup(char *name,
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

int settings_commit(void)
{
	struct settings_handler *ch;
	int rc;
	int rc2;

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
