/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_gpio.h"

/* Grotesque hack for pinmux boards */
#ifdef CONFIG_BOARD_FRDM_K64F
#include <drivers/pinmux.h>
#include <fsl_port.h>
#endif

static void board_setup(void)
{
#ifdef DT_INST_0_TEST_GPIO_BASIC_API
	/* PIN_IN and PIN_OUT must be on same controller. */
	if (strcmp(DT_INST_0_TEST_GPIO_BASIC_API_OUT_GPIOS_CONTROLLER,
		   DT_INST_0_TEST_GPIO_BASIC_API_IN_GPIOS_CONTROLLER) != 0) {
		printk("FATAL: output controller %s != input controller %s\n",
		       DT_INST_0_TEST_GPIO_BASIC_API_OUT_GPIOS_CONTROLLER,
		       DT_INST_0_TEST_GPIO_BASIC_API_IN_GPIOS_CONTROLLER);
		k_panic();
	}
#endif

#ifdef CONFIG_BOARD_FRDM_K64F
	/* TODO figure out how to get this from "GPIO_2" */
	const char *pmx_name = "portc";
	struct device *pmx = device_get_binding(pmx_name);

	pinmux_pin_set(pmx, PIN_OUT, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(pmx, PIN_IN, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif
}

void test_main(void)
{
	board_setup();
	ztest_test_suite(gpio_basic_test,
			 ztest_unit_test(test_gpio_port),
			 ztest_unit_test(test_gpio_pin_read_write),
			 ztest_unit_test(test_gpio_callback_add_remove),
			 ztest_unit_test(test_gpio_callback_self_remove),
			 ztest_unit_test(test_gpio_callback_enable_disable),
			 ztest_unit_test(test_gpio_callback_edge_high),
			 ztest_unit_test(test_gpio_callback_edge_low),
			 ztest_unit_test(test_gpio_callback_level_high),
			 ztest_unit_test(test_gpio_callback_level_low));
	ztest_run_test_suite(gpio_basic_test);
}
