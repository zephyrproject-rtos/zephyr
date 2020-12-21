/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/counter.h>

#define NR_SAMPLES 10	/* sample timer 10 times */

static uint32_t sync(const struct device *cmos)
{
	uint32_t this, last;
	int err;

	err = counter_get_value(cmos, &this);
	if (err) {
		printk("\tCan't read CMOS clock device.\n");
		return 0;
	}

	do {
		last = this;
		err = counter_get_value(cmos, &this);
		if (err) {
			printk("\tCan't read CMOS clock device.\n");
			return 0;
		}
	} while (last == this);

	return z_timer_cycle_get_32();
}

void timer(void)
{
	const struct device *cmos;

#if defined(CONFIG_APIC_TIMER)
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
		uint64_t sum = 0;

		printk("\tUsing CMOS RTC as reference clock:\n");

		for (int i = 0; i < NR_SAMPLES; ++i) {
			uint32_t start, end;

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
