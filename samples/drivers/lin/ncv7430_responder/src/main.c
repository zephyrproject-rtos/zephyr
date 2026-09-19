/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lin.h>

#include "ncv7430_mock.h"

#define LIN_BUS_BAUDRATE            19200U
#define LIN_BUS_BREAK_LEN           11U
#define LIN_BUS_BREAK_DELIMITER_LEN 1U

static const struct lin_config dut_config = {
	.mode = LIN_MODE_RESPONDER,
	.baudrate = LIN_BUS_BAUDRATE,
	.break_len = LIN_BUS_BREAK_LEN,
	.break_delimiter_len = LIN_BUS_BREAK_DELIMITER_LEN,
	.flags = 0,
};

#if DT_HAS_ALIAS(lin0)
static const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(lin0));
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_lin_sci_b)
static const struct device *const dev = DEVICE_DT_GET(DT_INST(0, renesas_ra_lin_sci_b));
#else
#error "No LIN device found. Please enable at least one LIN device."
#endif

int main(void)
{
	int ret;

	ret = device_is_ready(dev);
	if (!ret) {
		printk("NCV7430 mock device not found\n");
		return 0;
	}

	ret = lin_configure(dev, &dut_config);
	if (ret) {
		printk("LIN configure failed: %d\n", ret);
		return 0;
	}

	ret = ncv7430_mock_init(dev, NCV7430_NODE_ADDRESS);
	if (ret) {
		printk("NCV7430 mock initialization failed: %d\n", ret);
		return 0;
	}

	ret = lin_start(dev);
	if (ret) {
		printk("LIN start failed: %d\n", ret);
		return 0;
	}

	printk("NCV7430 mock responder started\n");

	return 0;
}
