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
 * The HW models run in their own simulated time. We do really not attempt
 * to link ourselves to the actual real time / wall time of the machine as this
 * would make execution indeterministic and debugging or instrumentation not
 * really possible. Although we may slow the run to real time.
 */

#include <soc.h>
#include "hw_models_top.h"
#include <stdlib.h>
#include "misc/util.h"
#include "cmdline.h"

void posix_exit(int exit_code)
{
	static int max_exit_code;

	max_exit_code = max(exit_code, max_exit_code);
	/*
	 * posix_soc_clean_up may not return if this is called from a SW thread,
	 * but instead it would get posix_exit() recalled again
	 * ASAP from the HW thread
	 */
	posix_soc_clean_up();
	hwm_cleanup();
	exit(max_exit_code);
}

/**
 * This is the actual main for the Linux process,
 * the Zephyr application main is renamed something else thru a define.
 *
 * Note that normally one wants to use this POSIX arch to be part of a
 * simulation engine, with some proper HW models and what not
 *
 * This is just a very simple demo which is able to run some of the sample
 * apps (hello world, synchronization, philosophers) and run the sanity-check
 * regression
 */
int main(int argc, char *argv[])
{

	native_handle_cmd_line(argc, argv);

	hwm_init();

	posix_boot_cpu();

	hwm_main_loop();

	return 0;

}
