/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2023 Sensorfy B.V.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(flash_map_shell);

extern const struct flash_area *flash_map;

static void fa_cb(const struct flash_area *fa, void *user_data)
{
	struct shell *sh = user_data;
#if CONFIG_FLASH_MAP_LABELS
	const char *fa_label = flash_area_label(fa);

	if (fa_label == NULL) {
		fa_label = "-";
	}
	shell_print(sh, "%2d   0x%0*" PRIxPTR "   %-26s  %-24.24s  0x%-10x 0x%-12x", (int)fa->fa_id,
		    sizeof(uintptr_t) * 2, (uintptr_t)fa->fa_dev, fa->fa_dev->name, fa_label,
		    (uint32_t)fa->fa_off, fa->fa_size);
#else
	shell_print(sh, "%2d   0x%0*" PRIxPTR "   %-26s  0x%-10x 0x%-12x", (int)fa->fa_id,
		    sizeof(uintptr_t) * 2, (uintptr_t)fa->fa_dev, fa->fa_dev->name,
		    (uint32_t)fa->fa_off, fa->fa_size);
#endif
}

static int cmd_flash_map_list(const struct shell *sh, size_t argc, char **argv)
{
#if CONFIG_FLASH_MAP_LABELS
	shell_print(sh, "ID | Device     | Device Name               "
			"| Label                   | Offset     | Size");
	shell_print(sh, "--------------------------------------------"
			"-----------------------------------------------");
#else
	shell_print(sh, "ID | Device     | Device Name               "
			"| Offset     | Size");
	shell_print(sh, "-----------------------------------------"
			"------------------------------");
#endif
	flash_area_foreach(fa_cb, (struct shell *)sh);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_flash_map,
	/* Alphabetically sorted. */
	SHELL_CMD(list, NULL, "List flash areas", cmd_flash_map_list),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(flash_map, &sub_flash_map, "Flash map commands", NULL);
