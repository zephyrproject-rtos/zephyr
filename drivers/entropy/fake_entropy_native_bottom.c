/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bottom/host side of the pseudo-random entropy generator for the native simulator
 */

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif

#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700

#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#ifndef __APPLE__
#include <sys/random.h>
#endif
#include "nsi_tracing.h"

void entropy_native_seed(unsigned int seed, bool seed_random)
{
	if (seed_random == false) {
		srandom(seed);
	} else {
		unsigned int buf;
#ifdef __APPLE__
		arc4random_buf(&buf, sizeof(buf));
#else
		int err = getrandom(&buf, sizeof(buf), 0);

		if (err != sizeof(buf)) {
			nsi_print_error_and_exit("Could not get random number (%i, %s)\n",
						 err, strerror(errno));
		}
#endif
		srandom(buf);

		/* Let's print the seed so users can still reproduce the run if they need to */
		nsi_print_trace("Random generator seeded with 0x%X\n", buf);
	}
}
