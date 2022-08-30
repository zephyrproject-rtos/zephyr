/**
 * Copyright (c) 2020 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "eswifi.h"

static struct eswifi_dev *eswifi;

void eswifi_shell_register(struct eswifi_dev *dev)
{
	/* only one instance supported */
	if (eswifi) {
		return;
	}

	eswifi = dev;
}

static int eswifi_shell_atcmd(const struct shell *shell, size_t argc,
			      char **argv)
{
	int i;

	if (eswifi == NULL) {
		shell_print(shell, "no eswifi device registered");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_help(shell);
		return -ENOEXEC;
	}

	eswifi_lock(eswifi);

	memset(eswifi->buf, 0, sizeof(eswifi->buf));
	for (i = 1; i < argc; i++) {
		strcat(eswifi->buf, argv[i]);
	}
	strcat(eswifi->buf, "\r");

	shell_print(shell, "> %s", eswifi->buf);
	eswifi_at_cmd(eswifi, eswifi->buf);
	shell_print(shell, "< %s", eswifi->buf);

	eswifi_unlock(eswifi);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(eswifi_shell,
	SHELL_CMD(atcmd, NULL, "<atcmd>", eswifi_shell_atcmd),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(eswifi, &eswifi_shell, "esWiFi debug shell", NULL);
