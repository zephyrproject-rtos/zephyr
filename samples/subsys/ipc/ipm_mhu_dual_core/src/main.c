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
	struct ipm_msg msg;
	const uint32_t set_mhu = 1;

	msg.data = &set_mhu;
	msg.size = 1;
	msg.id = MHU_CPU0;

	ipm_send(mhu0, 0, 0, &msg);

	while (1) {
	}
}

static void mhu_isr_callback(const struct device *dev, void *context,
			     uint32_t cpu_id, volatile void *data)
{
	const uint32_t set_mhu = 1;
	struct ipm_msg msg;

	printk("MHU ISR on CPU %d\n", cpu_id);
	if (cpu_id == MHU_CPU0) {
		msg.data = &set_mhu;
		msg.size = 1;
		msg.id = MHU_CPU1;

		ipm_send(dev, 0, 0, &msg);
	} else if (cpu_id == MHU_CPU1) {
		printk("MHU Test Done.\n");
	}
}

void main(void)
{
	printk("IPM MHU sample on %s\n", CONFIG_BOARD);

	mhu0 = DEVICE_DT_GET_ANY(arm_mhu);
	if (!(mhu0 && device_is_ready(mhu0))) {
		printk("CPU %d, get MHU0 fail!\n",
				sse_200_platform_get_cpu_id());
		while (1) {
		}
	} else {
		printk("CPU %d, get MHU0 success!\n",
				sse_200_platform_get_cpu_id());
		ipm_register_callback(mhu0, mhu_isr_callback, 0, NULL);
	}

	if (sse_200_platform_get_cpu_id() == MHU_CPU0) {
		wakeup_cpu1();
		main_cpu_0();
	}

	main_cpu_1();
}
