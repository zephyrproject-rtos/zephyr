/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "ulp_lp_core_utils.h"
#include <esp_cpu.h>
#include "riscv/rvruntime-frames.h"

/* Override for the ulp_lp_core_panic_handler() so it stops the debugger when abort() is called */
void ulp_lp_core_panic_handler(RvExcFrame *frame, int exccause)
{
	esp_cpu_dbgr_break();
}

void do_crash(void)
{
	volatile int *p = (int *)0x0;
	*p = 32;
	abort();
}

void do_things(int max)
{
	while (1) {
		for (int i = 0; i < max; i++) {
			ulp_lp_core_delay_us(100000);
			if (i > 0) {
				do_crash();
			}
		}
	}
}

int main(void)
{
	do_things(1000000000);
	return 0;
}
