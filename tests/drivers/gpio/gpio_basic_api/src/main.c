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
#ifdef CONFIG_BOARD_FRDM_K64F
	printk("Setting up pinmux for K64F\n");
	const char *pmx_name = "portc";
	struct device *pmx = device_get_binding(pmx_name);
	printk("pmx %s for %s: %p\n", pmx_name, DEV_NAME, pmx);
	int rc = pinmux_pin_set(pmx, PIN_OUT, PORT_PCR_MUX(kPORT_MuxAsGpio));
	printk("pmx pin %u: %d\n", PIN_OUT, rc);
	rc = pinmux_pin_set(pmx, PIN_IN, PORT_PCR_MUX(kPORT_MuxAsGpio));
	printk("pmx pin %u: %d\n", PIN_IN, rc);
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
			 ztest_unit_test(test_gpio_callback_variants));
	ztest_run_test_suite(gpio_basic_test);
}
