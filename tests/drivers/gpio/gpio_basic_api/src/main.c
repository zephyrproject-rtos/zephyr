/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_gpio.h"

/* Grotesque hack for pinmux boards */
#if defined(CONFIG_BOARD_RV32M1_VEGA)
#include <fsl_port.h>
#elif defined(CONFIG_BOARD_UDOO_NEO_FULL_M4)
#include "device_imx.h"
#elif defined(CONFIG_BOARD_MIMXRT1050_EVK)
#include <fsl_iomuxc.h>
#endif

static void board_setup(void)
{
#if DT_NODE_HAS_STATUS(DT_INST(0, test_gpio_basic_api), okay)
	/* PIN_IN and PIN_OUT must be on same controller. */
	const struct device *const in_dev = DEVICE_DT_GET(DEV_OUT);
	const struct device *const out_dev = DEVICE_DT_GET(DEV_IN);

	if (in_dev != out_dev) {
		printk("FATAL: output controller %s != input controller %s\n",
		       out_dev->name, in_dev->name);
		k_panic();
	}
#endif

#if defined(CONFIG_BOARD_UDOO_NEO_FULL_M4)
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
#elif defined(CONFIG_BOARD_MIMXRT1050_EVK)
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B1_06_GPIO1_IO22, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B1_07_GPIO1_IO23, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_06_GPIO1_IO22,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_HYS_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_07_GPIO1_IO23,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));
#elif defined(CONFIG_GPIO_EMUL)
	extern struct gpio_callback gpio_emul_callback;
	const struct device *const dev = DEVICE_DT_GET(DEV);

	zassert_true(device_is_ready(dev), "GPIO dev is not ready");
	int rc = gpio_add_callback(dev, &gpio_emul_callback);
	__ASSERT(rc == 0, "gpio_add_callback() failed: %d", rc);
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
