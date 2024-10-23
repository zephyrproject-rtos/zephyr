/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_gpio.h"

/* Grotesque hack for pinmux boards */
#if defined(CONFIG_BOARD_RV32M1_VEGA)
#include <fsl_port.h>
#elif defined(CONFIG_BOARD_UDOO_NEO_FULL_MCIMX6X_M4)
#include "device_imx.h"
#elif defined(CONFIG_BOARD_MIMXRT1050_EVK)
#include <fsl_iomuxc.h>
#elif defined(CONFIG_BOARD_NRF52_BSIM)
#include <NRF_GPIO.h>
#endif

static void board_setup(void)
{
#if defined(CONFIG_BOARD_UDOO_NEO_FULL_MCIMX6X_M4)
	/*
	 * Configure pin mux.
	 * The following code needs to configure the same GPIOs which were
	 * selected as test pins in device tree.
	 */

	if (PIN_IN != 15) {
		printk("FATAL: input pin set in DTS %d != %d\n", PIN_IN, 15);
		k_panic();
	}

	if (PIN_OUT != 14) {
		printk("FATAL: output pin set in DTS %d != %d\n", PIN_OUT, 14);
		k_panic();
	}

	/* Configure pin RGMII2_RD2 as GPIO5_IO14. */
	IOMUXC_SW_MUX_CTL_PAD_RGMII2_RD2 =
				IOMUXC_SW_MUX_CTL_PAD_RGMII2_RD2_MUX_MODE(5);
	/* Select pull enabled, speed 100 MHz, drive strength 43 ohm */
	IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD2 =
				IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD2_PUE_MASK |
				IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD2_PKE_MASK |
				IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD2_SPEED(2) |
				IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD2_DSE(6);

	/* Configure pin RGMII2_RD3 as GPIO5_IO15. */
	IOMUXC_SW_MUX_CTL_PAD_RGMII2_RD3 =
				IOMUXC_SW_MUX_CTL_PAD_RGMII2_RD3_MUX_MODE(5);
	/* Select pull enabled, speed 100 MHz, drive strength 43 ohm */
	IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD3 =
				IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD3_PUE_MASK |
				IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD3_PKE_MASK |
				IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD3_SPEED(2) |
				IOMUXC_SW_PAD_CTL_PAD_RGMII2_RD3_DSE(6);
#elif defined(CONFIG_GPIO_EMUL)
	extern struct gpio_callback gpio_emul_callback;
	const struct device *const dev = DEVICE_DT_GET(DEV);

	zassert_true(device_is_ready(dev), "GPIO dev is not ready");
	int rc = gpio_add_callback(dev, &gpio_emul_callback);
	__ASSERT(rc == 0, "gpio_add_callback() failed: %d", rc);
#elif defined(CONFIG_BOARD_NRF52_BSIM)
	static bool done;

	if (!done) {
		done = true;
		/* This functions allows to programmatically short-circuit SOC GPIO pins */
		nrf_gpio_backend_register_short(1, PIN_OUT, 1, PIN_IN);
	}
#endif
}

static void *gpio_basic_setup(void)
{
	board_setup();

	return NULL;
}

/* Test GPIO port configuration */
ZTEST_SUITE(gpio_port, NULL, gpio_basic_setup, NULL, NULL, NULL);

/* Test GPIO callback management */
ZTEST_SUITE(gpio_port_cb_mgmt, NULL, gpio_basic_setup, NULL, NULL, NULL);

/* Test GPIO callbacks */
ZTEST_SUITE(gpio_port_cb_vari, NULL, gpio_basic_setup, NULL, NULL, NULL);

/* Test GPIO port configuration influence on callbacks. Want to run just
 * after flash, hence the name starting in 'a'
 */
ZTEST_SUITE(after_flash_gpio_config_trigger, NULL, gpio_basic_setup, NULL, NULL,
	    NULL);
