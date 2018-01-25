/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: call _Cstart().
 *
 */

#include <stddef.h>
#include <toolchain.h>

#include <mips/m32c0.h>

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */

extern FUNC_NORETURN void _Cstart(void);

void software_init_hook(void)
{
#ifdef CONFIG_XIP
	_data_copy();
#endif
	_Cstart();
	CODE_UNREACHABLE;
}
