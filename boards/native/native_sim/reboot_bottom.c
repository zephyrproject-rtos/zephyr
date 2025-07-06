/*
 * Copyright (c) 2025 GARDENA GmbH
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <nsi_main.h>
#include <nsi_tasks.h>
#include <nsi_tracing.h>
#include <nsi_cmdline.h>

static const char module[] = "native_sim_reboot";

static long close_open_fds(void)
{
	/* close all open file descriptors except 0-2 */
	errno = 0;

	long max_fd = sysconf(_SC_OPEN_MAX);

	if (max_fd < 0) {
		if (errno != 0) {
			nsi_print_error_and_exit("%s: %s\n", module, strerror(errno));
		} else {
			nsi_print_warning("%s: Cannot determine maximum number of file descriptors"
					  "\n",
					  module);
		}
		return max_fd;
	}
	for (int fd = 3; fd < max_fd; fd++) {
		(void)close(fd);
	}
	return 0;
}

static bool reboot_on_exit;

void native_set_reboot_on_exit(void)
{
	reboot_on_exit = true;
}

void maybe_reboot(void)
{
	char **argv;
	int argc;

	if (!reboot_on_exit) {
		return;
	}

	reboot_on_exit = false; /* If we reenter it means we failed to reboot */

	nsi_get_cmd_line_args(&argc, &argv);

	if (close_open_fds() < 0) {
		nsi_exit(1);
	}

	nsi_print_warning("%s: Restarting process.\n", module);

	(void)execv("/proc/self/exe", argv);

	nsi_print_error_and_exit("%s: Failed to restart process, exiting (%s)\n", module,
				 strerror(errno));
}

NSI_TASK(maybe_reboot, ON_EXIT_POST, 999);
