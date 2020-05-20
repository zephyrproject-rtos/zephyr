/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief Execute initialization routines referenced in .init_array section
 */

typedef void (*func_ptr)(void);

extern func_ptr __init_array_start[];
extern func_ptr __init_array_end[];

/**
 * @brief Execute initialization routines referenced in .init_array section
 *
 * @return N/A
 */
void __do_init_array_aux(void)
{
	for (func_ptr *func = __init_array_start;
		func < __init_array_end;
		func++) {
		(*func)();
	}
}
