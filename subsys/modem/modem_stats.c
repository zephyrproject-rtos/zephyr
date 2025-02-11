/*
 * Copyright (c) 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef _POSIX_VERSION
#undef _POSIX_VERSION
#endif
#define _POSIX_VERSION 200809L

#include <zephyr/modem/stats.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_stats);

static struct k_spinlock stats_buffer_lock;
static sys_slist_t stats_buffer_list;

static struct modem_stats_buffer *stats_buffer_from_node(sys_snode_t *node)
{
	return (struct modem_stats_buffer *)node;
}

static void stats_buffer_list_append(struct modem_stats_buffer *buffer)
{
	K_SPINLOCK(&stats_buffer_lock) {
		sys_slist_append(&stats_buffer_list, &buffer->node);
	}
}

static struct modem_stats_buffer *stats_buffer_list_first(void)
{
	struct modem_stats_buffer *first;

	K_SPINLOCK(&stats_buffer_lock) {
		first = stats_buffer_from_node(sys_slist_peek_head(&stats_buffer_list));
	}

	return first;
}

static struct modem_stats_buffer *stats_buffer_list_next(struct modem_stats_buffer *buffer)
{
	struct modem_stats_buffer *next;

	K_SPINLOCK(&stats_buffer_lock) {
		next = stats_buffer_from_node(sys_slist_peek_next(&buffer->node));
	}

	return next;
}

static uint8_t percent_used(uint32_t max_used, uint32_t cap)
{
	uint64_t percent;

	if (max_used == 0) {
		return 0;
	}

	if (max_used == cap) {
		return 100;
	}

	percent = 100;
	percent *= max_used;
	percent /= cap;

	return (uint8_t)percent;
}

static void stats_buffer_get_and_clear_max_used(struct modem_stats_buffer *buffer,
						uint32_t *max_used)
{
	K_SPINLOCK(&stats_buffer_lock) {
		*max_used = buffer->max_used;
		buffer->max_used = 0;
	}
}

static bool stats_buffer_length_is_valid(const struct modem_stats_buffer *buffer, uint32_t length)
{
	return length <= buffer->size;
}

static void stats_buffer_log_invalid_length(const struct modem_stats_buffer *buffer,
					    uint32_t length)
{
	LOG_ERR("%s: length (%u) exceeds size (%u)", buffer->name, length, buffer->size);
}

static void stats_buffer_update_max_used(struct modem_stats_buffer *buffer, uint32_t length)
{
	K_SPINLOCK(&stats_buffer_lock) {
		if (buffer->max_used < length) {
			buffer->max_used = length;
		}
	}
}

static void stats_buffer_print_to_shell(const struct shell *sh,
					const struct modem_stats_buffer *buffer,
					uint32_t max_used)
{
	shell_print(sh, "%s: used at most: %u of %u (%u%%)", buffer->name, max_used,
		    buffer->size, percent_used(max_used, buffer->size));
}

static int stats_buffer_shell_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
	struct modem_stats_buffer *buffer;
	uint32_t max_used;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	buffer = stats_buffer_list_first();

	if (buffer == NULL) {
		shell_print(sh, "no buffers exist");
		return 0;
	}

	while (buffer != NULL) {
		stats_buffer_get_and_clear_max_used(buffer, &max_used);
		stats_buffer_print_to_shell(sh, buffer, max_used);
		buffer = stats_buffer_list_next(buffer);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_stats_cmds,
	SHELL_CMD(buffer, NULL, "Get buffer statistics", stats_buffer_shell_cmd_handler),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(modem_stats, &sub_stats_cmds, "Modem statistics commands", NULL);

static void stats_buffer_set_name(struct modem_stats_buffer *buffer, const char *name)
{
	buffer->name[sizeof(buffer->name) - 1] = '\0';
	strncpy(buffer->name, name, sizeof(buffer->name) - 1);
}

void modem_stats_buffer_init(struct modem_stats_buffer *buffer,
			     const char *name, uint32_t size)
{
	stats_buffer_set_name(buffer, name);
	buffer->max_used = 0;
	buffer->size = size;
	stats_buffer_list_append(buffer);
}

void modem_stats_buffer_advertise_length(struct modem_stats_buffer *buffer, uint32_t length)
{
	if (!stats_buffer_length_is_valid(buffer, length)) {
		stats_buffer_log_invalid_length(buffer, length);
		return;
	}

	stats_buffer_update_max_used(buffer, length);
}
