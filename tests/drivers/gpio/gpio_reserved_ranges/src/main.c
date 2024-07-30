/*
 * Copyright (c) 2023 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#define TEST_GPIO_1 DT_NODELABEL(test_gpio_1)
#define TEST_GPIO_2 DT_NODELABEL(test_gpio_2)
#define TEST_GPIO_3 DT_NODELABEL(test_gpio_3)
#define TEST_GPIO_4 DT_NODELABEL(test_gpio_4)
#define TEST_GPIO_5 DT_NODELABEL(test_gpio_5)
#define TEST_GPIO_6 DT_NODELABEL(test_gpio_6)

#define DT_DRV_COMPAT vnd_gpio_device

ZTEST(gpio_reserved_ranges, test_path_props)
{
	zassert_true(DT_NODE_HAS_PROP(TEST_GPIO_1, gpio_reserved_ranges), "");
	zassert_true(DT_NODE_HAS_PROP(TEST_GPIO_2, gpio_reserved_ranges), "");
	zassert_true(DT_NODE_HAS_PROP(TEST_GPIO_3, gpio_reserved_ranges), "");
	zassert_true(DT_NODE_HAS_PROP(TEST_GPIO_4, gpio_reserved_ranges), "");
	zassert_true(DT_NODE_HAS_PROP(TEST_GPIO_5, gpio_reserved_ranges), "");
	zassert_false(DT_NODE_HAS_PROP(TEST_GPIO_6, gpio_reserved_ranges), "");
}

ZTEST(gpio_reserved_ranges, test_has_status)
{
	zassert_equal(DT_NODE_HAS_STATUS(TEST_GPIO_1, okay), 1, "");
	zassert_equal(DT_NODE_HAS_STATUS(TEST_GPIO_2, okay), 1, "");
	zassert_equal(DT_NODE_HAS_STATUS(TEST_GPIO_3, okay), 1, "");
	zassert_equal(DT_NODE_HAS_STATUS(TEST_GPIO_4, okay), 1, "");
	zassert_equal(DT_NODE_HAS_STATUS(TEST_GPIO_5, okay), 1, "");
	zassert_equal(DT_NODE_HAS_STATUS(TEST_GPIO_6, okay), 1, "");
}

ZTEST(gpio_reserved_ranges, test_reserved_ranges)
{
	/* GPIO_DT_INST_RESERVED_RANGES_NGPIOS */
	zassert_equal(GPIO_DT_RESERVED_RANGES_NGPIOS(TEST_GPIO_1, 32),
			0xdeadbeef, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES_NGPIOS(TEST_GPIO_2, 32),
			0x7fffbeff, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES_NGPIOS(TEST_GPIO_3, 18),
			0xfffc0418, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES_NGPIOS(TEST_GPIO_4, 16),
			0xfffffff0, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES_NGPIOS(TEST_GPIO_5, 0),
			0xffffffff, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES_NGPIOS(TEST_GPIO_6, 32),
			0, "");

	/* GPIO_DT_INST_RESERVED_RANGES_NGPIOS */
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES_NGPIOS(0, 32), 0xdeadbeef,
			"");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES_NGPIOS(1, 32), 0x7fffbeff,
			"");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES_NGPIOS(2, 18), 0xfffc0418,
			"");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES_NGPIOS(3, 16), 0xfffffff0,
			"");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES_NGPIOS(4, 0), 0xffffffff,
			"");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES_NGPIOS(5, 32), 0, "");

	/* GPIO_DT_RESERVED_RANGES */
	zassert_equal(GPIO_DT_RESERVED_RANGES(TEST_GPIO_1), 0xdeadbeef, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES(TEST_GPIO_2), 0x7fffbeff, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES(TEST_GPIO_3), 0xfffc0418, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES(TEST_GPIO_4), 0xfffffff0, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES(TEST_GPIO_5), 0xffffffff, "");
	zassert_equal(GPIO_DT_RESERVED_RANGES(TEST_GPIO_6), 0x0, "");

	/* GPIO_DT_INST_RESERVED_RANGES */
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES(0), 0xdeadbeef, "");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES(1), 0x7fffbeff, "");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES(2), 0xfffc0418, "");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES(3), 0xfffffff0, "");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES(4), 0xffffffff, "");
	zassert_equal(GPIO_DT_INST_RESERVED_RANGES(5), 0x0, "");
}

ZTEST(gpio_reserved_ranges, test_port_pin_mask_exc)
{
	/* GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC */
	zassert_equal(GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(TEST_GPIO_1, 32),
			0x21524110, "");
	zassert_equal(GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(TEST_GPIO_2, 32),
			0x80004100, "");
	zassert_equal(GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(TEST_GPIO_3, 18),
			0x0003fbe7, "");
	zassert_equal(GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(TEST_GPIO_4, 16),
			0x0000000f, "");
	zassert_equal(GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(TEST_GPIO_5, 0),
			0x00000000, "");
	zassert_equal(GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(TEST_GPIO_6, 32),
			0xffffffff, "");

	/* GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC */
	zassert_equal(GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(0, 32), 0x21524110,
			"");
	zassert_equal(GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(1, 32), 0x80004100,
			"");
	zassert_equal(GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(2, 18), 0x0003fbe7,
			"");
	zassert_equal(GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(3, 16), 0x0000000f,
			"");
	zassert_equal(GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(4, 0), 0x00000000,
			"");
	zassert_equal(GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(5, 16), 0x0000ffff,
			"");

	/* GPIO_PORT_PIN_MASK_FROM_DT_NODE */
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_NODE(TEST_GPIO_1),
			0x21524110, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_NODE(TEST_GPIO_2),
			0x80004100, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_NODE(TEST_GPIO_3),
			0x0003fbe7, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_NODE(TEST_GPIO_4),
			0x0000000f, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_NODE(TEST_GPIO_5),
			0x00000000, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_NODE(TEST_GPIO_6),
			0xffffffff, "");

	/* GPIO_PORT_PIN_MASK_FROM_DT_INST */
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_INST(0), 0x21524110, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_INST(1), 0x80004100, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_INST(2), 0x0003fbe7, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_INST(3), 0x0000000f, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_INST(4), 0x00000000, "");
	zassert_equal(GPIO_PORT_PIN_MASK_FROM_DT_INST(5), 0xffffffff, "");
}

/* Test GPIO port configuration */
ZTEST_SUITE(gpio_reserved_ranges, NULL, NULL, NULL, NULL, NULL);
