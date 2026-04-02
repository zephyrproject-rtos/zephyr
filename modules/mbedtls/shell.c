/*
 * Copyright (C) 2021 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <mbedtls/memory_buffer_alloc.h>

#if defined(MBEDTLS_MEMORY_DEBUG)
static int cmd_mbedtls_heap_details(const struct shell *sh, size_t argc,
				    char **argv)
{
	mbedtls_memory_buffer_alloc_status();

	return 0;
}

static int cmd_mbedtls_heap_max_reset(const struct shell *sh, size_t argc,
				  char **argv)
{
	mbedtls_memory_buffer_alloc_max_reset();

	return 0;
}

static int cmd_mbedtls_heap(const struct shell *sh, size_t argc, char **argv)
{
	size_t max_used, max_blocks;
	size_t cur_used, cur_blocks;

	mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks);
	mbedtls_memory_buffer_alloc_cur_get(&cur_used, &cur_blocks);

	shell_print(sh, "Maximum (peak): %zu bytes, %zu blocks",
		    max_used, max_blocks);
	shell_print(sh, "Current: %zu bytes, %zu blocks",
		    cur_used, cur_blocks);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mbedtls_heap_cmds,
	SHELL_CMD_ARG(details, NULL, "Print heap details",
		      cmd_mbedtls_heap_details, 1, 0),
	SHELL_CMD_ARG(max_reset, NULL, "Reset max heap statistics",
		      cmd_mbedtls_heap_max_reset, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(mbedtls_cmds,
#if defined(MBEDTLS_MEMORY_DEBUG)
	SHELL_CMD_ARG(heap, &mbedtls_heap_cmds, "Show heap status",
		      cmd_mbedtls_heap, 1, 0),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(mbedtls, &mbedtls_cmds, "mbed TLS commands", NULL);
