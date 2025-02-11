/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Dynamic shell command completion for network interface.
 * This is be used by multiple commands.
 */

#define MAX_IFACE_HELP_STR_LEN sizeof("longbearername (0xabcd0123)")
#define MAX_IFACE_STR_LEN sizeof("xxx")

static char iface_help_buffer[MAX_IFACE_COUNT][MAX_IFACE_HELP_STR_LEN];
static char iface_index_buffer[MAX_IFACE_COUNT][MAX_IFACE_STR_LEN];

static void iface_index_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(iface_index, iface_index_get);

static char *set_iface_index_buffer(size_t idx)
{
	struct net_if *iface = net_if_get_by_index(idx);

	/* Network interfaces start at 1 */
	if (idx == 0) {
		return "";
	}

	if (!iface) {
		return NULL;
	}

	snprintk(iface_index_buffer[idx - 1], MAX_IFACE_STR_LEN, "%d", (uint8_t)idx);

	return iface_index_buffer[idx - 1];
}

static char *set_iface_index_help(size_t idx)
{
	struct net_if *iface = net_if_get_by_index(idx);

	/* Network interfaces start at 1 */
	if (idx == 0) {
		return "";
	}

	if (!iface) {
		return NULL;
	}

#if defined(CONFIG_NET_INTERFACE_NAME)
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1];

	net_if_get_name(iface, name, CONFIG_NET_INTERFACE_NAME_LEN);
	name[CONFIG_NET_INTERFACE_NAME_LEN] = '\0';

	snprintk(iface_help_buffer[idx - 1], MAX_IFACE_HELP_STR_LEN,
		 "%s [%s] (%p)", name, iface2str(iface, NULL), iface);
#else
	snprintk(iface_help_buffer[idx - 1], MAX_IFACE_HELP_STR_LEN,
		 "[%s] (%p)", iface2str(iface, NULL), iface);
#endif

	return iface_help_buffer[idx - 1];
}

static void iface_index_get(size_t idx, struct shell_static_entry *entry)
{
	entry->handler = NULL;
	entry->help  = set_iface_index_help(idx);
	entry->subcmd = &iface_index;
	entry->syntax = set_iface_index_buffer(idx);
}
