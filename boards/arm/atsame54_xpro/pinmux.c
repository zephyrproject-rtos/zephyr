/*
 * Copyright (c) 2019 Benjamin Valentin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(const struct device *dev)
{
	const struct device *muxa = DEVICE_DT_GET(DT_NODELABEL(pinmux_a));
	const struct device *muxb = DEVICE_DT_GET(DT_NODELABEL(pinmux_b));
	const struct device *muxc = DEVICE_DT_GET(DT_NODELABEL(pinmux_c));
	const struct device *muxd = DEVICE_DT_GET(DT_NODELABEL(pinmux_d));

	ARG_UNUSED(dev);

	if (!device_is_ready(muxa)) {
		return -ENXIO;
	}
	if (!device_is_ready(muxb)) {
		return -ENXIO;
	}
	if (!device_is_ready(muxc)) {
		return -ENXIO;
	}
	if (!device_is_ready(muxd)) {
		return -ENXIO;
	}

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_2, CONFIG_PINMUX_INIT_PRIORITY);
