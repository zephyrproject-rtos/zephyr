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
	struct settings_handler *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
		if (!strcmp(handler->name, ch->name)) {
			return -EEXIST;
		}
	}
	sys_slist_prepend(&settings_handlers, &handler->node);

	return 0;
}

bool settings_name_split(const char *name, const char *key, const char **next)
{
	bool status = false;
	const char *next_ptr = NULL;

	while (*key) {
		if (*key != *name) {
			break;
		}
		++key;
		++name;
	}
	if (*key == '\0') {
		if (*name == SETTINGS_NAME_SEPARATOR) {
			next_ptr = name + 1;
			status = true;
		} else if (*name == '\0') {
			status = true;
		} else {
			/* Nothing to do */
		}
	} else {
		/* Nothing to do */
	}

	if (next) {
		*next = next_ptr;
	}
	return status;
}

const char *settings_name_skip(const char *name)
{
	bool empty = true;

	while (*name) {
		if (*name++ == SETTINGS_NAME_SEPARATOR) {
			break; /* Return character after */
		}
		empty = false;
	}
	return empty ? NULL : name;
}

size_t settings_name_elen(const char *name)
{
	size_t n = 0;

	while ((*name != '\0') && (*name != SETTINGS_NAME_SEPARATOR)) {
		++n;
		++name;
	}
	return n;
}

bool settings_name_cmp(const char *name, const char *patt)
{
	LOG_DBG("%s (\"%s\", \"%s\");\n", __func__, name, patt);

	while (*patt) {
		if (*patt == '*') {
			++patt;
			if (*patt == '*') {
				++patt;
				if (*patt == SETTINGS_NAME_SEPARATOR) {
					++patt;
				}
				if (*patt == '\0') {
					/* Optimize this:
					 * it would be most used case
					 */
					LOG_DBG(" \"**\" at the end.\n");
					return true;
				}
				do {
					if (settings_name_cmp(patt, name))
						return true;
					name = settings_name_skip(name);
				} while (*name);
			} else {
				if (*patt == SETTINGS_NAME_SEPARATOR) {
					++patt;
				}
				name = settings_name_skip(name);
				if (!name) {
					LOG_DBG("No element to match "
						"\"*\" pattern\n");
					return false;
				}
				LOG_DBG("  skipped(\"%s\", \"%s\")\n",
					name, patt);
				continue;
			}
			if (!*patt) {
				break;
			}
		}
		if (*patt != *name) {
			break;
		}
		++patt;
		++name;
	}

	LOG_DBG("  returning(\"%s\", \"%s\")\n", name, patt);
	return (*patt == '\0' && *name == '\0');
}

struct settings_handler *settings_parse_and_lookup(const char *name,
						   const char **key)
{
	struct settings_handler *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
		if (settings_name_split(name, ch->name, key)) {
			return ch;
		}
	}
	return NULL;
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
