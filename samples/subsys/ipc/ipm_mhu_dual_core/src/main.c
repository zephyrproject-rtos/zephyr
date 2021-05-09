/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/ipm.h>

enum cpu_id_t {
	MHU_CPU0 = 0,
	MHU_CPU1,
	MHU_CPU_MAX,
};

const struct device *mhu0;

static void main_cpu_0(void)
{
	while (1) {
	}
}

static void main_cpu_1(void)
{
	const uint32_t set_mhu = 1;

	ipm_send(mhu0, 0, MHU_CPU0, &set_mhu, 1);

	while (1) {
	}
}

static void mhu_isr_callback(const struct device *dev, void *context,
			     uint32_t cpu_id, volatile void *data)
{
	const uint32_t set_mhu = 1;

	printk("MHU ISR on CPU %d\n", cpu_id);
	if (cpu_id == MHU_CPU0) {
		ipm_send(dev, 0, MHU_CPU1, &set_mhu, 1);
	} else if (cpu_id == MHU_CPU1) {
		printk("MHU Test Done.\n");
	}
}

void main(void)
{
	printk("IPM MHU sample on %s\n", CONFIG_BOARD);

	mhu0 = device_get_binding(DT_LABEL(DT_INST(0, arm_mhu)));
	if (!mhu0) {
		printk("CPU %d, get MHU0 fail!\n",
				sse_200_platform_get_cpu_id());
		while (1) {
		}
	} else {
		printk("CPU %d, get MHU0 success!\n",
				sse_200_platform_get_cpu_id());
		ipm_register_callback(mhu0, mhu_isr_callback, NULL);
	}

	if (sse_200_platform_get_cpu_id() == MHU_CPU0) {
		wakeup_cpu1();
		main_cpu_0();
	}

	main_cpu_1();
}
