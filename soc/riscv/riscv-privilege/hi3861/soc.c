/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/syscon.h>

#define GLB_CTL_GLB_WDT_RST_SEL_OFFSET (0x30)

static const struct device *glb_ctl = DEVICE_DT_GET(DT_NODELABEL(glb_ctl));

static int soc_hi3861_init(void)
{
	/*
	 * WDT was enabled by the romboot to check if the application is up and
	 * running. It is expected to be disabled by default in Zephyr, unless
	 * the WDT driver is explicitly enabled.
	 */
	syscon_write_reg(glb_ctl, GLB_CTL_GLB_WDT_RST_SEL_OFFSET, 0);

	return 0;
}

SYS_INIT(soc_hi3861_init, PRE_KERNEL_1, 0);
