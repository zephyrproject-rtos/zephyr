/*
 * CVA6 testbench poweroff / test finish operations.
 *
 * Copyright(c) 2024, CISPA Helmholtz Center for Information Security
 * SPDX-License-Identifier: Apache-2.0
 */
#include "cva6.h"

#include <stdint.h>
#include <stdio.h>

#include <zephyr/sys/poweroff.h>

/*
 * the CV64a6 testbench looks for a symbol called "tohost" and determines its load address
 * when something is written to the symbol, this terminates the test
 * the last bit of the written data indicate success or failure of the test
 */

/* the test bench must be able to find the symbol in the ELF by reading its symbols
 * this is accomplished using a linker script
 */

volatile int32_t tohost;

static int32_t cv64a6_test_status;

void z_cva6_finish_test(const int32_t status)
{
	cv64a6_test_status = status;

	sys_poweroff();
}

void z_sys_poweroff(void)
{
	/* write to this special address signals to the sim that we wish to end the simulation */
	tohost = 0x1 | (cv64a6_test_status << 1);

	CODE_UNREACHABLE;
}
