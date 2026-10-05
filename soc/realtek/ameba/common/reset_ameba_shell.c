/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/shell/shell.h>

static int cmd_reboot_uartburn(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	BKUP_Clear(BKUP_REG0, BKUP_MASK_UARTBURN_BOOT);
	BKUP_Set(BKUP_REG0, BKUP_BIT_UARTBURN_BOOT);

	System_Reset();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_reboot,
	SHELL_CMD_ARG(uartburn, NULL, "burn via uart", cmd_reboot_uartburn, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_ARG_REGISTER(reboot, &sub_reboot, "REBOOT commands", NULL, 2, 0);
