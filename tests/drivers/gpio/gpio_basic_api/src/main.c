/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_gpio.h"

/* Grotesque hack for pinmux boards */
#if defined(CONFIG_BOARD_FRDM_K64F) || defined(CONFIG_BOARD_RV32M1_VEGA)
#include <drivers/pinmux.h>
#include <fsl_port.h>
#elif defined(CONFIG_BOARD_UDOO_NEO_FULL_M4)
#include "device_imx.h"
#elif defined(CONFIG_BOARD_MIMXRT1050_EVK)
#include <fsl_iomuxc.h>
#elif defined(CONFIG_SOC_FAMILY_LPC)
#include <drivers/pinmux.h>
#include "soc.h"
#endif

static void board_setup(void)
{
#if DT_NODE_HAS_STATUS(DT_INST(0, test_gpio_basic_api), okay)
	/* PIN_IN and PIN_OUT must be on same controller. */
	if (strcmp(DT_GPIO_LABEL(DT_INST(0, test_gpio_basic_api), out_gpios),
		   DT_GPIO_LABEL(DT_INST(0, test_gpio_basic_api), in_gpios)) != 0) {
		printk("FATAL: output controller %s != input controller %s\n",
		       DT_GPIO_LABEL(DT_INST(0, test_gpio_basic_api), out_gpios),
		       DT_GPIO_LABEL(DT_INST(0, test_gpio_basic_api), in_gpios));
		k_panic();
	}
#endif

#if defined(CONFIG_BOARD_FRDM_K64F)
	/* TODO figure out how to get this from "GPIO_2" */
	const char *pmx_name = "portc";
	const struct device *pmx = device_get_binding(pmx_name);

	pinmux_pin_set(pmx, PIN_OUT, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(pmx, PIN_IN, PORT_PCR_MUX(kPORT_MuxAsGpio));
#elif defined(CONFIG_BOARD_UDOO_NEO_FULL_M4)
	/*
	 * Configure pin mux.
	 * The following code needs to configure the same GPIOs which were
	 * selected as test pins in device tree.
	 */

	if (strcmp(DEV_NAME, "GPIO_5") != 0) {
		printk("FATAL: controller set in DTS %s != controller %s\n",
		       DEV_NAME, "GPIO_5");
		k_panic();
	}

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
#elif defined(CONFIG_SOC_FAMILY_LPC)
	/* Assumes ARDUINO pins are mapped on PORT0 on all boards*/
	const struct device *port0 = DEVICE_DT_GET(DT_NODELABEL(pio0));
	const uint32_t pin_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_OPENDRAIN_DI
			);
	pinmux_pin_set(port0, PIN_IN,  pin_config);
	pinmux_pin_set(port0, PIN_OUT, pin_config);
#elif defined(CONFIG_BOARD_RV32M1_VEGA)
	const char *pmx_name = DT_LABEL(DT_NODELABEL(porta));
	const struct device *pmx = device_get_binding(pmx_name);

	pinmux_pin_set(pmx, PIN_OUT, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(pmx, PIN_IN, PORT_PCR_MUX(kPORT_MuxAsGpio));
#elif defined(CONFIG_GPIO_EMUL)
	extern struct gpio_callback gpio_emul_callback;
	const struct device *dev = device_get_binding(DEV_NAME);
	zassert_not_equal(dev, NULL,
			  "Device not found");
	int rc = gpio_add_callback(dev, &gpio_emul_callback);
	__ASSERT(rc == 0, "gpio_add_callback() failed: %d", rc);
#endif
}

void test_main(void)
{
	board_setup();
	ztest_test_suite(gpio_basic_test,
			 ztest_unit_test(test_gpio_port),
			 ztest_unit_test(test_gpio_callback_add_remove),
			 ztest_unit_test(test_gpio_callback_self_remove),
			 ztest_unit_test(test_gpio_callback_enable_disable),
			 ztest_unit_test(test_gpio_callback_variants));
	ztest_run_test_suite(gpio_basic_test);
}
