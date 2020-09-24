/*
 * Copyright 2020 Peter Bigot Consulting, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <shell/shell.h>
#include <power/reboot.h>

static int cmd_reboot(const struct shell *shell,
		       int type, size_t argc, char **argv)
{
	long int delay_s = 3;

	if (argc > 1) {
		char *ep = NULL;

		delay_s = strtol(argv[1], &ep, 0);
		if ((argv[1][0] == 0)
		    || (*ep != 0)
		    || (delay_s < 0)) {
			shell_print(shell, "Invalid delay: %s\n", argv[1]);
			return -EINVAL;
		}
	}
	shell_print(shell, "%s reboot in %ld s...",
		    (type == SYS_REBOOT_COLD) ? "cold"
		    : (type == SYS_REBOOT_WARM) ? "warm"
		    : "unknown", delay_s);
	while (delay_s > 0) {
		shell_print(shell, "\r%ld ...", delay_s);
		k_sleep(K_SECONDS(1));
		--delay_s;
	}
	shell_print(shell, "\r\nrebooting ...", delay_s);
	sys_reboot(type);
	return 0;
}

int cmd_reboot_cold(const struct shell *shell,
               size_t argc,
               char **argv)
{
	cmd_reboot(shell, SYS_REBOOT_COLD, argc, argv);
	return -ENOEXEC;
}

int cmd_reboot_warm(const struct shell *shell,
               size_t argc,
               char **argv)
{
	cmd_reboot(shell, SYS_REBOOT_COLD, argc, argv);
	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_reboot,
                               SHELL_CMD_ARG(cold, NULL,
                                             "cold reboot [delay = 3]",
                                             cmd_reboot_cold, 1, 1),
                               SHELL_CMD_ARG(warm, NULL,
                                             "warm reboot [delay = 3]",
                                             cmd_reboot_warm, 1, 1),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(reboot, &sub_reboot, "Reboot commands", NULL);
