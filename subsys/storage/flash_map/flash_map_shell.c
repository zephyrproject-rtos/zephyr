/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <init.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <storage/flash_map.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(flash_map_shell);

extern const struct flash_area *flash_map;
static int flash_area_id;

static void fa_cb(const struct flash_area *fa, void *user_data)
{
	struct shell *shell = user_data;

	shell_print(shell, "%-4d %-17p %-12p %-20s 0x%-8x 0x%-8x",
		    flash_area_id, fa, fa->fa_dev, fa->fa_dev->name,
		    fa->fa_off, fa->fa_size);
	++flash_area_id;
}

static int cmd_flash_map_list(const struct shell *shell, size_t argc,
			      char **argv)
{
	flash_area_id = 0;
	shell_print(shell, "ID | Flash area Ptr | Device Ptr | Device Name "
		    "       |  Offset  | Size");
	shell_print(shell, "--------------------------------------------------------------------"
			   "--------");
	flash_area_foreach(fa_cb, (struct shell *)shell);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_flash_map,
	/* Alphabetically sorted. */
	SHELL_CMD(list, NULL, "List flash areas", cmd_flash_map_list),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(flash_map, &sub_flash_map, "Flash map commands", NULL);
