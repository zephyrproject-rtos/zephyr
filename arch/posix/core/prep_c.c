/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * As for this arch we are running in another OS, and a loader has
 * loaded this program, all sections are already initialized
 *  => nothing to be done here
 *
 *
 * @return N/A
 */
void _PrepC(void)
{
	_Cstart();
	CODE_UNREACHABLE;
}
