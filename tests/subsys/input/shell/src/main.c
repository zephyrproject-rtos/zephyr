/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

#define SLEEP_TIME_MS 200
#define CMD_BUF_LEN 128

int main(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	bool val = false;
	int err;
	char buf[CMD_BUF_LEN];

	err = shell_execute_cmd(sh, "input dump on");
	if (err) {
		printf("Failed to execute the shell command: %d.\n",
		       err);
	}

	while (true) {
		snprintf(buf, 128, "input report 1 2 %d", val);
		err = shell_execute_cmd(sh, buf);
		if (err) {
			printf("Failed to execute the shell command: %d.\n",
			       err);
		}

		val = !val;

		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
