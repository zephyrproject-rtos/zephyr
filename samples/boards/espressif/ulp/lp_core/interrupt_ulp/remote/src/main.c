/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_interrupts.h"

volatile uint32_t lp_core_pmu_intr_count;

void ulp_lp_core_lp_pmu_intr_handler(void)
{
	ulp_lp_core_sw_intr_clear();
	lp_core_pmu_intr_count++;
	printf("LP PMU interrupt received: %d\n", lp_core_pmu_intr_count);
}

int main(void)
{
	lp_core_pmu_intr_count = 0;
	ulp_lp_core_intr_enable();
	ulp_lp_core_sw_intr_enable(true);

	while (1) {
		/* Wait forever, handling interrupts */
		asm volatile("wfi");
	}
	return 0;
}
