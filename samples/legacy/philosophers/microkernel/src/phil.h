/* phil.h - dining philosophers header file*/

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#else
#include <misc/printk.h>
#endif

#define N_PHILOSOPHERS	6
/*
 * There are multiple tasks doing printfs and they may conflict.
 * Therefore use puts() instead of printf().
 */
#if defined(CONFIG_STDOUT_CONSOLE)
#define PRINTF(...) { char output[256]; \
		      sprintf(output, __VA_ARGS__); puts(output); }
#else
#define PRINTF(...) printk(__VA_ARGS__)
#endif
