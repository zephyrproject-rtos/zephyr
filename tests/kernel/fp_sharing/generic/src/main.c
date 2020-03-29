/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#ifndef CONFIG_FLOAT
#error Rebuild with the FLOAT config option enabled
#endif

#ifndef CONFIG_FP_SHARING
#error Rebuild with the FP_SHARING config option enabled
#endif

#if defined(CONFIG_X86)
#ifndef CONFIG_SSE
#error Rebuild with the SSE config option enabled
#endif
#endif /* CONFIG_X86 */

void main(void)
{
	/* This very old test didn't have a main() function, and would
	 * dump gcov data immediately. Sleep forever, we'll invoke
	 * gcov manually later when the test completes.
	 */
	while (true) {
		k_sleep(K_MSEC(1000));
	}
}
