/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <drivers/pinmux.h>
#include "iomux.h"

/*
 * :::::::::::: NOTE ::::::::::::
 * For a list of possible I/O MUX settings,
 * See soc/xtensa/intel_s1000/iomux.h
 */

/*
 * Initializes the I/O MUX with the setting needed per Intel S1000 CRB
 * For customizations, please refer to the table above for available settings
 * Please note that a call to pinmux_pin_set is only needed when a setting
 * that is not default is required
 */
static int intel_s1000_pinmux_init(struct device *dev)
{
	struct device *pinmux;

	pinmux = device_get_binding(CONFIG_PINMUX_NAME);

	if (pinmux == NULL) {
		return -ENXIO;
	}

	/* Select PDM instead of I2S0 since board has 8 microphones */
	pinmux_pin_set(pinmux, PIN_GROUP(I2S0), PINMUX_FUNC_B);

	/*
	 * I2S3 is wired to the host interface connector.
	 * Select GPIO to avoid any conflict with hosts that may be driving
	 * the signals.
	 */
	pinmux_pin_set(pinmux, PIN_GROUP(I2S3), PINMUX_FUNC_B);

	/* TI DAC is on I2C1. Usually, there is no device on I2C0 */
	pinmux_pin_set(pinmux, PIN_GROUP(I2C), PINMUX_FUNC_B);

	/* Intel S1000 CRB has an octal SPI flash. Select MST_DQ */
	pinmux_pin_set(pinmux, PIN_GROUP(EM_DQ), PINMUX_FUNC_B);

	return 0;
}

SYS_INIT(intel_s1000_pinmux_init, PRE_KERNEL_2, CONFIG_PINMUX_INIT_PRIORITY);
