/*
 * Copyright (c) 2025 GARDENA GmbH
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#include <nsi_main.h>
#include <nsi_tasks.h>
#include <nsi_tracing.h>
#include <nsi_cmdline.h>
#include <nsi_host_trampolines.h>

static const char module[] = "native_sim_reboot";

static bool reboot_on_exit;

void native_set_reboot_on_exit(void)
{
	reboot_on_exit = true;
}

/*
 * Return the path of the running executable, or NULL if it cannot be determined
 */
static const char *self_exe_path(char *buffer, size_t size)
{
#ifdef __APPLE__
	uint32_t length = size;

	if (_NSGetExecutablePath(buffer, &length) != 0) {
		return NULL;
	}

	return buffer;
#else
	(void)buffer;
	(void)size;

	return "/proc/self/exe";
#endif
}

void maybe_reboot(void)
{
	char path_buffer[PATH_MAX];
	const char *path;
	char **argv;
	int argc;

	if (!reboot_on_exit) {
		return;
	}

	reboot_on_exit = false; /* If we reenter it means we failed to reboot */

	nsi_get_cmd_line_args(&argc, &argv);

	/* Let's set an environment variable which the native_sim hw_info driver may check */
	(void)nsi_host_setenv("NATIVE_SIM_RESET_CAUSE", "SOFTWARE", 1);

	nsi_print_warning("%s: Restarting process.\n", module);

	path = self_exe_path(path_buffer, sizeof(path_buffer));
	if (path == NULL) {
		nsi_print_error_and_exit("%s: Could not find own executable path, exiting\n",
					 module);
	}

	(void)execv(path, argv);

	nsi_print_error_and_exit("%s: Failed to restart process, exiting (%s)\n", module,
				 strerror(errno));
}

NSI_TASK(maybe_reboot, ON_EXIT_POST, 999);
