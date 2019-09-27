/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr.h>
#include <device.h>
#include <counter.h>

#define NR_SAMPLES 10	/* sample timer 10 times */

static u32_t sync(struct device *cmos)
{
	u32_t this, last;

	this = counter_read(cmos);
	do {
		last = this;
		this = counter_read(cmos);
	} while (last == this);

	return z_timer_cycle_get_32();
}

void timer(void)
{
	struct device *cmos;

#if defined(CONFIG_LOAPIC_TIMER)
	printk("TIMER: legacy local APIC");
#elif defined(CONFIG_APIC_TIMER)
	printk("TIMER: new local APIC");
#elif defined(CONFIG_HPET_TIMER)
	printk("TIMER: HPET");
#else
	printk("TIMER: unknown");
#endif

	printk(", configured frequency = %dHz\n",
		sys_clock_hw_cycles_per_sec());

	cmos = device_get_binding("CMOS");
	if (cmos == NULL) {
		printk("\tCan't get reference CMOS clock device.\n");
	} else {
		u64_t sum = 0;

		printk("\tUsing CMOS RTC as reference clock:\n");

		for (int i = 0; i < NR_SAMPLES; ++i) {
			u32_t start, end;

			start = sync(cmos);
			end = sync(cmos);
			sum += end - start;

			printk("\tstart = %u, end = %u, %u cycles\n",
				start, end, end - start);
		}

		printk("\taverage = %uHz\n", (unsigned) (sum / NR_SAMPLES));
	}

	printk("\n");
}
