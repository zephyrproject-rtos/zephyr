/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_powerquad.h>

#define PQ_NODE DT_NODELABEL(powerquad)
#define PQ_BASE ((POWERQUAD_Type *)DT_REG_ADDR(PQ_NODE))

static int powerquad_init(void)
{
	const struct device *clk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(PQ_NODE));
	int ret;

	if (!device_is_ready(clk_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(clk_dev, (clock_control_subsys_t)DT_CLOCKS_CELL(PQ_NODE, name));
	if (ret < 0) {
		return ret;
	}

	PQ_Init(PQ_BASE);
	return 0;
}

SYS_INIT(powerquad_init, POST_KERNEL, 0);
