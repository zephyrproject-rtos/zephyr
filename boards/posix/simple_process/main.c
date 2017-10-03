/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The basic principle of operation is:
 *   No asynchronous behavior, no undeterminism.
 *   If you run the same thing 20 times, you get exactly the same result 20
 *   times.
 *   It does not matter if you are running from console, or in a debugger
 *   and you go for lunch in the middle of the debug session.
 *
 * This is achieved as follows:
 * The HW models run in their own simulated time. We do not in any way attempt
 * to link ourselves to the actual real time / wall time of the machine as this
 * would make execution undeterministic and debugging or instrumentation not
 * really possible.
 */

#include <soc.h>
#include "hw_models.h"
#include <stdlib.h>


void main_clean_up(void)
{
	hw_cleanup();
	/*Eventually also cleanup threads in POSIX core*/
	exit(0);
}

/**
 * This is the actual main for the Linux process,
 * the Zephyr application main is renamed something else thru a define.
 *
 * Note that normally one wants to use this POSIX arch to be part of a
 * simulation engine, with some proper HW models and whatnot
 *
 * This is just a very simple demo which is able to run some of the sample
 * apps (hello world, synchronization, philosophers)
 */
int main(void)
{

	hw_init();

	posix_soc_boot_cpu();

	hw_models_main_loop();

	return 0;

}
