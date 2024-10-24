/*
 * Busy-wait-forever poweroff for the CVA6 CPU on the FPGA
 *
 * Copyright(c) 2024, CISPA Helmholtz Center for Information Security
 * SPDX - License - Identifier : Apache-2.0
 */
#include "cv64a6.h"

#include <stdint.h>
#include <stdio.h>

#include <zephyr/sys/poweroff.h>

void z_cv64a6_finish_test(const int32_t status)
{

	printf("Finishing test with status %u-", status);

	if (status == 0) {
		printf("TEST SUCCESS!\n");
	} else {
		printf("TEST FAIL!\n");
	}

	sys_poweroff();
}

void z_sys_poweroff(void)
{
	printf("System poweroff!\n");
	for (;;) {
	}
}
