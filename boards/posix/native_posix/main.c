/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The basic principle of operation is:
 *   No asynchronous behavior, no indeterminism.
 *   If you run the same thing 20 times, you get exactly the same result 20
 *   times.
 *   It does not matter if you are running from console, or in a debugger
 *   and you go for lunch in the middle of the debug session.
 *
 * This is achieved as follows:
 *  The execution of native_posix is decoupled from the underlying host and its
 *  peripherals (unless set otherwise).
 *  In general, time in native_posix is simulated.
 *
 * But, native_posix can also be linked if desired to the underlying host,
 * e.g.:You can use the provided Ethernet TAP driver, or a host BLE controller.
 *
 * In this case, the no-indeterminism principle is lost. Runs of native_posix
 * will depend on the host load and the interactions with those real host
 * peripherals.
 *
 */

#include <soc.h>
#include "hw_models_top.h"
#include <stdlib.h>
#include <zephyr/sys/util.h>
#include "cmdline.h"

void posix_exit(int exit_code)
{
	static int max_exit_code;

	max_exit_code = MAX(exit_code, max_exit_code);
	/*
	 * posix_soc_clean_up may not return if this is called from a SW thread,
	 * but instead it would get posix_exit() recalled again
	 * ASAP from the HW thread
	 */
	posix_soc_clean_up();
	hwm_cleanup();
	native_cleanup_cmd_line();
	exit(max_exit_code);
}

/**
 * This is the actual main for the Linux process,
 * the Zephyr application main is renamed something else thru a define.
 */
int main(int argc, char *argv[])
{
	run_native_tasks(_NATIVE_PRE_BOOT_1_LEVEL);

	native_handle_cmd_line(argc, argv);

	run_native_tasks(_NATIVE_PRE_BOOT_2_LEVEL);

	hwm_init();

	run_native_tasks(_NATIVE_PRE_BOOT_3_LEVEL);

	posix_boot_cpu();

	run_native_tasks(_NATIVE_FIRST_SLEEP_LEVEL);

	hwm_main_loop();

	/* This line should be unreachable */
	return 1; /* LCOV_EXCL_LINE */
}
