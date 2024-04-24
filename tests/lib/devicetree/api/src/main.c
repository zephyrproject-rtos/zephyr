/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mbox.h>

#include <stdlib.h>

#define TEST_CHILDREN	DT_PATH(test, test_children)
#define TEST_DEADBEEF	DT_PATH(test, gpio_deadbeef)
#define TEST_ABCD1234	DT_PATH(test, gpio_abcd1234)
#define TEST_ALIAS	DT_ALIAS(test_alias)
#define TEST_NODELABEL	DT_NODELABEL(test_nodelabel)
#define TEST_INST	DT_INST(0, vnd_gpio_device)
#define TEST_ARRAYS	DT_NODELABEL(test_arrays)
#define TEST_PH	DT_NODELABEL(test_phandles)
#define TEST_INTC	DT_NODELABEL(test_intc)
#define TEST_IRQ	DT_NODELABEL(test_irq)
#define TEST_IRQ_EXT	DT_NODELABEL(test_irq_extended)
#define TEST_TEMP	DT_NODELABEL(test_temp_sensor)
#define TEST_REG	DT_NODELABEL(test_reg)
#define TEST_VENDOR	DT_NODELABEL(test_vendor)
#define TEST_MODEL	DT_NODELABEL(test_vendor)
#define TEST_ENUM_0	DT_NODELABEL(test_enum_0)
#define TEST_64BIT	DT_NODELABEL(test_reg_64)
#define TEST_INTC	DT_NODELABEL(test_intc)

#define TEST_I2C DT_NODELABEL(test_i2c)
#define TEST_I2C_DEV DT_PATH(test, i2c_11112222, test_i2c_dev_10)
#define TEST_I2C_BUS DT_BUS(TEST_I2C_DEV)

#define TEST_I2C_MUX DT_NODELABEL(test_i2c_mux)
#define TEST_I2C_MUX_CTLR_1 DT_CHILD(TEST_I2C_MUX, i2c_mux_ctlr_1)
#define TEST_I2C_MUX_CTLR_2 DT_CHILD(TEST_I2C_MUX, i2c_mux_ctlr_2)
#define TEST_MUXED_I2C_DEV_1 DT_NODELABEL(test_muxed_i2c_dev_1)
#define TEST_MUXED_I2C_DEV_2 DT_NODELABEL(test_muxed_i2c_dev_2)

#define TEST_I3C DT_NODELABEL(test_i3c)
#define TEST_I3C_DEV DT_PATH(test, i3c_88889999, test_i3c_dev_420000abcd12345678)
#define TEST_I3C_BUS DT_BUS(TEST_I3C_DEV)

#define TEST_GPIO_1 DT_NODELABEL(test_gpio_1)
#define TEST_GPIO_2 DT_NODELABEL(test_gpio_2)
#define TEST_GPIO_4 DT_NODELABEL(test_gpio_4)

#define TEST_GPIO_HOG_1 DT_PATH(test, gpio_deadbeef, test_gpio_hog_1)
#define TEST_GPIO_HOG_2 DT_PATH(test, gpio_deadbeef, test_gpio_hog_2)
#define TEST_GPIO_HOG_3 DT_PATH(test, gpio_abcd1234, test_gpio_hog_3)

#define TEST_SPI DT_NODELABEL(test_spi)

#define TEST_SPI_DEV_0 DT_PATH(test, spi_33334444, test_spi_dev_0)
#define TEST_SPI_BUS_0 DT_BUS(TEST_SPI_DEV_0)

#define TEST_SPI_DEV_1 DT_PATH(test, spi_33334444, test_spi_dev_1)
#define TEST_SPI_BUS_1 DT_BUS(TEST_SPI_DEV_1)

#define TEST_SPI_NO_CS DT_NODELABEL(test_spi_no_cs)
#define TEST_SPI_DEV_NO_CS DT_NODELABEL(test_spi_no_cs)

#define TEST_PWM_CTLR_1 DT_NODELABEL(test_pwm1)
#define TEST_PWM_CTLR_2 DT_NODELABEL(test_pwm2)

#define TEST_CAN_CTRL_0 DT_NODELABEL(test_can0)
#define TEST_CAN_CTRL_1 DT_NODELABEL(test_can1)
#define TEST_CAN_CTRL_2 DT_NODELABEL(test_can2)
#define TEST_CAN_CTRL_3 DT_NODELABEL(test_can3)

#define TEST_DMA_CTLR_1 DT_NODELABEL(test_dma1)
#define TEST_DMA_CTLR_2 DT_NODELABEL(test_dma2)

#define TEST_IO_CHANNEL_CTLR_1 DT_NODELABEL(test_adc_1)
#define TEST_IO_CHANNEL_CTLR_2 DT_NODELABEL(test_adc_2)

#define TEST_RANGES_PCIE  DT_NODELABEL(test_ranges_pcie)
#define TEST_RANGES_OTHER DT_NODELABEL(test_ranges_other)
#define TEST_RANGES_EMPTY DT_NODELABEL(test_ranges_empty)

#define TEST_MTD_0 DT_PATH(test, test_mtd_ffeeddcc)
#define TEST_MTD_1 DT_PATH(test, test_mtd_33221100)

#define TEST_MEM_0 DT_CHILD(TEST_MTD_0, flash_20000000)

#define TEST_PARTITION_0 DT_PATH(test, test_mtd_ffeeddcc, flash_20000000, partitions, partition_0)
#define TEST_PARTITION_1 DT_PATH(test, test_mtd_ffeeddcc, flash_20000000, partitions, partition_c0)
#define TEST_PARTITION_2 DT_PATH(test, test_mtd_33221100, partitions, partition_6ff80)

#define ZEPHYR_USER DT_PATH(zephyr_user)

#define TA_HAS_COMPAT(compat) DT_NODE_HAS_COMPAT(TEST_ARRAYS, compat)

#define TO_STRING(x) TO_STRING_(x)
#define TO_STRING_(x) #x

ZTEST(devicetree_api, test_path_props)
{
	zassert_equal(DT_NUM_REGS(TEST_DEADBEEF), 1, "");
	zassert_equal(DT_REG_ADDR(TEST_DEADBEEF), 0xdeadbeef, "");
	zassert_equal(DT_REG_SIZE(TEST_DEADBEEF), 0x1000, "");
	zassert_equal(DT_PROP(TEST_DEADBEEF, gpio_controller), 1, "");
	zassert_equal(DT_PROP(TEST_DEADBEEF, ngpios), 100, "");
	zassert_true(!strcmp(DT_PROP(TEST_DEADBEEF, status), "okay"), "");
	zassert_equal(DT_PROP_LEN(TEST_DEADBEEF, compatible), 1, "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_DEADBEEF, compatible, 0),
			     "vnd,gpio-device"), "");
	zassert_true(DT_NODE_HAS_PROP(TEST_DEADBEEF, status), "");
	zassert_false(DT_NODE_HAS_PROP(TEST_DEADBEEF, foobar), "");

	zassert_true(DT_SAME_NODE(TEST_ABCD1234, TEST_GPIO_2), "");
	zassert_equal(DT_NUM_REGS(TEST_ABCD1234), 2, "");
	zassert_equal(DT_PROP(TEST_ABCD1234, gpio_controller), 1, "");
	zassert_equal(DT_PROP(TEST_ABCD1234, ngpios), 200, "");
	zassert_true(!strcmp(DT_PROP(TEST_ABCD1234, status), "okay"), "");
	zassert_equal(DT_PROP_LEN(TEST_ABCD1234, compatible), 1, "");
	zassert_equal(DT_PROP_LEN_OR(TEST_ABCD1234, compatible, 4), 1, "");
	zassert_equal(DT_PROP_LEN_OR(TEST_ABCD1234, invalid_property, 0), 0, "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ABCD1234, compatible, 0),
			     "vnd,gpio-device"), "");
}

ZTEST(devicetree_api, test_alias_props)
{
	zassert_equal(DT_NUM_REGS(TEST_ALIAS), 1, "");
	zassert_equal(DT_REG_ADDR(TEST_ALIAS), 0xdeadbeef, "");
	zassert_equal(DT_REG_SIZE(TEST_ALIAS), 0x1000, "");
	zassert_true(DT_SAME_NODE(TEST_ALIAS, TEST_GPIO_1), "");
	zassert_equal(DT_PROP(TEST_ALIAS, gpio_controller), 1, "");
	zassert_equal(DT_PROP(TEST_ALIAS, ngpios), 100, "");
	zassert_true(!strcmp(DT_PROP(TEST_ALIAS, status), "okay"), "");
	zassert_equal(DT_PROP_LEN(TEST_ALIAS, compatible), 1, "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ALIAS, compatible, 0),
			     "vnd,gpio-device"), "");
}

ZTEST(devicetree_api, test_nodelabel_props)
{
	zassert_equal(DT_NUM_REGS(TEST_NODELABEL), 1, "");
	zassert_equal(DT_REG_ADDR(TEST_NODELABEL), 0xdeadbeef, "");
	zassert_equal(DT_REG_SIZE(TEST_NODELABEL), 0x1000, "");
	zassert_equal(DT_PROP(TEST_NODELABEL, gpio_controller), 1, "");
	zassert_equal(DT_PROP(TEST_NODELABEL, ngpios), 100, "");
	zassert_true(!strcmp(DT_PROP(TEST_NODELABEL, status), "okay"), "");
	zassert_equal(DT_PROP_LEN(TEST_NODELABEL, compatible), 1, "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_NODELABEL, compatible, 0),
			     "vnd,gpio-device"), "");
	zassert_equal(DT_PROP_LEN(TEST_ENUM_0, val), 1, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_gpio_device
ZTEST(devicetree_api, test_inst_props)
{
	/*
	 * Careful:
	 *
	 * We can only test properties that are shared across all
	 * instances of this compatible here. This includes instances
	 * with status "disabled".
	 */

	zassert_equal(DT_PROP(TEST_INST, gpio_controller), 1, "");
	zassert_true(!strcmp(DT_PROP(TEST_INST, status), "okay") ||
		     !strcmp(DT_PROP(TEST_INST, status), "disabled"), "");
	zassert_equal(DT_PROP_LEN(TEST_INST, compatible), 1, "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_INST, compatible, 0),
			     "vnd,gpio-device"), "");

	zassert_equal(DT_INST_NODE_HAS_PROP(0, gpio_controller), 1, "");
	zassert_equal(DT_INST_PROP(0, gpio_controller), 1, "");
	zassert_equal(DT_INST_NODE_HAS_PROP(0, xxxx), 0, "");
	zassert_true(!strcmp(DT_INST_PROP(0, status), "okay") ||
		     !strcmp(DT_PROP(TEST_INST, status), "disabled"), "");
	zassert_equal(DT_INST_PROP_LEN(0, compatible), 1, "");
	zassert_true(!strcmp(DT_INST_PROP_BY_IDX(0, compatible, 0),
			     "vnd,gpio-device"), "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_device_with_props
ZTEST(devicetree_api, test_any_inst_prop)
{
	zassert_equal(DT_ANY_INST_HAS_PROP_STATUS_OKAY(foo), 1, "");
	zassert_equal(DT_ANY_INST_HAS_PROP_STATUS_OKAY(bar), 1, "");
	zassert_equal(DT_ANY_INST_HAS_PROP_STATUS_OKAY(baz), 0, "");
	zassert_equal(DT_ANY_INST_HAS_PROP_STATUS_OKAY(does_not_exist), 0, "");

	zassert_equal(COND_CODE_1(DT_ANY_INST_HAS_PROP_STATUS_OKAY(foo),
				  (5), (6)),
		      5, "");
	zassert_equal(COND_CODE_0(DT_ANY_INST_HAS_PROP_STATUS_OKAY(foo),
				  (5), (6)),
		      6, "");
	zassert_equal(COND_CODE_1(DT_ANY_INST_HAS_PROP_STATUS_OKAY(baz),
				  (5), (6)),
		      6, "");
	zassert_equal(COND_CODE_0(DT_ANY_INST_HAS_PROP_STATUS_OKAY(baz),
				  (5), (6)),
		      5, "");
	zassert_true(IS_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(foo)), "");
	zassert_true(!IS_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(baz)), "");
	zassert_equal(IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(foo), (1)) + 1,
		      2, "");
	zassert_equal(IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(baz), (1)) + 1,
		      1, "");
}

ZTEST(devicetree_api, test_default_prop_access)
{
	/*
	 * The APIs guarantee that the default_value is not expanded
	 * if the relevant property or cell is defined. This "X" macro
	 * is meant as poison which causes (hopefully) easy to
	 * understand build errors if this guarantee is not met due to
	 * a regression.
	 */
#undef X
#define X do.not.expand.this.argument

	/* Node identifier variants. */
	zassert_equal(DT_PROP_OR(TEST_REG, misc_prop, X), 1234, "");
	zassert_equal(DT_PROP_OR(TEST_REG, not_a_property, -1), -1, "");

	zassert_equal(DT_PHA_BY_IDX_OR(TEST_TEMP, dmas, 1, channel, X), 3, "");
	zassert_equal(DT_PHA_BY_IDX_OR(TEST_TEMP, dmas, 1, not_a_cell, -1), -1,
		      "");

	zassert_equal(DT_PHA_OR(TEST_TEMP, dmas, channel, X), 1, "");
	zassert_equal(DT_PHA_OR(TEST_TEMP, dmas, not_a_cell, -1), -1, "");

	zassert_equal(DT_PHA_BY_NAME_OR(TEST_TEMP, dmas, tx, channel, X), 1,
		      "");
	zassert_equal(DT_PHA_BY_NAME_OR(TEST_TEMP, dmas, tx, not_a_cell, -1),
		      -1, "");

	/* Instance number variants. */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_reg_holder
	zassert_equal(DT_INST_PROP_OR(0, misc_prop, X), 1234, "");
	zassert_equal(DT_INST_PROP_OR(0, not_a_property, -1), -1, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_array_holder
	zassert_equal(DT_INST_PROP_LEN_OR(0, a, X), 3, "");
	zassert_equal(DT_INST_PROP_LEN_OR(0, not_a_property, -1), -1, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
	zassert_equal(DT_INST_PHA_BY_IDX_OR(0, dmas, 1, channel, X), 3, "");
	zassert_equal(DT_INST_PHA_BY_IDX_OR(0, dmas, 1, not_a_cell, -1), -1,
		      "");

	zassert_equal(DT_INST_PHA_OR(0, dmas, channel, X), 1, "");
	zassert_equal(DT_INST_PHA_OR(0, dmas, not_a_cell, -1), -1, "");

	zassert_equal(DT_INST_PHA_BY_NAME_OR(0, dmas, tx, channel, X), 1,
		      "");
	zassert_equal(DT_INST_PHA_BY_NAME_OR(0, dmas, tx, not_a_cell, -1), -1,
		      "");

#undef X
}

ZTEST(devicetree_api, test_has_path)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_PATH(test, gpio_0), okay), 0, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_PATH(test, gpio_deadbeef), okay), 1,
		      "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_PATH(test, gpio_abcd1234), okay), 1,
		      "");
}

ZTEST(devicetree_api, test_has_alias)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_ALIAS(test_alias), okay), 1, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_ALIAS(test_undef), okay), 0, "");
}

ZTEST(devicetree_api, test_inst_checks)
{
	zassert_equal(DT_NODE_EXISTS(DT_INST(0, vnd_gpio_device)), 1, "");
	zassert_equal(DT_NODE_EXISTS(DT_INST(1, vnd_gpio_device)), 1, "");
	zassert_equal(DT_NODE_EXISTS(DT_INST(2, vnd_gpio_device)), 1, "");

	zassert_equal(DT_NUM_INST_STATUS_OKAY(vnd_gpio_device), 2, "");
	zassert_equal(DT_NUM_INST_STATUS_OKAY(xxxx), 0, "");
}

ZTEST(devicetree_api, test_has_nodelabel)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), okay), 0,
		      "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_nodelabel), okay), 1,
		      "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_nodelabel_allcaps),
					 okay),
		      1, "");
}

ZTEST(devicetree_api, test_has_compat)
{
	unsigned int compats;

	zassert_true(DT_HAS_COMPAT_STATUS_OKAY(vnd_gpio_device), "");
	zassert_true(DT_HAS_COMPAT_STATUS_OKAY(vnd_gpio_device), "");
	zassert_false(DT_HAS_COMPAT_STATUS_OKAY(vnd_disabled_compat), "");
	zassert_false(DT_HAS_COMPAT_STATUS_OKAY(vnd_reserved_compat), "");

	zassert_equal(TA_HAS_COMPAT(vnd_array_holder), 1, "");
	zassert_equal(TA_HAS_COMPAT(vnd_undefined_compat), 1, "");
	zassert_equal(TA_HAS_COMPAT(vnd_not_a_test_array_compat), 0, "");
	compats = ((TA_HAS_COMPAT(vnd_array_holder) << 0) |
		   (TA_HAS_COMPAT(vnd_undefined_compat) << 1) |
		   (TA_HAS_COMPAT(vnd_not_a_test_array_compat) << 2));
	zassert_equal(compats, 0x3, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_model1
	zassert_true(DT_INST_NODE_HAS_COMPAT(0, zephyr_model2));
}

ZTEST(devicetree_api, test_has_status)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_gpio_1), okay),
		      1, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_gpio_1), disabled),
		      0, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_gpio_1), reserved),
		      0, "");

	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_no_status), okay),
		      1, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_no_status), disabled),
		      0, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_no_status), reserved),
		      0, "");

	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), disabled),
		      1, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), okay),
		      0, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), reserved),
		      0, "");

	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(reserved_gpio), reserved),
		      1, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(reserved_gpio), disabled),
		      0, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(reserved_gpio), okay),
		      0, "");
}

ZTEST(devicetree_api, test_bus)
{
	int pin, flags;

	zassert_true(DT_SAME_NODE(TEST_I3C_BUS, TEST_I3C), "");
	zassert_true(DT_SAME_NODE(TEST_I2C_BUS, TEST_I2C), "");
	zassert_true(DT_SAME_NODE(TEST_SPI_BUS_0, TEST_SPI), "");
	zassert_true(DT_SAME_NODE(TEST_SPI_BUS_1, TEST_SPI), "");

	zassert_equal(DT_SPI_DEV_HAS_CS_GPIOS(TEST_SPI_DEV_0), 1, "");
	zassert_equal(DT_SPI_DEV_HAS_CS_GPIOS(TEST_SPI_DEV_NO_CS), 0, "");

	/* Test a nested I2C bus using vnd,i2c-mux. */
	zassert_true(DT_SAME_NODE(TEST_I2C_MUX_CTLR_1,
				  DT_BUS(TEST_MUXED_I2C_DEV_1)), "");
	zassert_true(DT_SAME_NODE(TEST_I2C_MUX_CTLR_2,
				  DT_BUS(TEST_MUXED_I2C_DEV_2)), "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_spi_device_2
	/* there is only one instance, and it has no CS */
	zassert_equal(DT_INST_SPI_DEV_HAS_CS_GPIOS(0), 0, "");
	/* since there's only one instance, we also know its bus. */
	zassert_true(DT_SAME_NODE(TEST_SPI_NO_CS, DT_INST_BUS(0)),
		     "expected TEST_SPI_NO_CS as bus for vnd,spi-device-2");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_spi_device
	/*
	 * DT_INST_SPI_DEV: use with care here. We could be matching
	 * either vnd,spi-device.
	 */
	zassert_equal(DT_INST_SPI_DEV_HAS_CS_GPIOS(0), 1, "");

#define CTLR_NODE DT_INST_SPI_DEV_CS_GPIOS_CTLR(0)
	zassert_true(DT_SAME_NODE(CTLR_NODE, DT_NODELABEL(test_gpio_1)) ||
		     DT_SAME_NODE(CTLR_NODE, DT_NODELABEL(test_gpio_2)), "");
#undef CTLR_NODE

	pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	zassert_true((pin == 0x10) || (pin == 0x30), "");

	flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	zassert_true((flags == 0x20) || (flags == 0x40), "");

	zassert_equal(DT_ON_BUS(TEST_SPI_DEV_0, spi), 1, "");
	zassert_equal(DT_ON_BUS(TEST_SPI_DEV_0, i2c), 0, "");
	zassert_equal(DT_ON_BUS(TEST_SPI_DEV_0, i3c), 0, "");

	zassert_equal(DT_ON_BUS(TEST_I2C_DEV, i2c), 1, "");
	zassert_equal(DT_ON_BUS(TEST_I2C_DEV, i3c), 0, "");
	zassert_equal(DT_ON_BUS(TEST_I2C_DEV, spi), 0, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_spi_device
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 2, "");

	zassert_equal(DT_INST_ON_BUS(0, spi), 1, "");
	zassert_equal(DT_INST_ON_BUS(0, i2c), 0, "");
	zassert_equal(DT_INST_ON_BUS(0, i3c), 0, "");

	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(spi), 1, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c), 0, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c), 0, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_i2c_device
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 2, "");

	zassert_equal(DT_INST_ON_BUS(0, i2c), 1, "");
	zassert_equal(DT_INST_ON_BUS(0, i3c), 0, "");
	zassert_equal(DT_INST_ON_BUS(0, spi), 0, "");

	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c), 1, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c), 0, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(spi), 0, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_i3c_device
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	zassert_equal(DT_INST_ON_BUS(0, i2c), 1, "");
	zassert_equal(DT_INST_ON_BUS(0, i3c), 1, "");
	zassert_equal(DT_INST_ON_BUS(0, spi), 0, "");

	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c), 1, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c), 1, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(spi), 0, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_i3c_i2c_device
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	zassert_equal(DT_INST_ON_BUS(0, i2c), 1, "");
	zassert_equal(DT_INST_ON_BUS(0, i3c), 1, "");
	zassert_equal(DT_INST_ON_BUS(0, spi), 0, "");

	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c), 1, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c), 1, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(spi), 0, "");

#undef DT_DRV_COMPAT

	/*
	 * Make sure the underlying DT_HAS_COMPAT_ON_BUS_STATUS_OKAY used by
	 * DT_ANY_INST_ON_BUS works without DT_DRV_COMPAT defined.
	 */
	zassert_equal(DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(vnd_spi_device, spi), 1);
	zassert_equal(DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(vnd_spi_device, i2c), 0);

	zassert_equal(DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(vnd_i2c_device, i2c), 1);
	zassert_equal(DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(vnd_i2c_device, spi), 0);

	zassert_equal(DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(vnd_gpio_expander, i2c), 1,
		      NULL);
	zassert_equal(DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(vnd_gpio_expander, spi), 1,
		      NULL);
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_vendor

#define VND_VENDOR "A stand-in for a real vendor which can be used in examples and tests"
#define ZEP_VENDOR "Zephyr-specific binding"

ZTEST(devicetree_api, test_vendor)
{
	/* DT_NODE_VENDOR_HAS_IDX */
	zassert_true(DT_NODE_VENDOR_HAS_IDX(TEST_VENDOR, 0), "");
	zassert_false(DT_NODE_VENDOR_HAS_IDX(TEST_VENDOR, 1), "");
	zassert_true(DT_NODE_VENDOR_HAS_IDX(TEST_VENDOR, 2), "");
	zassert_false(DT_NODE_VENDOR_HAS_IDX(TEST_VENDOR, 3), "");

	/* DT_NODE_VENDOR_BY_IDX */
	zassert_true(!strcmp(DT_NODE_VENDOR_BY_IDX(TEST_VENDOR, 0), VND_VENDOR), "");
	zassert_true(!strcmp(DT_NODE_VENDOR_BY_IDX(TEST_VENDOR, 2), ZEP_VENDOR), "");

	/* DT_NODE_VENDOR_BY_IDX_OR */
	zassert_true(!strcmp(DT_NODE_VENDOR_BY_IDX_OR(TEST_VENDOR, 0, NULL), VND_VENDOR), "");
	zassert_is_null(DT_NODE_VENDOR_BY_IDX_OR(TEST_VENDOR, 1, NULL), "");
	zassert_true(!strcmp(DT_NODE_VENDOR_BY_IDX_OR(TEST_VENDOR, 2, NULL), ZEP_VENDOR), "");
	zassert_is_null(DT_NODE_VENDOR_BY_IDX_OR(TEST_VENDOR, 3, NULL), "");

	/* DT_NODE_VENDOR_OR */
	zassert_true(!strcmp(DT_NODE_VENDOR_OR(TEST_VENDOR, NULL), VND_VENDOR), "");
}

#define VND_MODEL "model1"
#define ZEP_MODEL "model2"

ZTEST(devicetree_api, test_model)
{
	/* DT_NODE_MODEL_HAS_IDX */
	zassert_true(DT_NODE_MODEL_HAS_IDX(TEST_MODEL, 0), "");
	zassert_false(DT_NODE_MODEL_HAS_IDX(TEST_MODEL, 1), "");
	zassert_true(DT_NODE_MODEL_HAS_IDX(TEST_MODEL, 2), "");
	zassert_false(DT_NODE_MODEL_HAS_IDX(TEST_MODEL, 3), "");

	/* DT_NODE_MODEL_BY_IDX */
	zassert_true(!strcmp(DT_NODE_MODEL_BY_IDX(TEST_MODEL, 0), VND_MODEL), "");
	zassert_true(!strcmp(DT_NODE_MODEL_BY_IDX(TEST_MODEL, 2), ZEP_MODEL), "");

	/* DT_NODE_MODEL_BY_IDX_OR */
	zassert_true(!strcmp(DT_NODE_MODEL_BY_IDX_OR(TEST_MODEL, 0, NULL), VND_MODEL), "");
	zassert_is_null(DT_NODE_MODEL_BY_IDX_OR(TEST_MODEL, 1, NULL), "");
	zassert_true(!strcmp(DT_NODE_MODEL_BY_IDX_OR(TEST_MODEL, 2, NULL), ZEP_MODEL), "");
	zassert_is_null(DT_NODE_MODEL_BY_IDX_OR(TEST_MODEL, 3, NULL), "");

	/* DT_NODE_MODEL_OR */
	zassert_true(!strcmp(DT_NODE_MODEL_OR(TEST_MODEL, NULL), VND_MODEL), "");
}

#undef ZEP_MODEL
#undef VND_MODEL

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_reg_holder
ZTEST(devicetree_api, test_reg)
{
	/* DT_REG_HAS_IDX */
	zassert_true(DT_REG_HAS_IDX(TEST_ABCD1234, 0), "");
	zassert_true(DT_REG_HAS_IDX(TEST_ABCD1234, 1), "");
	zassert_false(DT_REG_HAS_IDX(TEST_ABCD1234, 2), "");

	/* DT_REG_ADDR_BY_IDX */
	zassert_equal(DT_REG_ADDR_BY_IDX(TEST_ABCD1234, 0), 0xabcd1234, "");
	zassert_equal(DT_REG_ADDR_BY_IDX(TEST_ABCD1234, 1), 0x98765432, "");

	/* DT_REG_SIZE_BY_IDX */
	zassert_equal(DT_REG_SIZE_BY_IDX(TEST_ABCD1234, 0), 0x500, "");
	zassert_equal(DT_REG_SIZE_BY_IDX(TEST_ABCD1234, 1), 0xff, "");

	/* DT_REG_ADDR */
	zassert_equal(DT_REG_ADDR(TEST_ABCD1234), 0xabcd1234, "");

	/* DT_REG_ADDR_U64 */
	zassert_equal(DT_REG_ADDR_U64(TEST_ABCD1234), 0xabcd1234, "");

	/* DT_REG_SIZE */
	zassert_equal(DT_REG_SIZE(TEST_ABCD1234), 0x500, "");

	/* DT_REG_ADDR_BY_NAME */
	zassert_equal(DT_REG_ADDR_BY_NAME(TEST_ABCD1234, one), 0xabcd1234, "");
	zassert_equal(DT_REG_ADDR_BY_NAME(TEST_ABCD1234, two), 0x98765432, "");

	/* DT_REG_ADDR_BY_NAME_U64 */
	zassert_equal(DT_REG_ADDR_BY_NAME_U64(TEST_ABCD1234, one), 0xabcd1234, "");
	zassert_equal(DT_REG_ADDR_BY_NAME_U64(TEST_ABCD1234, two), 0x98765432, "");

	/* DT_REG_SIZE_BY_NAME */
	zassert_equal(DT_REG_SIZE_BY_NAME(TEST_ABCD1234, one), 0x500, "");
	zassert_equal(DT_REG_SIZE_BY_NAME(TEST_ABCD1234, two), 0xff, "");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	/* DT_INST_REG_HAS_IDX */
	zassert_true(DT_INST_REG_HAS_IDX(0, 0), "");
	zassert_true(DT_INST_REG_HAS_IDX(0, 1), "");
	zassert_false(DT_INST_REG_HAS_IDX(0, 2), "");

	/* DT_INST_REG_ADDR_BY_IDX */
	zassert_equal(DT_INST_REG_ADDR_BY_IDX(0, 0), 0x9999aaaa, "");
	zassert_equal(DT_INST_REG_ADDR_BY_IDX(0, 1), 0xbbbbcccc, "");

	/* DT_INST_REG_SIZE_BY_IDX */
	zassert_equal(DT_INST_REG_SIZE_BY_IDX(0, 0), 0x1000, "");
	zassert_equal(DT_INST_REG_SIZE_BY_IDX(0, 1), 0x3f, "");

	/* DT_INST_REG_ADDR */
	zassert_equal(DT_INST_REG_ADDR(0), 0x9999aaaa, "");

	/* DT_INST_REG_ADDR_U64 */
	zassert_equal(DT_INST_REG_ADDR_U64(0), 0x9999aaaa, "");

	/* DT_INST_REG_SIZE */
	zassert_equal(DT_INST_REG_SIZE(0), 0x1000, "");

	/* DT_INST_REG_ADDR_BY_NAME */
	zassert_equal(DT_INST_REG_ADDR_BY_NAME(0, first), 0x9999aaaa, "");
	zassert_equal(DT_INST_REG_ADDR_BY_NAME(0, second), 0xbbbbcccc, "");

	/* DT_INST_REG_ADDR_BY_NAME_U64 */
	zassert_equal(DT_INST_REG_ADDR_BY_NAME_U64(0, first), 0x9999aaaa, "");
	zassert_equal(DT_INST_REG_ADDR_BY_NAME_U64(0, second), 0xbbbbcccc, "");

	/* DT_INST_REG_SIZE_BY_NAME */
	zassert_equal(DT_INST_REG_SIZE_BY_NAME(0, first), 0x1000, "");
	zassert_equal(DT_INST_REG_SIZE_BY_NAME(0, second), 0x3f, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_reg_holder_64
ZTEST(devicetree_api, test_reg_64)
{
	/* DT_REG_ADDR_U64 */
	zassert_equal(DT_REG_ADDR_U64(TEST_64BIT), 0xffffffff11223344, "");

	/* DT_REG_ADDR_BY_NAME_U64 */
	zassert_equal(DT_REG_ADDR_BY_NAME_U64(TEST_64BIT, test_name), 0xffffffff11223344, "");

	/* DT_INST_REG_ADDR_U64 */
	zassert_equal(DT_INST_REG_ADDR_U64(0), 0xffffffff11223344, "");

	/* DT_INST_REG_ADDR_BY_NAME_U64 */
	zassert_equal(DT_INST_REG_ADDR_BY_NAME_U64(0, test_name), 0xffffffff11223344, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_interrupt_holder
ZTEST(devicetree_api, test_irq)
{
	/* DT_NUM_IRQS */
	zassert_equal(DT_NUM_IRQS(TEST_DEADBEEF), 1, "");
	zassert_equal(DT_NUM_IRQS(TEST_I2C_BUS), 2, "");
	zassert_equal(DT_NUM_IRQS(TEST_SPI), 3, "");

	/* DT_IRQ_HAS_IDX */
	zassert_true(DT_IRQ_HAS_IDX(TEST_SPI_BUS_0, 0), "");
	zassert_true(DT_IRQ_HAS_IDX(TEST_SPI_BUS_0, 1), "");
	zassert_true(DT_IRQ_HAS_IDX(TEST_SPI_BUS_0, 2), "");
	zassert_false(DT_IRQ_HAS_IDX(TEST_SPI_BUS_0, 3), "");

	zassert_true(DT_IRQ_HAS_IDX(TEST_DEADBEEF, 0), "");
	zassert_false(DT_IRQ_HAS_IDX(TEST_DEADBEEF, 1), "");

	zassert_true(DT_IRQ_HAS_IDX(TEST_I2C_BUS, 0), "");
	zassert_true(DT_IRQ_HAS_IDX(TEST_I2C_BUS, 1), "");
	zassert_false(DT_IRQ_HAS_IDX(TEST_I2C_BUS, 2), "");

	/* DT_IRQ_BY_IDX */
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 0, irq), 8, "");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 1, irq), 9, "");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 2, irq), 10, "");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 0, priority), 3, "");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 1, priority), 0, "");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 2, priority), 1, "");

	/* DT_IRQ_BY_NAME */
	zassert_equal(DT_IRQ_BY_NAME(TEST_I2C_BUS, status, irq), 6, "");
	zassert_equal(DT_IRQ_BY_NAME(TEST_I2C_BUS, error, irq), 7, "");
	zassert_equal(DT_IRQ_BY_NAME(TEST_I2C_BUS, status, priority), 2, "");
	zassert_equal(DT_IRQ_BY_NAME(TEST_I2C_BUS, error, priority), 1, "");

	/* DT_IRQ_HAS_CELL_AT_IDX */
	zassert_true(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 0, irq), "");
	zassert_true(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 0, priority), "");
	zassert_false(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 0, foo), 0, "");
	zassert_true(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 2, irq), "");
	zassert_true(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 2, priority), "");
	zassert_false(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 2, foo), "");

	/* DT_IRQ_HAS_CELL */
	zassert_true(DT_IRQ_HAS_CELL(TEST_IRQ, irq), "");
	zassert_true(DT_IRQ_HAS_CELL(TEST_IRQ, priority), "");
	zassert_false(DT_IRQ_HAS_CELL(TEST_IRQ, foo), "");

	/* DT_IRQ_HAS_NAME */
	zassert_true(DT_IRQ_HAS_NAME(TEST_IRQ, err), "");
	zassert_true(DT_IRQ_HAS_NAME(TEST_IRQ, stat), "");
	zassert_true(DT_IRQ_HAS_NAME(TEST_IRQ, done), "");
	zassert_false(DT_IRQ_HAS_NAME(TEST_IRQ, alpha), "");

	/* DT_IRQ */
	zassert_equal(DT_IRQ(TEST_I2C_BUS, irq), 6, "");
	zassert_equal(DT_IRQ(TEST_I2C_BUS, priority), 2, "");

	/* DT_IRQN */
#ifndef CONFIG_MULTI_LEVEL_INTERRUPTS
	zassert_equal(DT_IRQN(TEST_I2C_BUS), 6, "");
	zassert_equal(DT_IRQN(DT_INST(0, DT_DRV_COMPAT)), 30, "");
#else
	zassert_equal(DT_IRQN(TEST_I2C_BUS),
			((6 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
	zassert_equal(DT_IRQN(DT_INST(0, DT_DRV_COMPAT)),
			((30 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
#endif

	/* DT_IRQN_BY_IDX */
#ifndef CONFIG_MULTI_LEVEL_INTERRUPTS
	zassert_equal(DT_IRQN_BY_IDX(DT_INST(0, DT_DRV_COMPAT), 0), 30, "");
	zassert_equal(DT_IRQN_BY_IDX(DT_INST(0, DT_DRV_COMPAT), 1), 40, "");
	zassert_equal(DT_IRQN_BY_IDX(DT_INST(0, DT_DRV_COMPAT), 2), 60, "");
#else
	zassert_equal(DT_IRQN_BY_IDX(DT_INST(0, DT_DRV_COMPAT), 0),
		      ((30 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
	zassert_equal(DT_IRQN_BY_IDX(DT_INST(0, DT_DRV_COMPAT), 1),
		      ((40 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
	zassert_equal(DT_IRQN_BY_IDX(DT_INST(0, DT_DRV_COMPAT), 2),
		      ((60 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
#endif

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	/* DT_INST_IRQ_HAS_IDX */
	zassert_equal(DT_INST_IRQ_HAS_IDX(0, 0), 1, "");
	zassert_equal(DT_INST_IRQ_HAS_IDX(0, 1), 1, "");
	zassert_equal(DT_INST_IRQ_HAS_IDX(0, 2), 1, "");
	zassert_equal(DT_INST_IRQ_HAS_IDX(0, 3), 0, "");

	/* DT_INST_IRQ_BY_IDX */
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 0, irq), 30, "");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 1, irq), 40, "");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 2, irq), 60, "");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 0, priority), 3, "");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 1, priority), 5, "");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 2, priority), 7, "");

	/* DT_INST_IRQ_BY_NAME */
	zassert_equal(DT_INST_IRQ_BY_NAME(0, err, irq), 30, "");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, stat, irq), 40, "");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, done, irq), 60, "");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, err, priority), 3, "");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, stat, priority), 5, "");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, done, priority), 7, "");

	/* DT_INST_IRQ */
	zassert_equal(DT_INST_IRQ(0, irq), 30, "");
	zassert_equal(DT_INST_IRQ(0, priority), 3, "");

	/* DT_INST_IRQN */
#ifndef CONFIG_MULTI_LEVEL_INTERRUPTS
	zassert_equal(DT_INST_IRQN(0), 30, "");
#else
	zassert_equal(DT_INST_IRQN(0), ((30 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
#endif

	/* DT_INST_IRQN_BY_IDX */
#ifndef CONFIG_MULTI_LEVEL_INTERRUPTS
	zassert_equal(DT_INST_IRQN_BY_IDX(0, 0), 30, "");
	zassert_equal(DT_INST_IRQN_BY_IDX(0, 1), 40, "");
	zassert_equal(DT_INST_IRQN_BY_IDX(0, 2), 60, "");
#else
	zassert_equal(DT_INST_IRQN_BY_IDX(0, 0),
		      ((30 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
	zassert_equal(DT_INST_IRQN_BY_IDX(0, 1),
		      ((40 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
	zassert_equal(DT_INST_IRQN_BY_IDX(0, 2),
		      ((60 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
#endif

	/* DT_INST_IRQ_HAS_CELL_AT_IDX */
	zassert_true(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 0, irq), "");
	zassert_true(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 0, priority), "");
	zassert_false(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 0, foo), "");
	zassert_true(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 2, irq), "");
	zassert_true(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 2, priority), "");
	zassert_false(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 2, foo), "");

	/* DT_INST_IRQ_HAS_CELL */
	zassert_true(DT_INST_IRQ_HAS_CELL(0, irq), "");
	zassert_true(DT_INST_IRQ_HAS_CELL(0, priority), "");
	zassert_false(DT_INST_IRQ_HAS_CELL(0, foo), "");

	/* DT_INST_IRQ_HAS_NAME */
	zassert_true(DT_INST_IRQ_HAS_NAME(0, err), "");
	zassert_true(DT_INST_IRQ_HAS_NAME(0, stat), "");
	zassert_true(DT_INST_IRQ_HAS_NAME(0, done), "");
	zassert_false(DT_INST_IRQ_HAS_NAME(0, alpha), "");

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	/* the following asserts check if interrupt IDs are encoded
	 * properly when dealing with a node that consumes interrupts
	 * from L2 aggregators extending different L1 interrupts.
	 */
	zassert_equal(DT_IRQN_BY_IDX(TEST_IRQ_EXT, 0),
		      ((70 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11, "");
	zassert_equal(DT_IRQN_BY_IDX(TEST_IRQ_EXT, 2),
		      ((42 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 12, "");
#else
	zassert_equal(DT_IRQN_BY_IDX(TEST_IRQ_EXT, 0), 70, "");
	zassert_equal(DT_IRQN_BY_IDX(TEST_IRQ_EXT, 2), 42, "");
#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
}

ZTEST(devicetree_api, test_irq_level)
{
	/* DT_IRQ_LEVEL */
	zassert_equal(DT_IRQ_LEVEL(TEST_TEMP), 0, "");
	zassert_equal(DT_IRQ_LEVEL(TEST_INTC), 1, "");
	zassert_equal(DT_IRQ_LEVEL(TEST_SPI), 2, "");

	/* DT_IRQ_LEVEL */
	#undef DT_DRV_COMPAT
	#define DT_DRV_COMPAT vnd_adc_temp_sensor
	zassert_equal(DT_INST_IRQ_LEVEL(0), 0, "");

	#undef DT_DRV_COMPAT
	#define DT_DRV_COMPAT vnd_intc
	zassert_equal(DT_INST_IRQ_LEVEL(1), 1, "");

	#undef DT_DRV_COMPAT
	#define DT_DRV_COMPAT vnd_spi
	zassert_equal(DT_INST_IRQ_LEVEL(0), 2, "");
}

struct gpios_struct {
	gpio_pin_t pin;
	gpio_flags_t flags;
};

#define CLOCK_FREQUENCY_AND_COMMA(node_id, prop, idx) \
	DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, clock_frequency),

/* Helper macro that UTIL_LISTIFY can use and produces an element with comma */
#define DT_GPIO_ELEM(idx, node_id, prop) \
	{ \
		DT_PHA_BY_IDX(node_id, prop, idx, pin),\
		DT_PHA_BY_IDX(node_id, prop, idx, flags),\
	}
#define DT_GPIO_LISTIFY(node_id, prop) \
	{ LISTIFY(DT_PROP_LEN(node_id, prop), DT_GPIO_ELEM, (,), \
		  node_id, prop) }

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_phandle_holder
ZTEST(devicetree_api, test_phandles)
{
	bool gpio_controller = DT_PROP_BY_PHANDLE(TEST_PH, ph, gpio_controller);
	size_t phs_freqs[] = { DT_FOREACH_PROP_ELEM(TEST_PH, phs, CLOCK_FREQUENCY_AND_COMMA) };
	struct gpios_struct gps[] = DT_GPIO_LISTIFY(TEST_PH, gpios);

	/* phandle */
	zassert_true(DT_NODE_HAS_PROP(TEST_PH, ph), "");
	zassert_true(DT_SAME_NODE(DT_PROP(TEST_PH, ph),
				  DT_NODELABEL(test_gpio_1)), "");
	zassert_equal(DT_PROP_LEN(TEST_PH, ph), 1, "");
	zassert_true(DT_SAME_NODE(DT_PROP_BY_IDX(TEST_PH, ph, 0),
				  DT_NODELABEL(test_gpio_1)), "");
	/* DT_PROP_BY_PHANDLE */
	zassert_equal(gpio_controller, true, "");

	/* phandles */
	zassert_true(DT_NODE_HAS_PROP(TEST_PH, phs), "");
	zassert_equal(ARRAY_SIZE(phs_freqs), 2, "");
	zassert_equal(DT_PROP_LEN(TEST_PH, phs), 2, "");
	zassert_true(DT_SAME_NODE(DT_PROP_BY_IDX(TEST_PH, phs, 1),
				  TEST_SPI), "");

	/* DT_FOREACH_PROP_ELEM on a phandles type property */
	zassert_equal(phs_freqs[0], 100000, "");
	zassert_equal(phs_freqs[1], 2000000, "");

	/* DT_PROP_BY_PHANDLE_IDX on a phandles type property */
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_PH, phs, 0, clock_frequency),
		      100000, "");
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_PH, phs, 1, clock_frequency),
		      2000000, "");

	/* DT_PROP_BY_PHANDLE_IDX on a phandle-array type property */
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 0,
					     ngpios), 100, "");
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 1,
					     ngpios), 200, "");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 0, status),
			     "okay"), "");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 1, status),
			     "okay"), "");

	/* DT_PROP_BY_PHANDLE_IDX_OR */
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX_OR(TEST_PH, phs_or, 0,
						val, "zero"), "one"), "");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX_OR(TEST_PH, phs_or, 1,
						val, "zero"), "zero"), "");

	/* phandle-array */
	zassert_true(DT_NODE_HAS_PROP(TEST_PH, gpios), "");
	zassert_equal(ARRAY_SIZE(gps), 2, "");
	zassert_equal(DT_PROP_LEN(TEST_PH, gpios), 2, "");

	/* DT_PROP_HAS_IDX */
	zassert_true(DT_PROP_HAS_IDX(TEST_PH, gpios, 0), "");
	zassert_true(DT_PROP_HAS_IDX(TEST_PH, gpios, 1), "");
	zassert_false(DT_PROP_HAS_IDX(TEST_PH, gpios, 2), "");

	/* DT_PROP_HAS_NAME */
	zassert_false(DT_PROP_HAS_NAME(TEST_PH, foos, A), "");
	zassert_true(DT_PROP_HAS_NAME(TEST_PH, foos, a), "");
	zassert_false(DT_PROP_HAS_NAME(TEST_PH, foos, b-c), "");
	zassert_true(DT_PROP_HAS_NAME(TEST_PH, foos, b_c), "");
	zassert_false(DT_PROP_HAS_NAME(TEST_PH, bazs, jane), "");

	/* DT_PHA_HAS_CELL_AT_IDX */
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, gpios, 1, pin), "");
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, gpios, 1, flags), "");
	/* pha-gpios index 1 has nothing, not even a phandle */
	zassert_false(DT_PROP_HAS_IDX(TEST_PH, pha_gpios, 1), "");
	zassert_false(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 1, pin), "");
	zassert_false(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 1, flags),
		      "");
	/* index 2 only has a pin cell, no flags */
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 2, pin), "");
	zassert_false(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 2, flags),
		      "");
	/* index 3 has both pin and flags cells*/
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 3, pin), "");
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 3, flags), "");
	/* even though index 1 has nothing, the length is still 4 */
	zassert_equal(DT_PROP_LEN(TEST_PH, pha_gpios), 4, "");

	/* DT_PHA_HAS_CELL */
	zassert_true(DT_PHA_HAS_CELL(TEST_PH, gpios, flags), "");
	zassert_false(DT_PHA_HAS_CELL(TEST_PH, gpios, bar), "");

	/* DT_PHANDLE_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_PHANDLE_BY_IDX(TEST_PH, gpios, 0), TEST_GPIO_1), "");
	zassert_true(DT_SAME_NODE(DT_PHANDLE_BY_IDX(TEST_PH, gpios, 1), TEST_GPIO_2), "");

	/* DT_PHANDLE */
	zassert_true(DT_SAME_NODE(DT_PHANDLE(TEST_PH, gpios), TEST_GPIO_1), "");

	/* DT_PHA */
	zassert_equal(DT_PHA(TEST_PH, gpios, pin), 10, "");
	zassert_equal(DT_PHA(TEST_PH, gpios, flags), 20, "");

	/* DT_PHA_BY_IDX */
	zassert_equal(DT_PHA_BY_IDX(TEST_PH, gpios, 0, pin), 10, "");
	zassert_equal(DT_PHA_BY_IDX(TEST_PH, gpios, 0, flags), 20, "");

	zassert_equal(DT_PHA_BY_IDX(TEST_PH, gpios, 1, pin), 30, "");
	zassert_equal(DT_PHA_BY_IDX(TEST_PH, gpios, 1, flags), 40, "");

	/* DT_PHA_BY_NAME */
	zassert_equal(DT_PHA_BY_NAME(TEST_PH, foos, a, foocell), 100, "");
	zassert_equal(DT_PHA_BY_NAME(TEST_PH, foos, b_c, foocell), 110, "");

	/* DT_PHANDLE_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_PHANDLE_BY_NAME(TEST_PH, foos, a), TEST_GPIO_1), "");
	zassert_true(DT_SAME_NODE(DT_PHANDLE_BY_NAME(TEST_PH, foos, b_c), TEST_GPIO_2), "");

	/* array initializers */
	zassert_equal(gps[0].pin, 10, "");
	zassert_equal(gps[0].flags, 20, "");

	zassert_equal(gps[1].pin, 30, "");
	zassert_equal(gps[1].flags, 40, "");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	/* DT_INST_PROP_BY_PHANDLE */
	zassert_equal(DT_INST_PROP_BY_PHANDLE(0, ph, ngpios), 100, "");

	/* DT_INST_PROP_BY_PHANDLE_IDX */
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, phs, 0, clock_frequency),
		      100000, "");
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, phs, 1, clock_frequency),
		      2000000, "");
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 0,
					     gpio_controller),
		      1, "");
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 1,
					     gpio_controller),
		      1, "");
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 0, ngpios),
		      100, "");
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 1, ngpios),
		      200, "");

	/* DT_INST_PROP_HAS_IDX */
	zassert_true(DT_INST_PROP_HAS_IDX(0, gpios, 0), "");
	zassert_true(DT_INST_PROP_HAS_IDX(0, gpios, 1), "");
	zassert_false(DT_INST_PROP_HAS_IDX(0, gpios, 2), "");

	/* DT_INST_PROP_HAS_NAME */
	zassert_false(DT_INST_PROP_HAS_NAME(0, foos, A), "");
	zassert_true(DT_INST_PROP_HAS_NAME(0, foos, a), "");
	zassert_false(DT_INST_PROP_HAS_NAME(0, foos, b-c), "");
	zassert_true(DT_INST_PROP_HAS_NAME(0, foos, b_c), "");
	zassert_false(DT_INST_PROP_HAS_NAME(0, bazs, jane), "");

	/* DT_INST_PHA_HAS_CELL_AT_IDX */
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, gpios, 1, pin), "");
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, gpios, 1, flags), "");
	/* index 1 has nothing, not even a phandle */
	zassert_false(DT_INST_PROP_HAS_IDX(0, pha_gpios, 1), "");
	zassert_false(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 1, pin), "");
	zassert_false(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 1, flags), "");
	/* index 2 only has pin, no flags */
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 2, pin), "");
	zassert_false(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 2, flags), "");
	/* index 3 has both pin and flags */
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 3, pin), "");
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 3, flags), "");
	/* even though index 1 has nothing, the length is still 4 */
	zassert_equal(DT_INST_PROP_LEN(0, pha_gpios), 4, "");

	/* DT_INST_PHA_HAS_CELL */
	zassert_true(DT_INST_PHA_HAS_CELL(0, gpios, flags), "");
	zassert_false(DT_INST_PHA_HAS_CELL(0, gpios, bar), "");

	/* DT_INST_PHANDLE_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_INST_PHANDLE_BY_IDX(0, gpios, 0), TEST_GPIO_1), "");
	zassert_true(DT_SAME_NODE(DT_INST_PHANDLE_BY_IDX(0, gpios, 1), TEST_GPIO_2), "");

	/* DT_INST_PHANDLE */
	zassert_true(DT_SAME_NODE(DT_INST_PHANDLE(0, gpios), TEST_GPIO_1), "");

	/* DT_INST_PHA */
	zassert_equal(DT_INST_PHA(0, gpios, pin), 10, "");
	zassert_equal(DT_INST_PHA(0, gpios, flags), 20, "");

	/* DT_INST_PHA_BY_IDX */
	zassert_equal(DT_INST_PHA_BY_IDX(0, gpios, 0, pin), 10, "");
	zassert_equal(DT_INST_PHA_BY_IDX(0, gpios, 0, flags), 20, "");

	zassert_equal(DT_INST_PHA_BY_IDX(0, gpios, 1, pin), 30, "");
	zassert_equal(DT_INST_PHA_BY_IDX(0, gpios, 1, flags), 40, "");

	/* DT_INST_PHA_BY_NAME */
	zassert_equal(DT_INST_PHA_BY_NAME(0, foos, a, foocell), 100, "");
	zassert_equal(DT_INST_PHA_BY_NAME(0, foos, b_c, foocell), 110, "");

	/* DT_INST_PHANDLE_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_INST_PHANDLE_BY_NAME(0, foos, a), TEST_GPIO_1), "");
	zassert_true(DT_SAME_NODE(DT_INST_PHANDLE_BY_NAME(0, foos, b_c), TEST_GPIO_2), "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_phandle_holder
ZTEST(devicetree_api, test_gpio)
{
	/* DT_GPIO_CTLR_BY_IDX */
	zassert_true(!strcmp(TO_STRING(DT_GPIO_CTLR_BY_IDX(TEST_PH, gpios, 0)),
			     TO_STRING(DT_NODELABEL(test_gpio_1))), "");
	zassert_true(!strcmp(TO_STRING(DT_GPIO_CTLR_BY_IDX(TEST_PH, gpios, 1)),
			     TO_STRING(DT_NODELABEL(test_gpio_2))), "");

	/* DT_GPIO_CTLR */
	zassert_true(!strcmp(TO_STRING(DT_GPIO_CTLR(TEST_PH, gpios)),
			     TO_STRING(DT_NODELABEL(test_gpio_1))), "");

	/* DT_GPIO_PIN_BY_IDX */
	zassert_equal(DT_GPIO_PIN_BY_IDX(TEST_PH, gpios, 0), 10, "");
	zassert_equal(DT_GPIO_PIN_BY_IDX(TEST_PH, gpios, 1), 30, "");

	/* DT_GPIO_PIN */
	zassert_equal(DT_GPIO_PIN(TEST_PH, gpios), 10, "");

	/* DT_GPIO_FLAGS_BY_IDX */
	zassert_equal(DT_GPIO_FLAGS_BY_IDX(TEST_PH, gpios, 0), 20, "");
	zassert_equal(DT_GPIO_FLAGS_BY_IDX(TEST_PH, gpios, 1), 40, "");

	/* DT_GPIO_FLAGS */
	zassert_equal(DT_GPIO_FLAGS(TEST_PH, gpios), 20, "");

	/* DT_NUM_GPIO_HOGS */
	zassert_equal(DT_NUM_GPIO_HOGS(TEST_GPIO_HOG_1), 2, "");
	zassert_equal(DT_NUM_GPIO_HOGS(TEST_GPIO_HOG_2), 1, "");
	zassert_equal(DT_NUM_GPIO_HOGS(TEST_GPIO_HOG_3), 1, "");

	/* DT_GPIO_HOG_PIN_BY_IDX */
	zassert_equal(DT_GPIO_HOG_PIN_BY_IDX(TEST_GPIO_HOG_1, 0), 0, "");
	zassert_equal(DT_GPIO_HOG_PIN_BY_IDX(TEST_GPIO_HOG_1, 1), 1, "");
	zassert_equal(DT_GPIO_HOG_PIN_BY_IDX(TEST_GPIO_HOG_2, 0), 3, "");
	zassert_equal(DT_GPIO_HOG_PIN_BY_IDX(TEST_GPIO_HOG_3, 0), 4, "");

	/* DT_GPIO_HOG_FLAGS_BY_IDX */
	zassert_equal(DT_GPIO_HOG_FLAGS_BY_IDX(TEST_GPIO_HOG_1, 0), 0x00, "");
	zassert_equal(DT_GPIO_HOG_FLAGS_BY_IDX(TEST_GPIO_HOG_1, 1), 0x10, "");
	zassert_equal(DT_GPIO_HOG_FLAGS_BY_IDX(TEST_GPIO_HOG_2, 0), 0x20, "");
	zassert_equal(DT_GPIO_HOG_FLAGS_BY_IDX(TEST_GPIO_HOG_3, 0), 0x30, "");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	/* DT_INST_GPIO_PIN_BY_IDX */
	zassert_equal(DT_INST_GPIO_PIN_BY_IDX(0, gpios, 0), 10, "");
	zassert_equal(DT_INST_GPIO_PIN_BY_IDX(0, gpios, 1), 30, "");

	/* DT_INST_GPIO_PIN */
	zassert_equal(DT_INST_GPIO_PIN(0, gpios), 10, "");

	/* DT_INST_GPIO_FLAGS_BY_IDX */
	zassert_equal(DT_INST_GPIO_FLAGS_BY_IDX(0, gpios, 0), 20, "");
	zassert_equal(DT_INST_GPIO_FLAGS_BY_IDX(0, gpios, 1), 40, "");

	/* DT_INST_GPIO_FLAGS */
	zassert_equal(DT_INST_GPIO_FLAGS(0, gpios), 20, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
ZTEST(devicetree_api, test_io_channels)
{
	/* DT_IO_CHANNELS_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_IO_CHANNELS_CTLR_BY_IDX(TEST_TEMP, 0),
				  TEST_IO_CHANNEL_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_IO_CHANNELS_CTLR_BY_IDX(TEST_TEMP, 1),
				  TEST_IO_CHANNEL_CTLR_2), "");

	/* DT_IO_CHANNELS_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_IO_CHANNELS_CTLR_BY_NAME(TEST_TEMP, ch1),
				  TEST_IO_CHANNEL_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_IO_CHANNELS_CTLR_BY_NAME(TEST_TEMP, ch2),
				  TEST_IO_CHANNEL_CTLR_2), "");

	/* DT_IO_CHANNELS_CTLR */
	zassert_true(DT_SAME_NODE(DT_IO_CHANNELS_CTLR(TEST_TEMP),
				  TEST_IO_CHANNEL_CTLR_1), "");

	/* DT_INST_IO_CHANNELS_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_INST_IO_CHANNELS_CTLR_BY_IDX(0, 0),
				  TEST_IO_CHANNEL_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_INST_IO_CHANNELS_CTLR_BY_IDX(0, 1),
				  TEST_IO_CHANNEL_CTLR_2), "");

	/* DT_INST_IO_CHANNELS_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_INST_IO_CHANNELS_CTLR_BY_NAME(0, ch1),
				  TEST_IO_CHANNEL_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_INST_IO_CHANNELS_CTLR_BY_NAME(0, ch2),
				  TEST_IO_CHANNEL_CTLR_2), "");

	/* DT_INST_IO_CHANNELS_CTLR */
	zassert_true(DT_SAME_NODE(DT_INST_IO_CHANNELS_CTLR(0),
				  TEST_IO_CHANNEL_CTLR_1), "");

	zassert_equal(DT_IO_CHANNELS_INPUT_BY_IDX(TEST_TEMP, 0), 10, "");
	zassert_equal(DT_IO_CHANNELS_INPUT_BY_IDX(TEST_TEMP, 1), 20, "");
	zassert_equal(DT_IO_CHANNELS_INPUT_BY_NAME(TEST_TEMP, ch1), 10, "");
	zassert_equal(DT_IO_CHANNELS_INPUT_BY_NAME(TEST_TEMP, ch2), 20, "");
	zassert_equal(DT_IO_CHANNELS_INPUT(TEST_TEMP), 10, "");

	zassert_equal(DT_INST_IO_CHANNELS_INPUT_BY_IDX(0, 0), 10, "");
	zassert_equal(DT_INST_IO_CHANNELS_INPUT_BY_IDX(0, 1), 20, "");
	zassert_equal(DT_INST_IO_CHANNELS_INPUT_BY_NAME(0, ch1), 10, "");
	zassert_equal(DT_INST_IO_CHANNELS_INPUT_BY_NAME(0, ch2), 20, "");
	zassert_equal(DT_INST_IO_CHANNELS_INPUT(0), 10, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
ZTEST(devicetree_api, test_io_channel_names)
{
	struct adc_dt_spec adc_spec;

	/* ADC_DT_SPEC_GET_BY_NAME */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_NAME(TEST_TEMP, ch1);
	zassert_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_GET_BY_NAME(TEST_TEMP, ch2);
	zassert_equal(adc_spec.channel_id, 20, "");

	/* ADC_DT_SPEC_INST_GET_BY_NAME */
	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME(0, ch1);
	zassert_equal(adc_spec.channel_id, 10, "");

	adc_spec = (struct adc_dt_spec)ADC_DT_SPEC_INST_GET_BY_NAME(0, ch2);
	zassert_equal(adc_spec.channel_id, 20, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
ZTEST(devicetree_api, test_dma)
{
	/* DT_DMAS_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_DMAS_CTLR_BY_IDX(TEST_TEMP, 0),
				  TEST_DMA_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_DMAS_CTLR_BY_IDX(TEST_TEMP, 1),
				  TEST_DMA_CTLR_2), "");

	/* DT_DMAS_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_DMAS_CTLR_BY_NAME(TEST_TEMP, tx),
				  TEST_DMA_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_DMAS_CTLR_BY_NAME(TEST_TEMP, rx),
				  TEST_DMA_CTLR_2), "");

	/* DT_DMAS_CTLR */
	zassert_true(DT_SAME_NODE(DT_DMAS_CTLR(TEST_TEMP),
				  TEST_DMA_CTLR_1), "");

	/* DT_INST_DMAS_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_INST_DMAS_CTLR_BY_IDX(0, 0),
				  TEST_DMA_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_INST_DMAS_CTLR_BY_IDX(0, 1),
				  TEST_DMA_CTLR_2), "");

	/* DT_INST_DMAS_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_INST_DMAS_CTLR_BY_NAME(0, tx),
				  TEST_DMA_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_INST_DMAS_CTLR_BY_NAME(0, rx),
				  TEST_DMA_CTLR_2), "");

	/* DT_INST_DMAS_CTLR */
	zassert_true(DT_SAME_NODE(DT_INST_DMAS_CTLR(0), TEST_DMA_CTLR_1), "");

	zassert_equal(DT_DMAS_CELL_BY_NAME(TEST_TEMP, rx, channel), 3, "");
	zassert_equal(DT_INST_DMAS_CELL_BY_NAME(0, rx, channel), 3, "");
	zassert_equal(DT_DMAS_CELL_BY_NAME(TEST_TEMP, rx, slot), 4, "");
	zassert_equal(DT_INST_DMAS_CELL_BY_NAME(0, rx, slot), 4, "");

	zassert_equal(DT_DMAS_CELL_BY_IDX(TEST_TEMP, 1, channel), 3, "");
	zassert_equal(DT_INST_DMAS_CELL_BY_IDX(0, 1, channel), 3, "");
	zassert_equal(DT_DMAS_CELL_BY_IDX(TEST_TEMP, 1, slot), 4, "");
	zassert_equal(DT_INST_DMAS_CELL_BY_IDX(0, 1, slot), 4, "");

	zassert_true(DT_DMAS_HAS_NAME(TEST_TEMP, tx), "");
	zassert_true(DT_INST_DMAS_HAS_NAME(0, tx), "");
	zassert_false(DT_DMAS_HAS_NAME(TEST_TEMP, output), "");
	zassert_false(DT_INST_DMAS_HAS_NAME(0, output), "");

	zassert_true(DT_DMAS_HAS_IDX(TEST_TEMP, 1), "");
	zassert_true(DT_INST_DMAS_HAS_IDX(0, 1), "");
	zassert_false(DT_DMAS_HAS_IDX(TEST_TEMP, 2), "");
	zassert_false(DT_INST_DMAS_HAS_IDX(0, 2), "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_phandle_holder
ZTEST(devicetree_api, test_pwms)
{
	/* DT_PWMS_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(TEST_PH, 0),
				  TEST_PWM_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_PWMS_CTLR_BY_IDX(TEST_PH, 1),
				  TEST_PWM_CTLR_2), "");

	/* DT_PWMS_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_PWMS_CTLR_BY_NAME(TEST_PH, red),
				  TEST_PWM_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_PWMS_CTLR_BY_NAME(TEST_PH, green),
				  TEST_PWM_CTLR_2), "");

	/* DT_PWMS_CTLR */
	zassert_true(DT_SAME_NODE(DT_PWMS_CTLR(TEST_PH),
				  TEST_PWM_CTLR_1), "");

	/* DT_PWMS_CELL_BY_IDX */
	zassert_equal(DT_PWMS_CELL_BY_IDX(TEST_PH, 1, channel), 5, "");
	zassert_equal(DT_PWMS_CELL_BY_IDX(TEST_PH, 1, period), 100, "");
	zassert_equal(DT_PWMS_CELL_BY_IDX(TEST_PH, 1, flags), 1, "");

	/* DT_PWMS_CELL_BY_NAME */
	zassert_equal(DT_PWMS_CELL_BY_NAME(TEST_PH, red, channel), 8, "");
	zassert_equal(DT_PWMS_CELL_BY_NAME(TEST_PH, red, period), 200, "");
	zassert_equal(DT_PWMS_CELL_BY_NAME(TEST_PH, red, flags), 3, "");

	/* DT_PWMS_CELL */
	zassert_equal(DT_PWMS_CELL(TEST_PH, channel), 8, "");
	zassert_equal(DT_PWMS_CELL(TEST_PH, period), 200, "");
	zassert_equal(DT_PWMS_CELL(TEST_PH, flags), 3, "");

	/* DT_PWMS_CHANNEL_BY_IDX */
	zassert_equal(DT_PWMS_CHANNEL_BY_IDX(TEST_PH, 1), 5, "");

	/* DT_PWMS_CHANNEL_BY_NAME */
	zassert_equal(DT_PWMS_CHANNEL_BY_NAME(TEST_PH, green), 5, "");

	/* DT_PWMS_CHANNEL */
	zassert_equal(DT_PWMS_CHANNEL(TEST_PH), 8, "");

	/* DT_PWMS_PERIOD_BY_IDX */
	zassert_equal(DT_PWMS_PERIOD_BY_IDX(TEST_PH, 1), 100, "");

	/* DT_PWMS_PERIOD_BY_NAME */
	zassert_equal(DT_PWMS_PERIOD_BY_NAME(TEST_PH, green), 100, "");

	/* DT_PWMS_PERIOD */
	zassert_equal(DT_PWMS_PERIOD(TEST_PH), 200, "");

	/* DT_PWMS_FLAGS_BY_IDX */
	zassert_equal(DT_PWMS_FLAGS_BY_IDX(TEST_PH, 1), 1, "");

	/* DT_PWMS_FLAGS_BY_NAME */
	zassert_equal(DT_PWMS_FLAGS_BY_NAME(TEST_PH, green), 1, "");

	/* DT_PWMS_FLAGS */
	zassert_equal(DT_PWMS_FLAGS(TEST_PH), 3, "");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	/* DT_INST_PWMS_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_INST_PWMS_CTLR_BY_IDX(0, 0),
				  TEST_PWM_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_INST_PWMS_CTLR_BY_IDX(0, 1),
				  TEST_PWM_CTLR_2), "");

	/* DT_INST_PWMS_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_INST_PWMS_CTLR_BY_NAME(0, red),
				  TEST_PWM_CTLR_1), "");
	zassert_true(DT_SAME_NODE(DT_INST_PWMS_CTLR_BY_NAME(0, green),
				  TEST_PWM_CTLR_2), "");

	/* DT_INST_PWMS_CTLR */
	zassert_true(DT_SAME_NODE(DT_INST_PWMS_CTLR(0), TEST_PWM_CTLR_1), "");

	/* DT_INST_PWMS_CELL_BY_IDX */
	zassert_equal(DT_INST_PWMS_CELL_BY_IDX(0, 1, channel), 5, "");
	zassert_equal(DT_INST_PWMS_CELL_BY_IDX(0, 1, period), 100, "");
	zassert_equal(DT_INST_PWMS_CELL_BY_IDX(0, 1, flags), 1, "");

	/* DT_INST_PWMS_CELL_BY_NAME */
	zassert_equal(DT_INST_PWMS_CELL_BY_NAME(0, green, channel), 5, "");
	zassert_equal(DT_INST_PWMS_CELL_BY_NAME(0, green, period), 100, "");
	zassert_equal(DT_INST_PWMS_CELL_BY_NAME(0, green, flags), 1, "");

	/* DT_INST_PWMS_CELL */
	zassert_equal(DT_INST_PWMS_CELL(0, channel), 8, "");
	zassert_equal(DT_INST_PWMS_CELL(0, period), 200, "");
	zassert_equal(DT_INST_PWMS_CELL(0, flags), 3, "");

	/* DT_INST_PWMS_CHANNEL_BY_IDX */
	zassert_equal(DT_INST_PWMS_CHANNEL_BY_IDX(0, 1), 5, "");

	/* DT_INST_PWMS_CHANNEL_BY_NAME */
	zassert_equal(DT_INST_PWMS_CHANNEL_BY_NAME(0, green), 5, "");

	/* DT_INST_PWMS_CHANNEL */
	zassert_equal(DT_INST_PWMS_CHANNEL(0), 8, "");

	/* DT_INST_PWMS_PERIOD_BY_IDX */
	zassert_equal(DT_INST_PWMS_PERIOD_BY_IDX(0, 1), 100, "");

	/* DT_INST_PWMS_PERIOD_BY_NAME */
	zassert_equal(DT_INST_PWMS_PERIOD_BY_NAME(0, red), 200, "");

	/* DT_INST_PWMS_PERIOD */
	zassert_equal(DT_INST_PWMS_PERIOD(0), 200, "");

	/* DT_INST_PWMS_FLAGS_BY_IDX */
	zassert_equal(DT_INST_PWMS_FLAGS_BY_IDX(0, 1), 1, "");

	/* DT_INST_PWMS_FLAGS_BY_NAME */
	zassert_equal(DT_INST_PWMS_FLAGS_BY_NAME(0, red), 3, "");

	/* DT_INST_PWMS_FLAGS */
	zassert_equal(DT_INST_PWMS_FLAGS(0), 3, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_can_controller
ZTEST(devicetree_api, test_can)
{
	/* DT_CAN_TRANSCEIVER_MIN_BITRATE */
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_0, 0), 10000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_0, 10000), 10000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_0, 20000), 20000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_1, 0), 50000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_1, 50000), 50000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_1, 100000), 100000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_2, 0), 0, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_2, 10000), 10000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_2, 20000), 20000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_3, 0), 0, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_3, 30000), 30000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MIN_BITRATE(TEST_CAN_CTRL_3, 40000), 40000, "");

	/* DT_INST_CAN_TRANSCEIVER_MIN_BITRATE */
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(0, 0), 10000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(0, 10000), 10000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(0, 20000), 20000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(1, 0), 50000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(1, 50000), 50000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(1, 100000), 100000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(2, 0), 0, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(2, 10000), 10000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(2, 20000), 20000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(3, 0), 0, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(3, 30000), 30000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MIN_BITRATE(3, 40000), 40000, "");

	/* DT_CAN_TRANSCEIVER_MAX_BITRATE */
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_0, 1000000), 1000000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_0, 5000000), 5000000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_0, 8000000), 5000000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_1, 125000), 125000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_1, 2000000), 2000000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_1, 5000000), 2000000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_2, 125000), 125000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_2, 1000000), 1000000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_2, 5000000), 1000000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_3, 125000), 125000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_3, 1000000), 1000000, "");
	zassert_equal(DT_CAN_TRANSCEIVER_MAX_BITRATE(TEST_CAN_CTRL_3, 5000000), 1000000, "");

	/* DT_INST_CAN_TRANSCEIVER_MAX_BITRATE */
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(0, 1000000), 1000000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(0, 5000000), 5000000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(0, 8000000), 5000000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(1, 125000), 125000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(1, 2000000), 2000000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(1, 5000000), 2000000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(2, 125000), 125000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(2, 1000000), 1000000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(2, 5000000), 1000000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(3, 125000), 125000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(3, 1000000), 1000000, "");
	zassert_equal(DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(3, 5000000), 1000000, "");
}

ZTEST(devicetree_api, test_macro_names)
{
	/* white box */
	zassert_true(!strcmp(TO_STRING(DT_PATH(test, gpio_deadbeef)),
			     "DT_N_S_test_S_gpio_deadbeef"), "");
	zassert_true(!strcmp(TO_STRING(DT_ALIAS(test_alias)),
			     "DT_N_S_test_S_gpio_deadbeef"), "");
	zassert_true(!strcmp(TO_STRING(DT_NODELABEL(test_nodelabel)),
			     "DT_N_S_test_S_gpio_deadbeef"), "");
	zassert_true(!strcmp(TO_STRING(DT_NODELABEL(test_nodelabel_allcaps)),
			     "DT_N_S_test_S_gpio_deadbeef"), "");

#define CHILD_NODE_ID DT_CHILD(DT_PATH(test, i2c_11112222), test_i2c_dev_10)
#define FULL_PATH_ID DT_PATH(test, i2c_11112222, test_i2c_dev_10)

	zassert_true(!strcmp(TO_STRING(CHILD_NODE_ID),
			     TO_STRING(FULL_PATH_ID)), "");

#undef CHILD_NODE_ID
#undef FULL_PATH_ID
}

static int a[] = DT_PROP(TEST_ARRAYS, a);
static unsigned char b[] = DT_PROP(TEST_ARRAYS, b);
static char *c[] = DT_PROP(TEST_ARRAYS, c);

ZTEST(devicetree_api, test_arrays)
{
	int ok;

	zassert_equal(ARRAY_SIZE(a), 3, "");
	zassert_equal(ARRAY_SIZE(b), 4, "");
	zassert_equal(ARRAY_SIZE(c), 2, "");

	zassert_equal(a[0], 1000, "");
	zassert_equal(a[1], 2000, "");
	zassert_equal(a[2], 3000, "");

	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 0), "");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 1), "");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 2), "");
	zassert_false(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 3), "");

	/*
	 * Verify that DT_PROP_HAS_IDX can be used with COND_CODE_1()
	 * and COND_CODE_0(), i.e. its expansion is a literal 1 or 0,
	 * not an equivalent expression that evaluates to 1 or 0.
	 */
	ok = 0;
	COND_CODE_1(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 0), (ok = 1;), ());
	zassert_equal(ok, 1, "");
	ok = 0;
	COND_CODE_0(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 3), (ok = 1;), ());
	zassert_equal(ok, 1, "");

	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, a, 0), a[0], "");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, a, 1), a[1], "");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, a, 2), a[2], "");

	zassert_equal(DT_PROP_LEN(TEST_ARRAYS, a), 3, "");

	zassert_equal(b[0], 0xaa, "");
	zassert_equal(b[1], 0xbb, "");
	zassert_equal(b[2], 0xcc, "");
	zassert_equal(b[3], 0xdd, "");

	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 0), "");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 1), "");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 2), "");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 3), "");
	zassert_false(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 4), "");

	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, b, 0), b[0], "");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, b, 1), b[1], "");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, b, 2), b[2], "");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, b, 3), b[3], "");

	zassert_equal(DT_PROP_LEN(TEST_ARRAYS, b), 4, "");

	zassert_true(!strcmp(c[0], "bar"), "");
	zassert_true(!strcmp(c[1], "baz"), "");

	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, c, 0), "");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, c, 1), "");
	zassert_false(DT_PROP_HAS_IDX(TEST_ARRAYS, c, 2), "");

	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ARRAYS, c, 0), c[0]), "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ARRAYS, c, 1), c[1]), "");

	zassert_equal(DT_PROP_LEN(TEST_ARRAYS, c), 2, "");
}

ZTEST(devicetree_api, test_foreach)
{
	/*
	 * We don't know what platform we are running on, so settle for
	 * some basic checks related to nodes we know are in our overlay.
	 */
#define IS_ALIASES(node_id) + DT_SAME_NODE(DT_PATH(aliases), node_id)
#define IS_DISABLED_GPIO(node_id) + DT_SAME_NODE(DT_NODELABEL(disabled_gpio), \
						node_id)
	zassert_equal(1, DT_FOREACH_NODE(IS_ALIASES), "");
	zassert_equal(1, DT_FOREACH_NODE(IS_DISABLED_GPIO), "");
	zassert_equal(1, DT_FOREACH_STATUS_OKAY_NODE(IS_ALIASES), "");
	zassert_equal(0, DT_FOREACH_STATUS_OKAY_NODE(IS_DISABLED_GPIO), "");

#define IS_ALIASES_VARGS(node_id, mul) + ((mul) * DT_SAME_NODE(DT_PATH(aliases), node_id))
#define IS_DISABLED_GPIO_VARGS(node_id, mul) + ((mul) * \
			       DT_SAME_NODE(DT_NODELABEL(disabled_gpio), node_id))
	zassert_equal(2, DT_FOREACH_NODE_VARGS(IS_ALIASES_VARGS, 2), "");
	zassert_equal(2, DT_FOREACH_NODE_VARGS(IS_DISABLED_GPIO_VARGS, 2), "");
	zassert_equal(2, DT_FOREACH_STATUS_OKAY_NODE_VARGS(IS_ALIASES_VARGS, 2), "");
	zassert_equal(0, DT_FOREACH_STATUS_OKAY_NODE_VARGS(IS_DISABLED_GPIO_VARGS, 2), "");


#undef IS_ALIASES
#undef IS_DISABLED_GPIO
#undef IS_ALIASES_VARGS
#undef IS_DISABLED_GPIO_VARGS
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_gpio_device
ZTEST(devicetree_api, test_foreach_status_okay)
{
	/*
	 * For-each-node type macro tests.
	 *
	 * See test_foreach_prop_elem*() for tests of
	 * for-each-property type macros.
	 */
	unsigned int val;
	const char *str;

	/* This should expand to something like:
	 *
	 * "/test/enum-0" "/test/enum-1"
	 *
	 * but there is no guarantee about the order of nodes in the
	 * expansion, so we test both.
	 */
	str = DT_FOREACH_STATUS_OKAY(vnd_enum_holder, DT_NODE_PATH);
	zassert_true(!strcmp(str, "/test/enum-0/test/enum-1") ||
		     !strcmp(str, "/test/enum-1/test/enum-0"), "");

#undef MY_FN
#define MY_FN(node_id, operator) DT_ENUM_IDX(node_id, val) operator
	/* This should expand to something like:
	 *
	 * 0 + 2 + 3
	 *
	 * and order of expansion doesn't matter, since we're adding
	 * the values all up.
	 */
	val = DT_FOREACH_STATUS_OKAY_VARGS(vnd_enum_holder, MY_FN, +) 3;
	zassert_equal(val, 5, "");

	/*
	 * Make sure DT_INST_FOREACH_STATUS_OKAY can be called from functions
	 * using macros with side effects in the current scope.
	 */
	val = 0;
#define INC(inst_ignored) do { val++; } while (0);
	DT_INST_FOREACH_STATUS_OKAY(INC)
	zassert_equal(val, 2, "");
#undef INC

	val = 0;
#define INC_ARG(arg) do { val++; val += arg; } while (0)
#define INC(inst_ignored, arg) INC_ARG(arg);
	DT_INST_FOREACH_STATUS_OKAY_VARGS(INC, 1)
	zassert_equal(val, 4, "");
#undef INC_ARG
#undef INC

	/*
	 * Make sure DT_INST_FOREACH_STATUS_OKAY works with 0 instances, and does
	 * not expand its argument at all.
	 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT xxxx
#define BUILD_BUG_ON_EXPANSION (there is a bug in devicetree.h)
	DT_INST_FOREACH_STATUS_OKAY(BUILD_BUG_ON_EXPANSION)
#undef BUILD_BUG_ON_EXPANSION

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT xxxx
#define BUILD_BUG_ON_EXPANSION(arg) (there is a bug in devicetree.h)
	DT_INST_FOREACH_STATUS_OKAY_VARGS(BUILD_BUG_ON_EXPANSION, 1)
#undef BUILD_BUG_ON_EXPANSION
}

ZTEST(devicetree_api, test_foreach_prop_elem)
{
#define TIMES_TWO(node_id, prop, idx) \
	(2 * DT_PROP_BY_IDX(node_id, prop, idx)),
#define BY_IDX_COMMA(node_id, prop, idx) DT_PROP_BY_IDX(node_id, prop, idx),

	int array_a[] = {
		DT_FOREACH_PROP_ELEM(TEST_ARRAYS, a, TIMES_TWO)
	};

	zassert_equal(ARRAY_SIZE(array_a), 3, "");
	zassert_equal(array_a[0], 2000, "");
	zassert_equal(array_a[1], 4000, "");
	zassert_equal(array_a[2], 6000, "");

	int array_b[] = {
		DT_FOREACH_PROP_ELEM(TEST_ARRAYS, b, TIMES_TWO)
	};

	zassert_equal(ARRAY_SIZE(array_b), 4, "");
	zassert_equal(array_b[0], 2 * 0xAA, "");
	zassert_equal(array_b[1], 2 * 0xBB, "");
	zassert_equal(array_b[2], 2 * 0xCC, "");
	zassert_equal(array_b[3], 2 * 0xDD, "");

	static const char * const array_c[] = {
		DT_FOREACH_PROP_ELEM(TEST_ARRAYS, c, BY_IDX_COMMA)
	};

	zassert_equal(ARRAY_SIZE(array_c), 2, "");
	zassert_true(!strcmp(array_c[0], "bar"), "");
	zassert_true(!strcmp(array_c[1], "baz"), "");

	static const char * const array_val[] = {
		DT_FOREACH_PROP_ELEM(TEST_ENUM_0, val, BY_IDX_COMMA)
	};

	zassert_equal(ARRAY_SIZE(array_val), 1, "");
	zassert_true(!strcmp(array_val[0], "zero"), "");

	static const char * const string_zephyr_user[] = {
		DT_FOREACH_PROP_ELEM(ZEPHYR_USER, string, BY_IDX_COMMA)
	};

	zassert_equal(ARRAY_SIZE(string_zephyr_user), 1, "");
	zassert_true(!strcmp(string_zephyr_user[0], "foo"), "");

#undef BY_IDX_COMMA

#define PATH_COMMA(node_id, prop, idx) \
	DT_NODE_PATH(DT_PROP_BY_IDX(node_id, prop, idx)),

	static const char * const array_ph[] = {
		DT_FOREACH_PROP_ELEM(TEST_PH, ph, PATH_COMMA)
	};

	zassert_equal(ARRAY_SIZE(array_ph), 1, "");
	zassert_true(!strcmp(array_ph[0], DT_NODE_PATH(TEST_GPIO_1)), "");

	static const char * const array_zephyr_user_ph[] = {
		DT_FOREACH_PROP_ELEM(ZEPHYR_USER, ph, PATH_COMMA)
	};

	zassert_equal(ARRAY_SIZE(array_zephyr_user_ph), 1, "");
	zassert_true(!strcmp(array_zephyr_user_ph[0],
			     DT_NODE_PATH(TEST_GPIO_1)), "");

	static const char * const array_phs[] = {
		DT_FOREACH_PROP_ELEM(TEST_PH, phs, PATH_COMMA)
	};

	zassert_equal(ARRAY_SIZE(array_phs), 2, "");
	zassert_true(!strcmp(array_phs[0], DT_NODE_PATH(TEST_I2C)), "");
	zassert_true(!strcmp(array_phs[1], DT_NODE_PATH(TEST_SPI)), "");

#undef PATH_COMMA

#define PIN_COMMA(node_id, prop, idx) DT_GPIO_PIN_BY_IDX(node_id, prop, idx),

	int array_gpios[] = {
		DT_FOREACH_PROP_ELEM(TEST_PH, gpios, PIN_COMMA)
	};

	zassert_equal(ARRAY_SIZE(array_gpios), 2, "");
	zassert_equal(array_gpios[0], 10, "");
	zassert_equal(array_gpios[1], 30, "");

#undef PATH_COMMA

	int array_sep[] = {
		DT_FOREACH_PROP_ELEM_SEP(TEST_ARRAYS, a, DT_PROP_BY_IDX, (,))
	};

	zassert_equal(ARRAY_SIZE(array_sep), 3, "");
	zassert_equal(array_sep[0], 1000, "");
	zassert_equal(array_sep[1], 2000, "");
	zassert_equal(array_sep[2], 3000, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_array_holder

	int inst_array[] = {
		DT_INST_FOREACH_PROP_ELEM(0, a, TIMES_TWO)
	};

	zassert_equal(ARRAY_SIZE(inst_array), ARRAY_SIZE(array_a), "");
	zassert_equal(inst_array[0], array_a[0], "");
	zassert_equal(inst_array[1], array_a[1], "");
	zassert_equal(inst_array[2], array_a[2], "");

	int inst_array_sep[] = {
		DT_INST_FOREACH_PROP_ELEM_SEP(0, a, DT_PROP_BY_IDX, (,))
	};

	zassert_equal(ARRAY_SIZE(inst_array_sep), ARRAY_SIZE(array_sep), "");
	zassert_equal(inst_array_sep[0], array_sep[0], "");
	zassert_equal(inst_array_sep[1], array_sep[1], "");
	zassert_equal(inst_array_sep[2], array_sep[2], "");
#undef TIMES_TWO
}

ZTEST(devicetree_api, test_foreach_prop_elem_varg)
{
#define TIMES_TWO_ADD(node_id, prop, idx, arg) \
	((2 * DT_PROP_BY_IDX(node_id, prop, idx)) + arg),

	int array[] = {
		DT_FOREACH_PROP_ELEM_VARGS(TEST_ARRAYS, a, TIMES_TWO_ADD, 3)
	};

	zassert_equal(ARRAY_SIZE(array), 3, "");
	zassert_equal(array[0], 2003, "");
	zassert_equal(array[1], 4003, "");
	zassert_equal(array[2], 6003, "");

#define PROP_PLUS_ARG(node_id, prop, idx, arg) \
	(DT_PROP_BY_IDX(node_id, prop, idx) + arg)

	int array_sep[] = {
		DT_FOREACH_PROP_ELEM_SEP_VARGS(TEST_ARRAYS, a, PROP_PLUS_ARG,
					       (,), 3)
	};

	zassert_equal(ARRAY_SIZE(array_sep), 3, "");
	zassert_equal(array_sep[0], 1003, "");
	zassert_equal(array_sep[1], 2003, "");
	zassert_equal(array_sep[2], 3003, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_array_holder

	int inst_array[] = {
		DT_INST_FOREACH_PROP_ELEM_VARGS(0, a, TIMES_TWO_ADD, 3)
	};

	zassert_equal(ARRAY_SIZE(inst_array), ARRAY_SIZE(array), "");
	zassert_equal(inst_array[0], array[0], "");
	zassert_equal(inst_array[1], array[1], "");
	zassert_equal(inst_array[2], array[2], "");

	int inst_array_sep[] = {
		DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(0, a, PROP_PLUS_ARG, (,),
						    3)
	};

	zassert_equal(ARRAY_SIZE(inst_array_sep), ARRAY_SIZE(array_sep), "");
	zassert_equal(inst_array_sep[0], array_sep[0], "");
	zassert_equal(inst_array_sep[1], array_sep[1], "");
	zassert_equal(inst_array_sep[2], array_sep[2], "");
#undef TIMES_TWO
}

struct test_gpio_info {
	uint32_t reg_addr;
	uint32_t reg_len;
};

struct test_gpio_data {
	bool init_called;
	bool is_gpio_ctlr;
};

static int test_gpio_init(const struct device *dev)
{
	struct test_gpio_data *data = dev->data;

	data->init_called = 1;
	return 0;
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_gpio_device

static const struct gpio_driver_api test_api;

#define TEST_GPIO_INIT(num)					\
	static struct test_gpio_data gpio_data_##num = {	\
		.is_gpio_ctlr = DT_INST_PROP(num,		\
					     gpio_controller),	\
	};							\
	static const struct test_gpio_info gpio_info_##num = {	\
		.reg_addr = DT_INST_REG_ADDR(num),		\
		.reg_len = DT_INST_REG_SIZE(num),		\
	};							\
	DEVICE_DT_INST_DEFINE(num,				\
			      test_gpio_init,			\
			      NULL,				\
			      &gpio_data_##num,			\
			      &gpio_info_##num,			\
			      POST_KERNEL,			\
			      CONFIG_APPLICATION_INIT_PRIORITY,	\
			      &test_api);

DT_INST_FOREACH_STATUS_OKAY(TEST_GPIO_INIT)

ZTEST(devicetree_api, test_devices)
{
	const struct device *devs[2] = { DEVICE_DT_INST_GET(0), DEVICE_DT_INST_GET(1) };
	const struct device *dev_abcd;
	struct test_gpio_data *data_dev0;
	struct test_gpio_data *data_dev1;
	const struct test_gpio_info *config_abdc;

	zassert_equal(DT_NUM_INST_STATUS_OKAY(vnd_gpio_device), 2, "");

	data_dev0 = devs[0]->data;
	data_dev1 = devs[1]->data;

	zassert_not_null(devs[0], "");
	zassert_not_null(devs[1], "");

	zassert_true(data_dev0->is_gpio_ctlr, "");
	zassert_true(data_dev1->is_gpio_ctlr, "");
	zassert_true(data_dev0->init_called, "");
	zassert_true(data_dev1->init_called, "");

	dev_abcd = DEVICE_DT_GET(TEST_ABCD1234);
	config_abdc = dev_abcd->config;
	zassert_not_null(dev_abcd, "");
	zassert_equal(config_abdc->reg_addr, 0xabcd1234, "");
	zassert_equal(config_abdc->reg_len, 0x500, "");
}

ZTEST(devicetree_api, test_cs_gpios)
{
	zassert_equal(DT_SPI_HAS_CS_GPIOS(TEST_SPI_NO_CS), 0, "");
	zassert_equal(DT_SPI_NUM_CS_GPIOS(TEST_SPI_NO_CS), 0, "");

	zassert_equal(DT_SPI_HAS_CS_GPIOS(TEST_SPI), 1, "");
	zassert_equal(DT_SPI_NUM_CS_GPIOS(TEST_SPI), 3, "");

	zassert_equal(DT_DEP_ORD(DT_SPI_DEV_CS_GPIOS_CTLR(TEST_SPI_DEV_0)),
		      DT_DEP_ORD(DT_NODELABEL(test_gpio_1)),
		     "dev 0 cs gpio controller");
	zassert_equal(DT_SPI_DEV_CS_GPIOS_PIN(TEST_SPI_DEV_0), 0x10, "");
	zassert_equal(DT_SPI_DEV_CS_GPIOS_FLAGS(TEST_SPI_DEV_0), 0x20, "");
}

ZTEST(devicetree_api, test_chosen)
{
	zassert_equal(DT_HAS_CHOSEN(ztest_xxxx), 0, "");
	zassert_equal(DT_HAS_CHOSEN(ztest_gpio), 1, "");
	zassert_true(!strcmp(TO_STRING(DT_CHOSEN(ztest_gpio)),
			     "DT_N_S_test_S_gpio_deadbeef"), "");
}

#define TO_MY_ENUM(token) TO_MY_ENUM_2(token) /* force another expansion */
#define TO_MY_ENUM_2(token) MY_ENUM_ ## token
ZTEST(devicetree_api, test_enums)
{
	enum {
		MY_ENUM_zero = 0xff,
		MY_ENUM_ZERO = 0xaa,
	};

	/* DT_ENUM_IDX and DT_ENUM_HAS_VALUE on string enum */
	zassert_equal(DT_ENUM_IDX(TEST_ENUM_0, val), 0, "0");
	zassert_true(DT_ENUM_HAS_VALUE(TEST_ENUM_0, val, zero), "");
	zassert_false(DT_ENUM_HAS_VALUE(TEST_ENUM_0, val, one), "");
	zassert_false(DT_ENUM_HAS_VALUE(TEST_ENUM_0, val, two), "");

	/* DT_ENUM_IDX and DT_ENUM_HAS_VALUE on int enum */
	zassert_equal(DT_ENUM_IDX(DT_NODELABEL(test_enum_int_default_0), val), 0, "0");
	zassert_true(DT_ENUM_HAS_VALUE(DT_NODELABEL(test_enum_int_default_0), val, 5), "");
	zassert_false(DT_ENUM_HAS_VALUE(DT_NODELABEL(test_enum_int_default_0), val, 6), "");
	zassert_false(DT_ENUM_HAS_VALUE(DT_NODELABEL(test_enum_int_default_0), val, 7), "");
}
#undef TO_MY_ENUM
#undef TO_MY_ENUM_2

ZTEST(devicetree_api, test_enums_required_false)
{
	/* DT_ENUM_IDX_OR on string value */
	zassert_equal(DT_ENUM_IDX_OR(DT_NODELABEL(test_enum_default_0), val, 2),
		      1, "");
	zassert_equal(DT_ENUM_IDX_OR(DT_NODELABEL(test_enum_default_1), val, 2),
		      2, "");
	/* DT_ENUM_IDX_OR on int value */
	zassert_equal(DT_ENUM_IDX_OR(DT_NODELABEL(test_enum_int_default_0),
				     val, 4),
		      0, "");
	zassert_equal(DT_ENUM_IDX_OR(DT_NODELABEL(test_enum_int_default_1),
				     val, 4),
		      4, "");
}

ZTEST(devicetree_api, test_inst_enums)
{
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_enum_holder_inst
	zassert_equal(DT_INST_ENUM_IDX(0, val), 0, "");
	zassert_equal(DT_INST_ENUM_IDX_OR(0, val, 2), 0, "");
	zassert_true(DT_INST_ENUM_HAS_VALUE(0, val, zero), "");
	zassert_false(DT_INST_ENUM_HAS_VALUE(0, val, one), "");
	zassert_false(DT_INST_ENUM_HAS_VALUE(0, val, two), "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_enum_required_false_holder_inst
	zassert_equal(DT_INST_ENUM_IDX_OR(0, val, 2), 2, "");
	zassert_false(DT_INST_ENUM_HAS_VALUE(0, val, zero), "");
	zassert_false(DT_INST_ENUM_HAS_VALUE(0, val, one), "");
	zassert_false(DT_INST_ENUM_HAS_VALUE(0, val, two), "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
ZTEST(devicetree_api, test_clocks)
{
	/* DT_CLOCKS_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(TEST_TEMP, 1),
				  DT_NODELABEL(test_fixed_clk)), "");

	/* DT_CLOCKS_CTLR */
	zassert_true(DT_SAME_NODE(DT_CLOCKS_CTLR(TEST_TEMP),
				  DT_NODELABEL(test_clk)), "");

	/* DT_CLOCKS_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_CLOCKS_CTLR_BY_NAME(TEST_TEMP, clk_b),
				  DT_NODELABEL(test_clk)), "");

	/* DT_NUM_CLOCKS */
	zassert_equal(DT_NUM_CLOCKS(TEST_TEMP), 3, "");

	/* DT_CLOCKS_HAS_IDX */
	zassert_true(DT_CLOCKS_HAS_IDX(TEST_TEMP, 2), "");
	zassert_false(DT_CLOCKS_HAS_IDX(TEST_TEMP, 3), "");

	/* DT_CLOCKS_HAS_NAME */
	zassert_true(DT_CLOCKS_HAS_NAME(TEST_TEMP, clk_a), "");
	zassert_false(DT_CLOCKS_HAS_NAME(TEST_TEMP, clk_z), "");

	/* DT_CLOCKS_CELL_BY_IDX */
	zassert_equal(DT_CLOCKS_CELL_BY_IDX(TEST_TEMP, 2, bits), 2, "");
	zassert_equal(DT_CLOCKS_CELL_BY_IDX(TEST_TEMP, 2, bus), 8, "");

	/* DT_CLOCKS_CELL_BY_NAME */
	zassert_equal(DT_CLOCKS_CELL_BY_NAME(TEST_TEMP, clk_a, bits), 7, "");
	zassert_equal(DT_CLOCKS_CELL_BY_NAME(TEST_TEMP, clk_b, bus), 8, "");

	/* DT_CLOCKS_CELL */
	zassert_equal(DT_CLOCKS_CELL(TEST_TEMP, bits), 7, "");
	zassert_equal(DT_CLOCKS_CELL(TEST_TEMP, bus), 3, "");

	/* clock-freq on fixed clock */
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_TEMP, clocks, 1,
					     clock_frequency),
		      25000000, "");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	/* DT_INST_CLOCKS_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_INST_CLOCKS_CTLR_BY_IDX(0, 1),
				  DT_NODELABEL(test_fixed_clk)), "");

	/* DT_INST_CLOCKS_CTLR */
	zassert_true(DT_SAME_NODE(DT_INST_CLOCKS_CTLR(0),
				  DT_NODELABEL(test_clk)), "");

	/* DT_INST_CLOCKS_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk_b),
				  DT_NODELABEL(test_clk)), "");

	/* DT_INST_NUM_CLOCKS */
	zassert_equal(DT_INST_NUM_CLOCKS(0), 3, "");

	/* DT_INST_CLOCKS_HAS_IDX */
	zassert_true(DT_INST_CLOCKS_HAS_IDX(0, 2), "");
	zassert_false(DT_INST_CLOCKS_HAS_IDX(0, 3), "");

	/* DT_INST_CLOCKS_HAS_NAME */
	zassert_true(DT_INST_CLOCKS_HAS_NAME(0, clk_a), "");
	zassert_false(DT_INST_CLOCKS_HAS_NAME(0, clk_z), "");

	/* DT_INST_CLOCKS_CELL_BY_IDX */
	zassert_equal(DT_INST_CLOCKS_CELL_BY_IDX(0, 2, bits), 2, "");
	zassert_equal(DT_INST_CLOCKS_CELL_BY_IDX(0, 2, bus), 8, "");

	/* DT_INST_CLOCKS_CELL_BY_NAME */
	zassert_equal(DT_INST_CLOCKS_CELL_BY_NAME(0, clk_a, bits), 7, "");
	zassert_equal(DT_INST_CLOCKS_CELL_BY_NAME(0, clk_b, bus), 8, "");

	/* DT_INST_CLOCKS_CELL */
	zassert_equal(DT_INST_CLOCKS_CELL(0, bits), 7, "");
	zassert_equal(DT_INST_CLOCKS_CELL(0, bus), 3, "");

	/* clock-freq on fixed clock */
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, clocks, 1,
						  clock_frequency),
		      25000000, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_spi_device
ZTEST(devicetree_api, test_parent)
{
	zassert_true(DT_SAME_NODE(DT_PARENT(TEST_SPI_DEV_0), TEST_SPI_BUS_0), "");

	/*
	 * The parent's label for the first instance of vnd,spi-device,
	 * child of TEST_SPI, is the same as TEST_SPI.
	 */
	zassert_true(DT_SAME_NODE(DT_INST_PARENT(0), TEST_SPI), "");
	/*
	 * We should be able to use DT_PARENT() even with nodes, like /test,
	 * that have no matching compatible.
	 */
	zassert_true(DT_SAME_NODE(DT_CHILD(DT_PARENT(TEST_SPI_BUS_0), spi_33334444),
				  TEST_SPI_BUS_0), "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_i2c_mux_controller
ZTEST(devicetree_api, test_gparent)
{
	zassert_true(DT_SAME_NODE(DT_GPARENT(TEST_I2C_MUX_CTLR_1), TEST_I2C), "");
	zassert_true(DT_SAME_NODE(DT_INST_GPARENT(0), TEST_I2C), "");
	zassert_true(DT_SAME_NODE(DT_INST_GPARENT(1), TEST_I2C), "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_child_bindings
ZTEST(devicetree_api, test_children)
{
	zassert_equal(DT_PROP(DT_CHILD(DT_NODELABEL(test_children), child_a),
			      val), 0, "");
	zassert_equal(DT_PROP(DT_CHILD(DT_NODELABEL(test_children), child_b),
			      val), 1, "");
	zassert_equal(DT_PROP(DT_CHILD(DT_NODELABEL(test_children), child_c),
			      val), 2, "");

	zassert_equal(DT_PROP(DT_INST_CHILD(0, child_a), val), 0, "");
	zassert_equal(DT_PROP(DT_INST_CHILD(0, child_b), val), 1, "");
	zassert_equal(DT_PROP(DT_INST_CHILD(0, child_c), val), 2, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_child_bindings
ZTEST(devicetree_api, test_child_nodes_list)
{
	#define TEST_FUNC(child) { DT_PROP(child, val) }
	#define TEST_FUNC_AND_COMMA(child) TEST_FUNC(child),
	#define TEST_PARENT DT_PARENT(DT_NODELABEL(test_child_a))

	struct vnd_child_binding {
		int val;
	};

	struct vnd_child_binding vals[] = {
		DT_FOREACH_CHILD(TEST_PARENT, TEST_FUNC_AND_COMMA)
	};

	struct vnd_child_binding vals_sep[] = {
		DT_FOREACH_CHILD_SEP(TEST_PARENT, TEST_FUNC, (,))
	};

	struct vnd_child_binding vals_inst[] = {
		DT_INST_FOREACH_CHILD(0, TEST_FUNC_AND_COMMA)
	};

	struct vnd_child_binding vals_inst_sep[] = {
		DT_INST_FOREACH_CHILD_SEP(0, TEST_FUNC, (,))
	};

	struct vnd_child_binding vals_status_okay[] = {
		DT_FOREACH_CHILD_STATUS_OKAY(TEST_PARENT, TEST_FUNC_AND_COMMA)
	};

	struct vnd_child_binding vals_status_okay_inst[] = {
		DT_INST_FOREACH_CHILD_STATUS_OKAY(0, TEST_FUNC_AND_COMMA)
	};

	struct vnd_child_binding vals_status_okay_sep[] = {
		DT_FOREACH_CHILD_STATUS_OKAY_SEP(TEST_PARENT, TEST_FUNC, (,))
	};

	struct vnd_child_binding vals_status_okay_inst_sep[] = {
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(0, TEST_FUNC, (,))
	};

	zassert_equal(ARRAY_SIZE(vals), 3, "");
	zassert_equal(ARRAY_SIZE(vals_sep), 3, "");
	zassert_equal(ARRAY_SIZE(vals_inst), 3, "");
	zassert_equal(ARRAY_SIZE(vals_inst_sep), 3, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay), 2, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay_inst), 2, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay_sep), 2, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay_inst_sep), 2, "");

	zassert_equal(vals[0].val, 0, "");
	zassert_equal(vals[1].val, 1, "");
	zassert_equal(vals[2].val, 2, "");
	zassert_equal(vals_sep[0].val, 0, "");
	zassert_equal(vals_sep[1].val, 1, "");
	zassert_equal(vals_sep[2].val, 2, "");
	zassert_equal(vals_inst[0].val, 0, "");
	zassert_equal(vals_inst[1].val, 1, "");
	zassert_equal(vals_inst[2].val, 2, "");
	zassert_equal(vals_inst_sep[0].val, 0, "");
	zassert_equal(vals_inst_sep[1].val, 1, "");
	zassert_equal(vals_inst_sep[2].val, 2, "");
	zassert_equal(vals_status_okay[0].val, 0, "");
	zassert_equal(vals_status_okay[1].val, 1, "");
	zassert_equal(vals_status_okay_inst[0].val, 0, "");
	zassert_equal(vals_status_okay_inst[1].val, 1, "");
	zassert_equal(vals_status_okay_sep[0].val, 0, "");
	zassert_equal(vals_status_okay_sep[1].val, 1, "");
	zassert_equal(vals_status_okay_inst_sep[0].val, 0, "");
	zassert_equal(vals_status_okay_inst_sep[1].val, 1, "");

	#undef TEST_PARENT
	#undef TEST_FUNC_AND_COMMA
	#undef TEST_FUNC
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_child_bindings
ZTEST(devicetree_api, test_child_nodes_list_varg)
{
	#define TEST_FUNC(child, arg) { DT_PROP(child, val) + arg }
	#define TEST_FUNC_AND_COMMA(child, arg) TEST_FUNC(child, arg),
	#define TEST_PARENT DT_PARENT(DT_NODELABEL(test_child_a))

	struct vnd_child_binding {
		int val;
	};

	struct vnd_child_binding vals[] = {
		DT_FOREACH_CHILD_VARGS(TEST_PARENT, TEST_FUNC_AND_COMMA, 1)
	};

	struct vnd_child_binding vals_sep[] = {
		DT_FOREACH_CHILD_SEP_VARGS(TEST_PARENT, TEST_FUNC, (,), 1)
	};

	struct vnd_child_binding vals_inst[] = {
		DT_INST_FOREACH_CHILD_VARGS(0, TEST_FUNC_AND_COMMA, 1)
	};

	struct vnd_child_binding vals_inst_sep[] = {
		DT_INST_FOREACH_CHILD_SEP_VARGS(0, TEST_FUNC, (,), 1)
	};

	struct vnd_child_binding vals_status_okay[] = {
		DT_FOREACH_CHILD_STATUS_OKAY_VARGS(TEST_PARENT, TEST_FUNC_AND_COMMA, 1)
	};

	struct vnd_child_binding vals_status_okay_inst[] = {
		DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(0, TEST_FUNC_AND_COMMA, 1)
	};

	struct vnd_child_binding vals_status_okay_sep[] = {
		DT_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(TEST_PARENT, TEST_FUNC, (,), 1)
	};

	struct vnd_child_binding vals_status_okay_inst_sep[] = {
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(0, TEST_FUNC, (,), 1)
	};

	zassert_equal(ARRAY_SIZE(vals), 3, "");
	zassert_equal(ARRAY_SIZE(vals_sep), 3, "");
	zassert_equal(ARRAY_SIZE(vals_inst), 3, "");
	zassert_equal(ARRAY_SIZE(vals_inst_sep), 3, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay), 2, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay_inst), 2, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay_sep), 2, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay_inst_sep), 2, "");

	zassert_equal(vals[0].val, 1, "");
	zassert_equal(vals[1].val, 2, "");
	zassert_equal(vals[2].val, 3, "");
	zassert_equal(vals_sep[0].val, 1, "");
	zassert_equal(vals_sep[1].val, 2, "");
	zassert_equal(vals_sep[2].val, 3, "");
	zassert_equal(vals_inst[0].val, 1, "");
	zassert_equal(vals_inst[1].val, 2, "");
	zassert_equal(vals_inst[2].val, 3, "");
	zassert_equal(vals_inst_sep[0].val, 1, "");
	zassert_equal(vals_inst_sep[1].val, 2, "");
	zassert_equal(vals_inst_sep[2].val, 3, "");
	zassert_equal(vals_status_okay[0].val, 1, "");
	zassert_equal(vals_status_okay[1].val, 2, "");
	zassert_equal(vals_status_okay_inst[0].val, 1, "");
	zassert_equal(vals_status_okay_inst[1].val, 2, "");
	zassert_equal(vals_status_okay_sep[0].val, 1, "");
	zassert_equal(vals_status_okay_sep[1].val, 2, "");
	zassert_equal(vals_status_okay_inst_sep[0].val, 1, "");
	zassert_equal(vals_status_okay_inst_sep[1].val, 2, "");

	#undef TEST_PARENT
	#undef TEST_FUNC_AND_COMMA
	#undef TEST_FUNC
}

ZTEST(devicetree_api, test_great_grandchild)
{
	zassert_equal(DT_PROP(DT_NODELABEL(test_ggc), ggc_prop), 42, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_test_ranges_pcie
ZTEST(devicetree_api, test_ranges_pcie)
{
#define FLAGS(node_id, idx)				\
	DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx),
#define CHILD_BUS_ADDR(node_id, idx)				\
	DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(node_id, idx),
#define PARENT_BUS_ADDR(node_id, idx)				\
	DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(node_id, idx),
#define LENGTH(node_id, idx) DT_RANGES_LENGTH_BY_IDX(node_id, idx),

	unsigned int count = DT_NUM_RANGES(TEST_RANGES_PCIE);

	const uint64_t ranges_pcie_flags[] = {
		DT_FOREACH_RANGE(TEST_RANGES_PCIE, FLAGS)
	};

	const uint64_t ranges_child_bus_addr[] = {
		DT_FOREACH_RANGE(TEST_RANGES_PCIE, CHILD_BUS_ADDR)
	};

	const uint64_t ranges_parent_bus_addr[] = {
		DT_FOREACH_RANGE(TEST_RANGES_PCIE, PARENT_BUS_ADDR)
	};

	const uint64_t ranges_length[] = {
		DT_FOREACH_RANGE(TEST_RANGES_PCIE, LENGTH)
	};

	zassert_equal(count, 3, "");

	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_PCIE, 0), 1, "");
	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_PCIE, 1), 1, "");
	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_PCIE, 2), 1, "");
	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_PCIE, 3), 0, "");

	zassert_equal(DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(TEST_RANGES_PCIE, 0),
		      1, "");
	zassert_equal(DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(TEST_RANGES_PCIE, 1),
		      1, "");
	zassert_equal(DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(TEST_RANGES_PCIE, 2),
		      1, "");
	zassert_equal(DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(TEST_RANGES_PCIE, 3),
		      0, "");

	zassert_equal(ranges_pcie_flags[0], 0x1000000, "");
	zassert_equal(ranges_pcie_flags[1], 0x2000000, "");
	zassert_equal(ranges_pcie_flags[2], 0x3000000, "");
	zassert_equal(ranges_child_bus_addr[0], 0, "");
	zassert_equal(ranges_child_bus_addr[1], 0x10000000, "");
	zassert_equal(ranges_child_bus_addr[2], 0x8000000000, "");
	zassert_equal(ranges_parent_bus_addr[0], 0x3eff0000, "");
	zassert_equal(ranges_parent_bus_addr[1], 0x10000000, "");
	zassert_equal(ranges_parent_bus_addr[2], 0x8000000000, "");
	zassert_equal(ranges_length[0], 0x10000, "");
	zassert_equal(ranges_length[1], 0x2eff0000, "");
	zassert_equal(ranges_length[2], 0x8000000000, "");

#undef FLAGS
#undef CHILD_BUS_ADDR
#undef PARENT_BUS_ADDR
#undef LENGTH
}

ZTEST(devicetree_api, test_ranges_other)
{
#define HAS_FLAGS(node_id, idx) \
	DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(node_id, idx)
#define FLAGS(node_id, idx) \
	DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx),
#define CHILD_BUS_ADDR(node_id, idx) \
	DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(node_id, idx),
#define PARENT_BUS_ADDR(node_id, idx) \
	DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(node_id, idx),
#define LENGTH(node_id, idx) DT_RANGES_LENGTH_BY_IDX(node_id, idx),

	unsigned int count = DT_NUM_RANGES(TEST_RANGES_OTHER);

	const uint32_t ranges_child_bus_addr[] = {
		DT_FOREACH_RANGE(TEST_RANGES_OTHER, CHILD_BUS_ADDR)
	};

	const uint32_t ranges_parent_bus_addr[] = {
		DT_FOREACH_RANGE(TEST_RANGES_OTHER, PARENT_BUS_ADDR)
	};

	const uint32_t ranges_length[] = {
		DT_FOREACH_RANGE(TEST_RANGES_OTHER, LENGTH)
	};

	zassert_equal(count, 2, "");

	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_OTHER, 0), 1, "");
	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_OTHER, 1), 1, "");
	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_OTHER, 2), 0, "");
	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_OTHER, 3), 0, "");

	zassert_equal(HAS_FLAGS(TEST_RANGES_OTHER, 0), 0, "");
	zassert_equal(HAS_FLAGS(TEST_RANGES_OTHER, 1), 0, "");
	zassert_equal(HAS_FLAGS(TEST_RANGES_OTHER, 2), 0, "");
	zassert_equal(HAS_FLAGS(TEST_RANGES_OTHER, 3), 0, "");

	zassert_equal(ranges_child_bus_addr[0], 0, "");
	zassert_equal(ranges_child_bus_addr[1], 0x10000000, "");
	zassert_equal(ranges_parent_bus_addr[0], 0x3eff0000, "");
	zassert_equal(ranges_parent_bus_addr[1], 0x10000000, "");
	zassert_equal(ranges_length[0], 0x10000, "");
	zassert_equal(ranges_length[1], 0x2eff0000, "");

#undef HAS_FLAGS
#undef FLAGS
#undef CHILD_BUS_ADDR
#undef PARENT_BUS_ADDR
#undef LENGTH
}

ZTEST(devicetree_api, test_ranges_empty)
{
	zassert_equal(DT_NODE_HAS_PROP(TEST_RANGES_EMPTY, ranges), 1, "");

	zassert_equal(DT_NUM_RANGES(TEST_RANGES_EMPTY), 0, "");

	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_EMPTY, 0), 0, "");
	zassert_equal(DT_RANGES_HAS_IDX(TEST_RANGES_EMPTY, 1), 0, "");

#define FAIL(node_id, idx) ztest_test_fail();

	DT_FOREACH_RANGE(TEST_RANGES_EMPTY, FAIL);

#undef FAIL
}

ZTEST(devicetree_api, test_compat_get_any_status_okay)
{
	zassert_true(
		DT_SAME_NODE(
			DT_COMPAT_GET_ANY_STATUS_OKAY(vnd_reg_holder),
			TEST_REG),
		"");

	/*
	 * DT_SAME_NODE requires that both its arguments are valid
	 * node identifiers, so we can't pass it DT_INVALID_NODE,
	 * which is what this DT_COMPAT_GET_ANY_STATUS_OKAY() expands to.
	 */
	zassert_false(
		DT_NODE_EXISTS(
			DT_COMPAT_GET_ANY_STATUS_OKAY(this_is_not_a_real_compat)),
		"");
}

static bool ord_in_array(unsigned int ord, unsigned int *array,
			 size_t array_size)
{
	size_t i;

	for (i = 0; i < array_size; i++) {
		if (array[i] == ord) {
			return true;
		}
	}

	return false;
}

/* Magic numbers used by COMBINED_ORD_ARRAY. Must be invalid dependency
 * ordinals.
 */
#define ORD_LIST_SEP		0xFFFF0000
#define ORD_LIST_END		0xFFFF0001
#define INJECTED_DEP_0		0xFFFF0002
#define INJECTED_DEP_1		0xFFFF0003

#define DEP_ORD_AND_COMMA(node_id) DT_DEP_ORD(node_id),
#define CHILD_ORDINALS(node_id) DT_FOREACH_CHILD(node_id, DEP_ORD_AND_COMMA)

#define COMBINED_ORD_ARRAY(node_id)		\
	{					\
		DT_DEP_ORD(node_id),		\
		DT_DEP_ORD(DT_PARENT(node_id)),	\
		CHILD_ORDINALS(node_id)		\
		ORD_LIST_SEP,			\
		DT_REQUIRES_DEP_ORDS(node_id)	\
		INJECTED_DEP_0,			\
		INJECTED_DEP_1,			\
		ORD_LIST_SEP,			\
		DT_SUPPORTS_DEP_ORDS(node_id)	\
		ORD_LIST_END			\
	}

ZTEST(devicetree_api, test_dep_ord)
{
#define ORD_IN_ARRAY(ord, array) ord_in_array(ord, array, ARRAY_SIZE(array))

	unsigned int root_ord = DT_DEP_ORD(DT_ROOT),
		test_ord = DT_DEP_ORD(DT_PATH(test)),
		root_requires[] = { DT_REQUIRES_DEP_ORDS(DT_ROOT) },
		test_requires[] = { DT_REQUIRES_DEP_ORDS(DT_PATH(test)) },
		root_supports[] = { DT_SUPPORTS_DEP_ORDS(DT_ROOT) },
		test_supports[] = { DT_SUPPORTS_DEP_ORDS(DT_PATH(test)) },
		children_ords[] = {
			DT_FOREACH_CHILD(TEST_CHILDREN, DEP_ORD_AND_COMMA)
		},
		children_combined_ords[] = COMBINED_ORD_ARRAY(TEST_CHILDREN),
		child_a_combined_ords[] =
			COMBINED_ORD_ARRAY(DT_NODELABEL(test_child_a));
	size_t i;

	/* DT_DEP_ORD */
	zassert_equal(root_ord, 0, "");
	zassert_true(DT_DEP_ORD(DT_NODELABEL(test_child_a)) >
		     DT_DEP_ORD(DT_NODELABEL(test_children)), "");
	zassert_true(DT_DEP_ORD(DT_NODELABEL(test_irq)) >
		     DT_DEP_ORD(DT_NODELABEL(test_intc)), "");
	zassert_true(DT_DEP_ORD(DT_NODELABEL(test_phandles)) >
		     DT_DEP_ORD(DT_NODELABEL(test_gpio_1)), "");

	/* DT_REQUIRES_DEP_ORDS */
	zassert_equal(ARRAY_SIZE(root_requires), 0, "");
	zassert_true(ORD_IN_ARRAY(root_ord, test_requires), "");

	/* DT_SUPPORTS_DEP_ORDS */
	zassert_true(ORD_IN_ARRAY(test_ord, root_supports), "");
	zassert_false(ORD_IN_ARRAY(root_ord, test_supports), "");

	unsigned int children_combined_ords_expected[] = {
		/*
		 * Combined ordinals for /test/test-children are from
		 * these nodes in this order:
		 */
		DT_DEP_ORD(TEST_CHILDREN),		/* node */
		DT_DEP_ORD(DT_PATH(test)),		/* parent */
		DT_DEP_ORD(DT_NODELABEL(test_child_a)),	/* children */
		DT_DEP_ORD(DT_NODELABEL(test_child_b)),
		DT_DEP_ORD(DT_NODELABEL(test_child_c)),
		ORD_LIST_SEP,				/* separator */
		DT_DEP_ORD(DT_PATH(test)),		/* requires */
		INJECTED_DEP_0,				/* injected
							 * dependencies
							 */
		INJECTED_DEP_1,
		ORD_LIST_SEP,				/* separator */
		DT_DEP_ORD(DT_NODELABEL(test_child_a)),	/* supports */
		DT_DEP_ORD(DT_NODELABEL(test_child_b)),
		DT_DEP_ORD(DT_NODELABEL(test_child_c)),
		ORD_LIST_END,				/* terminator */
	};
	zassert_equal(ARRAY_SIZE(children_combined_ords),
		      ARRAY_SIZE(children_combined_ords_expected),
		      "%zu", ARRAY_SIZE(children_combined_ords));
	for (i = 0; i < ARRAY_SIZE(children_combined_ords); i++) {
		zassert_equal(children_combined_ords[i],
			      children_combined_ords_expected[i],
			      "test-children at %zu", i);
	}

	unsigned int child_a_combined_ords_expected[] = {
		/*
		 * Combined ordinals for /test/test-children/child-a
		 * are from these nodes in this order:
		 */
		DT_DEP_ORD(DT_NODELABEL(test_child_a)), /* node */
		DT_DEP_ORD(TEST_CHILDREN),		/* parent */
		/* children (none) */
		ORD_LIST_SEP,				/* separator */
		DT_DEP_ORD(TEST_CHILDREN),		/* requires */
		INJECTED_DEP_0,				/* injected
							 * dependencies
							 */
		INJECTED_DEP_1,
		ORD_LIST_SEP,				/* separator */
		/* supports (none) */
		ORD_LIST_END,				/* terminator */
	};
	zassert_equal(ARRAY_SIZE(child_a_combined_ords),
		      ARRAY_SIZE(child_a_combined_ords_expected),
		      "%zu", ARRAY_SIZE(child_a_combined_ords));
	for (i = 0; i < ARRAY_SIZE(child_a_combined_ords); i++) {
		zassert_equal(child_a_combined_ords[i],
			      child_a_combined_ords_expected[i],
			      "child-a at %zu", i);
	}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_child_bindings

	/* DT_INST_DEP_ORD */
	zassert_equal(DT_INST_DEP_ORD(0),
		      DT_DEP_ORD(DT_NODELABEL(test_children)), "");

	/* DT_INST_REQUIRES_DEP_ORDS */
	unsigned int inst_requires[] = { DT_INST_REQUIRES_DEP_ORDS(0) };

	zassert_equal(ARRAY_SIZE(inst_requires), 1, "");
	zassert_equal(inst_requires[0], test_ord, "");

	/* DT_INST_SUPPORTS_DEP_ORDS */
	unsigned int inst_supports[] = { DT_INST_SUPPORTS_DEP_ORDS(0) };

	zassert_equal(ARRAY_SIZE(inst_supports), 3, "");
	for (i = 0; i < ARRAY_SIZE(inst_supports); i++) {
		zassert_true(ORD_IN_ARRAY(inst_supports[i], children_ords), "");
	}
}

ZTEST(devicetree_api, test_dep_ord_str_sortable)
{
	const char *root_ord = STRINGIFY(DT_DEP_ORD_STR_SORTABLE(DT_ROOT));

	/* Root ordinal is always 0 */
	zassert_mem_equal(root_ord, "00000", 6);

	/* Test string sortable versions equal decimal values.
	 * We go through the STRINGIFY()->atoi conversion cycle to avoid
	 * the C compiler treating the number as octal due to leading zeros.
	 * atoi() simply ignores them.
	 */
	zassert_equal(atoi(STRINGIFY(DT_DEP_ORD_STR_SORTABLE(DT_ROOT))),
		      DT_DEP_ORD(DT_ROOT), "Invalid sortable string");
	zassert_equal(atoi(STRINGIFY(DT_DEP_ORD_STR_SORTABLE(TEST_DEADBEEF))),
		      DT_DEP_ORD(TEST_DEADBEEF), "Invalid sortable string");
	zassert_equal(atoi(STRINGIFY(DT_DEP_ORD_STR_SORTABLE(TEST_TEMP))),
		      DT_DEP_ORD(TEST_TEMP), "Invalid sortable string");
	zassert_equal(atoi(STRINGIFY(DT_DEP_ORD_STR_SORTABLE(TEST_REG))),
		      DT_DEP_ORD(TEST_REG), "Invalid sortable string");
}

ZTEST(devicetree_api, test_path)
{
	zassert_true(!strcmp(DT_NODE_PATH(DT_ROOT), "/"), "");
	zassert_true(!strcmp(DT_NODE_PATH(TEST_DEADBEEF),
			     "/test/gpio@deadbeef"), "");
}

ZTEST(devicetree_api, test_node_name)
{
	zassert_true(!strcmp(DT_NODE_FULL_NAME(DT_ROOT), "/"), "");
	zassert_true(!strcmp(DT_NODE_FULL_NAME(TEST_DEADBEEF),
			     "gpio@deadbeef"), "");
	zassert_true(!strcmp(DT_NODE_FULL_NAME(TEST_TEMP),
			     "temperature-sensor"), "");
	zassert_true(strcmp(DT_NODE_FULL_NAME(TEST_REG),
			     "reg-holder"), "");
}

ZTEST(devicetree_api, test_node_child_idx)
{
	zassert_equal(DT_NODE_CHILD_IDX(DT_NODELABEL(test_child_a)), 0, "");
	zassert_equal(DT_NODE_CHILD_IDX(DT_NODELABEL(test_child_b)), 1, "");
	zassert_equal(DT_NODE_CHILD_IDX(DT_NODELABEL(test_child_c)), 2, "");
}

ZTEST(devicetree_api, test_same_node)
{
	zassert_true(DT_SAME_NODE(TEST_DEADBEEF, TEST_DEADBEEF), "");
	zassert_false(DT_SAME_NODE(TEST_DEADBEEF, TEST_ABCD1234), "");
}

ZTEST(devicetree_api, test_pinctrl)
{
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
	/*
	 * Tests when a node does have pinctrl properties.
	 */

	/*
	 * node_id versions:
	 */

	zassert_true(DT_SAME_NODE(DT_PINCTRL_BY_IDX(TEST_TEMP, 0, 1),
				  DT_NODELABEL(test_pincfg_b)), "");
	zassert_true(DT_SAME_NODE(DT_PINCTRL_BY_IDX(TEST_TEMP, 1, 0),
				  DT_NODELABEL(test_pincfg_c)), "");

	zassert_true(DT_SAME_NODE(DT_PINCTRL_0(TEST_TEMP, 0),
				  DT_NODELABEL(test_pincfg_a)), "");

	zassert_true(DT_SAME_NODE(DT_PINCTRL_BY_NAME(TEST_TEMP, default, 1),
				  DT_NODELABEL(test_pincfg_b)), "");
	zassert_true(DT_SAME_NODE(DT_PINCTRL_BY_NAME(TEST_TEMP, sleep, 0),
				  DT_NODELABEL(test_pincfg_c)), "");
	zassert_true(DT_SAME_NODE(DT_PINCTRL_BY_NAME(TEST_TEMP, f_o_o2, 0),
				  DT_NODELABEL(test_pincfg_d)), "");

	zassert_equal(DT_PINCTRL_NAME_TO_IDX(TEST_TEMP, default), 0, "");
	zassert_equal(DT_PINCTRL_NAME_TO_IDX(TEST_TEMP, sleep), 1, "");
	zassert_equal(DT_PINCTRL_NAME_TO_IDX(TEST_TEMP, f_o_o2), 2, "");

	zassert_equal(DT_NUM_PINCTRLS_BY_IDX(TEST_TEMP, 0), 2, "");

	zassert_equal(DT_NUM_PINCTRLS_BY_NAME(TEST_TEMP, default), 2, "");
	zassert_equal(DT_NUM_PINCTRLS_BY_NAME(TEST_TEMP, f_o_o2), 1, "");

	zassert_equal(DT_NUM_PINCTRL_STATES(TEST_TEMP), 3, "");

	zassert_equal(DT_PINCTRL_HAS_IDX(TEST_TEMP, 0), 1, "");
	zassert_equal(DT_PINCTRL_HAS_IDX(TEST_TEMP, 1), 1, "");
	zassert_equal(DT_PINCTRL_HAS_IDX(TEST_TEMP, 2), 1, "");
	zassert_equal(DT_PINCTRL_HAS_IDX(TEST_TEMP, 3), 0, "");

	zassert_equal(DT_PINCTRL_HAS_NAME(TEST_TEMP, default), 1, "");
	zassert_equal(DT_PINCTRL_HAS_NAME(TEST_TEMP, sleep), 1, "");
	zassert_equal(DT_PINCTRL_HAS_NAME(TEST_TEMP, f_o_o2), 1, "");
	zassert_equal(DT_PINCTRL_HAS_NAME(TEST_TEMP, bar), 0, "");

#undef MAKE_TOKEN
#define MAKE_TOKEN(pc_idx)						\
	_CONCAT(NODE_ID_ENUM_,						\
		DT_PINCTRL_IDX_TO_NAME_TOKEN(TEST_TEMP, pc_idx))
#undef MAKE_UPPER_TOKEN
#define MAKE_UPPER_TOKEN(pc_idx)					\
	_CONCAT(NODE_ID_ENUM_,						\
		DT_PINCTRL_IDX_TO_NAME_UPPER_TOKEN(TEST_TEMP, pc_idx))
	enum {
		MAKE_TOKEN(0) = 10,
		MAKE_TOKEN(1) = 11,
		MAKE_TOKEN(2) = 12,
		MAKE_TOKEN(3) = 13,

		MAKE_UPPER_TOKEN(0) = 20,
		MAKE_UPPER_TOKEN(1) = 21,
		MAKE_UPPER_TOKEN(2) = 22,
		MAKE_UPPER_TOKEN(3) = 23,
	};

	zassert_equal(NODE_ID_ENUM_default, 10, "");
	zassert_equal(NODE_ID_ENUM_sleep, 11, "");
	zassert_equal(NODE_ID_ENUM_f_o_o2, 12, "");

	zassert_equal(NODE_ID_ENUM_DEFAULT, 20, "");
	zassert_equal(NODE_ID_ENUM_SLEEP, 21, "");
	zassert_equal(NODE_ID_ENUM_F_O_O2, 22, "");

	/*
	 * inst versions:
	 */

	zassert_true(DT_SAME_NODE(DT_INST_PINCTRL_BY_IDX(0, 0, 1),
				  DT_NODELABEL(test_pincfg_b)), "");
	zassert_true(DT_SAME_NODE(DT_INST_PINCTRL_BY_IDX(0, 1, 0),
				  DT_NODELABEL(test_pincfg_c)), "");

	zassert_true(DT_SAME_NODE(DT_INST_PINCTRL_0(0, 0),
				  DT_NODELABEL(test_pincfg_a)), "");

	zassert_true(DT_SAME_NODE(DT_INST_PINCTRL_BY_NAME(0, default, 1),
				  DT_NODELABEL(test_pincfg_b)), "");
	zassert_true(DT_SAME_NODE(DT_INST_PINCTRL_BY_NAME(0, sleep, 0),
				  DT_NODELABEL(test_pincfg_c)), "");
	zassert_true(DT_SAME_NODE(DT_INST_PINCTRL_BY_NAME(0, f_o_o2, 0),
				  DT_NODELABEL(test_pincfg_d)), "");

	zassert_equal(DT_INST_PINCTRL_NAME_TO_IDX(0, default), 0, "");
	zassert_equal(DT_INST_PINCTRL_NAME_TO_IDX(0, sleep), 1, "");
	zassert_equal(DT_INST_PINCTRL_NAME_TO_IDX(0, f_o_o2), 2, "");

	zassert_equal(DT_INST_NUM_PINCTRLS_BY_IDX(0, 0), 2, "");

	zassert_equal(DT_INST_NUM_PINCTRLS_BY_NAME(0, default), 2, "");
	zassert_equal(DT_INST_NUM_PINCTRLS_BY_NAME(0, f_o_o2), 1, "");

	zassert_equal(DT_INST_NUM_PINCTRL_STATES(0), 3, "");

	zassert_equal(DT_INST_PINCTRL_HAS_IDX(0, 0), 1, "");
	zassert_equal(DT_INST_PINCTRL_HAS_IDX(0, 1), 1, "");
	zassert_equal(DT_INST_PINCTRL_HAS_IDX(0, 2), 1, "");
	zassert_equal(DT_INST_PINCTRL_HAS_IDX(0, 3), 0, "");

	zassert_equal(DT_INST_PINCTRL_HAS_NAME(0, default), 1, "");
	zassert_equal(DT_INST_PINCTRL_HAS_NAME(0, sleep), 1, "");
	zassert_equal(DT_INST_PINCTRL_HAS_NAME(0, f_o_o2), 1, "");
	zassert_equal(DT_INST_PINCTRL_HAS_NAME(0, bar), 0, "");

#undef MAKE_TOKEN
#define MAKE_TOKEN(pc_idx)						\
	_CONCAT(INST_ENUM_,						\
			DT_INST_PINCTRL_IDX_TO_NAME_TOKEN(0, pc_idx))
#undef MAKE_UPPER_TOKEN
#define MAKE_UPPER_TOKEN(pc_idx)					\
	_CONCAT(INST_ENUM_,						\
			DT_INST_PINCTRL_IDX_TO_NAME_UPPER_TOKEN(0, pc_idx))
	enum {
		MAKE_TOKEN(0) = 10,
		MAKE_TOKEN(1) = 11,
		MAKE_TOKEN(2) = 12,

		MAKE_UPPER_TOKEN(0) = 20,
		MAKE_UPPER_TOKEN(1) = 21,
		MAKE_UPPER_TOKEN(2) = 22,
	};

	zassert_equal(INST_ENUM_default, 10, "");
	zassert_equal(INST_ENUM_sleep, 11, "");
	zassert_equal(INST_ENUM_f_o_o2, 12, "");

	zassert_equal(INST_ENUM_DEFAULT, 20, "");
	zassert_equal(INST_ENUM_SLEEP, 21, "");
	zassert_equal(INST_ENUM_F_O_O2, 22, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_reg_holder
	/*
	 * Tests when a node does NOT have any pinctrl properties.
	 */

	/* node_id versions */
	zassert_equal(DT_NUM_PINCTRL_STATES(TEST_REG), 0, "");
	zassert_equal(DT_PINCTRL_HAS_IDX(TEST_REG, 0), 0, "");
	zassert_equal(DT_PINCTRL_HAS_NAME(TEST_REG, f_o_o2), 0, "");

	/* inst versions */
	zassert_equal(DT_INST_NUM_PINCTRL_STATES(0), 0, "");
	zassert_equal(DT_INST_PINCTRL_HAS_IDX(0, 0), 0, "");
	zassert_equal(DT_INST_PINCTRL_HAS_NAME(0, f_o_o2), 0, "");
}

DEVICE_DT_DEFINE(DT_NODELABEL(test_mbox), NULL, NULL, NULL, NULL, POST_KERNEL,
		 90, NULL);
DEVICE_DT_DEFINE(DT_NODELABEL(test_mbox_zero_cell), NULL, NULL, NULL, NULL,
		 POST_KERNEL, 90, NULL);

ZTEST(devicetree_api, test_mbox)
{
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor

	const struct mbox_dt_spec channel_tx = MBOX_DT_SPEC_GET(TEST_TEMP, tx);
	const struct mbox_dt_spec channel_rx = MBOX_DT_SPEC_GET(TEST_TEMP, rx);

	zassert_equal(channel_tx.channel_id, 1, "");
	zassert_equal(channel_rx.channel_id, 2, "");

	zassert_equal(DT_MBOX_CHANNEL_BY_NAME(TEST_TEMP, tx), 1, "");
	zassert_equal(DT_MBOX_CHANNEL_BY_NAME(TEST_TEMP, rx), 2, "");

	zassert_true(DT_SAME_NODE(DT_MBOX_CTLR_BY_NAME(TEST_TEMP, tx),
				  DT_NODELABEL(test_mbox)), "");
	zassert_true(DT_SAME_NODE(DT_MBOX_CTLR_BY_NAME(TEST_TEMP, rx),
				  DT_NODELABEL(test_mbox)), "");

	zassert_equal(DT_MBOX_CHANNEL_BY_NAME(TEST_TEMP, tx), 1, "");
	zassert_equal(DT_MBOX_CHANNEL_BY_NAME(TEST_TEMP, rx), 2, "");

	const struct mbox_dt_spec channel_zero = MBOX_DT_SPEC_GET(TEST_TEMP, zero);

	zassert_equal(channel_zero.channel_id, 0, "");

	zassert_equal(DT_MBOX_CHANNEL_BY_NAME(TEST_TEMP, zero), 0, "");

	zassert_true(DT_SAME_NODE(DT_MBOX_CTLR_BY_NAME(TEST_TEMP, zero),
				  DT_NODELABEL(test_mbox_zero_cell)), "");
}

ZTEST(devicetree_api, test_fixed_partitions)
{
	/* Test finding fixed partitions by the 'label' property. */
	zassert_true(DT_HAS_FIXED_PARTITION_LABEL(test_partition_0));
	zassert_true(DT_HAS_FIXED_PARTITION_LABEL(test_partition_1));
	zassert_true(DT_HAS_FIXED_PARTITION_LABEL(test_partition_2));

	zassert_true(DT_SAME_NODE(TEST_PARTITION_0,
				  DT_NODE_BY_FIXED_PARTITION_LABEL(test_partition_0)));
	zassert_true(DT_SAME_NODE(TEST_PARTITION_1,
				  DT_NODE_BY_FIXED_PARTITION_LABEL(test_partition_1)));
	zassert_true(DT_SAME_NODE(TEST_PARTITION_2,
				  DT_NODE_BY_FIXED_PARTITION_LABEL(test_partition_2)));

	zassert_true(DT_FIXED_PARTITION_EXISTS(TEST_PARTITION_0));
	zassert_true(DT_FIXED_PARTITION_EXISTS(TEST_PARTITION_1));
	zassert_true(DT_FIXED_PARTITION_EXISTS(TEST_PARTITION_2));

	/* There should not be a node with `label = "test_partition_3"`. */
	zassert_false(DT_HAS_FIXED_PARTITION_LABEL(test_partition_3));
	zassert_false(DT_NODE_EXISTS(DT_NODE_BY_FIXED_PARTITION_LABEL(test_partition_3)));

	/* There is a node with `label = "FOO"`, but it is not a fixed partition. */
	zassert_false(DT_HAS_FIXED_PARTITION_LABEL(FOO));
	zassert_false(DT_NODE_EXISTS(DT_NODE_BY_FIXED_PARTITION_LABEL(FOO)));

	/* Test DT_MTD_FROM_FIXED_PARTITION. */
	zassert_true(DT_NODE_EXISTS(DT_MTD_FROM_FIXED_PARTITION(TEST_PARTITION_0)));
	zassert_true(DT_NODE_EXISTS(DT_MTD_FROM_FIXED_PARTITION(TEST_PARTITION_1)));
	zassert_true(DT_NODE_EXISTS(DT_MTD_FROM_FIXED_PARTITION(TEST_PARTITION_2)));

	zassert_true(DT_SAME_NODE(TEST_MTD_0, DT_MTD_FROM_FIXED_PARTITION(TEST_PARTITION_0)));
	zassert_true(DT_SAME_NODE(TEST_MTD_0, DT_MTD_FROM_FIXED_PARTITION(TEST_PARTITION_1)));
	zassert_true(DT_SAME_NODE(TEST_MTD_1, DT_MTD_FROM_FIXED_PARTITION(TEST_PARTITION_2)));

	/* Test DT_MEM_FROM_FIXED_PARTITION. */
	zassert_true(DT_NODE_EXISTS(DT_MEM_FROM_FIXED_PARTITION(TEST_PARTITION_0)));
	zassert_true(DT_NODE_EXISTS(DT_MEM_FROM_FIXED_PARTITION(TEST_PARTITION_1)));
	zassert_false(DT_NODE_EXISTS(DT_MEM_FROM_FIXED_PARTITION(TEST_PARTITION_2)));

	zassert_true(DT_SAME_NODE(TEST_MEM_0, DT_MEM_FROM_FIXED_PARTITION(TEST_PARTITION_0)));
	zassert_true(DT_SAME_NODE(TEST_MEM_0, DT_MEM_FROM_FIXED_PARTITION(TEST_PARTITION_1)));

	/* Test DT_FIXED_PARTITION_ADDR. */
	zassert_equal(DT_FIXED_PARTITION_ADDR(TEST_PARTITION_0), 0x20000000);
	zassert_equal(DT_FIXED_PARTITION_ADDR(TEST_PARTITION_1), 0x200000c0);

	/* DT_FIXED_PARTITION_ADDR(TEST_PARTITION_2) expands to an invalid expression.
	 * Test this by way of string comparison.
	 */
	zassert_true(!strcmp(TO_STRING(DT_FIXED_PARTITION_ADDR(TEST_PARTITION_2)),
			     "(__REG_IDX_0_VAL_ADDRESS + 458624)"));
	zassert_equal(DT_REG_ADDR(TEST_PARTITION_2), 458624);

	/* Test that all DT_FIXED_PARTITION_ID are defined and unique. */
#define FIXED_PARTITION_ID_COMMA(node_id) DT_FIXED_PARTITION_ID(node_id),

	static const int ids[] = {
		DT_FOREACH_STATUS_OKAY_VARGS(fixed_partitions, DT_FOREACH_CHILD,
					     FIXED_PARTITION_ID_COMMA)
	};
	bool found[ARRAY_SIZE(ids)] = { false };

	for (int i = 0; i < ARRAY_SIZE(ids); i++) {
		zassert_between_inclusive(ids[i], 0, ARRAY_SIZE(ids) - 1, "");
		zassert_false(found[ids[i]]);
		found[ids[i]] = true;
	}

#undef FIXED_PARTITION_ID_COMMA
}

ZTEST(devicetree_api, test_string_token)
{
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_string_token
	enum {
		token_zero,
		token_one,
		token_two,
		token_no_inst,
	};
	enum {
		TOKEN_ZERO = token_no_inst + 1,
		TOKEN_ONE,
		TOKEN_TWO,
		TOKEN_NO_INST,
	};
	int i;

	/* Test DT_INST_STRING_TOKEN */
#define STRING_TOKEN_TEST_INST_EXPANSION(inst) \
	DT_INST_STRING_TOKEN(inst, val),
	int array_inst[] = {
		DT_INST_FOREACH_STATUS_OKAY(STRING_TOKEN_TEST_INST_EXPANSION)
	};

	for (i = 0; i < ARRAY_SIZE(array_inst); i++) {
		zassert_between_inclusive(array_inst[i], token_zero, token_two, "");
	}

	/* Test DT_INST_STRING_UPPER_TOKEN */
#define STRING_UPPER_TOKEN_TEST_INST_EXPANSION(inst) \
	DT_INST_STRING_UPPER_TOKEN(inst, val),
	int array_inst_upper[] = {
		DT_INST_FOREACH_STATUS_OKAY(STRING_UPPER_TOKEN_TEST_INST_EXPANSION)
	};

	for (i = 0; i < ARRAY_SIZE(array_inst_upper); i++) {
		zassert_between_inclusive(array_inst_upper[i], TOKEN_ZERO, TOKEN_TWO, "");
	}

	/* Test DT_INST_STRING_TOKEN_OR when property is found */
#define STRING_TOKEN_OR_TEST_INST_EXPANSION(inst) \
	DT_INST_STRING_TOKEN_OR(inst, val, token_no_inst),
	int array_inst_or[] = {
		DT_INST_FOREACH_STATUS_OKAY(STRING_TOKEN_OR_TEST_INST_EXPANSION)
	};

	for (i = 0; i < ARRAY_SIZE(array_inst_or); i++) {
		zassert_between_inclusive(array_inst_or[i], token_zero, token_two, "");
	}

	/* Test DT_INST_STRING_UPPER_TOKEN_OR when property is found */
#define STRING_UPPER_TOKEN_OR_TEST_INST_EXPANSION(inst)	\
	DT_INST_STRING_UPPER_TOKEN_OR(inst, val, TOKEN_NO_INST),
	int array_inst_upper_or[] = {
		DT_INST_FOREACH_STATUS_OKAY(STRING_UPPER_TOKEN_OR_TEST_INST_EXPANSION)
	};

	for (i = 0; i < ARRAY_SIZE(array_inst_upper_or); i++) {
		zassert_between_inclusive(array_inst_upper_or[i], TOKEN_ZERO, TOKEN_TWO, "");
	}

	/* Test DT_STRING_TOKEN_OR when property is found */
	zassert_equal(DT_STRING_TOKEN_OR(DT_NODELABEL(test_string_token_0),
					 val, token_one), token_zero, "");
	zassert_equal(DT_STRING_TOKEN_OR(DT_NODELABEL(test_string_token_1),
					 val, token_two), token_one, "");

	/* Test DT_STRING_TOKEN_OR is not found */
	zassert_equal(DT_STRING_TOKEN_OR(DT_NODELABEL(test_string_token_1),
					 no_inst, token_zero), token_zero, "");

	/* Test DT_STRING_UPPER_TOKEN_OR when property is found */
	zassert_equal(DT_STRING_UPPER_TOKEN_OR(DT_NODELABEL(test_string_token_0),
					       val, TOKEN_ONE), TOKEN_ZERO, "");
	zassert_equal(DT_STRING_UPPER_TOKEN_OR(DT_NODELABEL(test_string_token_1),
					       val, TOKEN_TWO), TOKEN_ONE, "");

	/* Test DT_STRING_UPPER_TOKEN_OR is not found */
	zassert_equal(DT_STRING_UPPER_TOKEN_OR(DT_NODELABEL(test_string_token_1),
					       no_inst, TOKEN_ZERO), TOKEN_ZERO, "");

	/* Test DT_INST_STRING_TOKEN_OR when property is not found */
#define STRING_TOKEN_TEST_NO_INST_EXPANSION(inst) \
	DT_INST_STRING_TOKEN_OR(inst, no_inst, token_no_inst),
	int array_no_inst_or[] = {
		DT_INST_FOREACH_STATUS_OKAY(STRING_TOKEN_TEST_NO_INST_EXPANSION)
	};
	for (i = 0; i < ARRAY_SIZE(array_no_inst_or); i++) {
		zassert_equal(array_no_inst_or[i], token_no_inst, "");
	}

	/* Test DT_INST_STRING_UPPER_TOKEN_OR when property is not found */
#define STRING_UPPER_TOKEN_TEST_NO_INST_EXPANSION(inst)	\
	DT_INST_STRING_UPPER_TOKEN_OR(inst, no_inst, TOKEN_NO_INST),
	int array_no_inst_upper_or[] = {
		DT_INST_FOREACH_STATUS_OKAY(STRING_UPPER_TOKEN_TEST_NO_INST_EXPANSION)
	};
	for (i = 0; i < ARRAY_SIZE(array_no_inst_upper_or); i++) {
		zassert_equal(array_no_inst_upper_or[i], TOKEN_NO_INST, "");
	}
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_string_array_token
ZTEST(devicetree_api, test_string_idx_token)
{
	enum token_string_idx {
		/* Tokens */
		token_first_idx_zero,
		token_first_idx_one,
		token_first_idx_two,
		token_second_idx_zero,
		token_second_idx_one,
		token_second_idx_two,
		token_second_idx_three,
		/* Upper tokens */
		TOKEN_FIRST_IDX_ZERO,
		TOKEN_FIRST_IDX_ONE,
		TOKEN_FIRST_IDX_TWO,
		TOKEN_SECOND_IDX_ZERO,
		TOKEN_SECOND_IDX_ONE,
		TOKEN_SECOND_IDX_TWO,
		TOKEN_SECOND_IDX_THREE
	};

	/* Test direct idx access */
	zassert_equal(DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_0), val, 0),
			token_first_idx_zero, "");
	zassert_equal(DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_0), val, 1),
			token_first_idx_one, "");
	zassert_equal(DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_0), val, 2),
			token_first_idx_two, "");
	zassert_equal(DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_1), val, 0),
			token_second_idx_zero, "");
	zassert_equal(DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_1), val, 1),
			token_second_idx_one, "");
	zassert_equal(DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_1), val, 2),
			token_second_idx_two, "");
	zassert_equal(DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_1), val, 3),
			token_second_idx_three, "");

	zassert_equal(DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_0), val, 0),
			TOKEN_FIRST_IDX_ZERO, "");
	zassert_equal(DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_0), val, 1),
			TOKEN_FIRST_IDX_ONE, "");
	zassert_equal(DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_0), val, 2),
			TOKEN_FIRST_IDX_TWO, "");
	zassert_equal(DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_1), val, 0),
			TOKEN_SECOND_IDX_ZERO, "");
	zassert_equal(DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_1), val, 1),
			TOKEN_SECOND_IDX_ONE, "");
	zassert_equal(DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_1), val, 2),
			TOKEN_SECOND_IDX_TWO, "");
	zassert_equal(DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(test_str_array_token_1), val, 3),
			TOKEN_SECOND_IDX_THREE, "");

	/* Test instances */
#define STRING_TOKEN_BY_IDX_VAR(node_id) _CONCAT(var_token_, node_id)
#define STRING_TOKEN_BY_IDX_TEST_INST_EXPANSION(inst) \
	enum token_string_idx STRING_TOKEN_BY_IDX_VAR(DT_DRV_INST(inst))[] = { \
		DT_INST_STRING_TOKEN_BY_IDX(inst, val, 0), \
		DT_INST_STRING_TOKEN_BY_IDX(inst, val, 1), \
		DT_INST_STRING_TOKEN_BY_IDX(inst, val, 2)  \
	};
	DT_INST_FOREACH_STATUS_OKAY(STRING_TOKEN_BY_IDX_TEST_INST_EXPANSION);

	zassert_equal(STRING_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_0))[0],
			token_first_idx_zero, "");
	zassert_equal(STRING_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_0))[1],
			token_first_idx_one, "");
	zassert_equal(STRING_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_0))[2],
			token_first_idx_two, "");
	zassert_equal(STRING_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_1))[0],
			token_second_idx_zero, "");
	zassert_equal(STRING_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_1))[1],
			token_second_idx_one, "");
	zassert_equal(STRING_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_1))[2],
			token_second_idx_two, "");

#define STRING_UPPER_TOKEN_BY_IDX_VAR(node_id) _CONCAT(var_upper_token, node_id)
#define STRING_UPPER_TOKEN_BY_IDX_TEST_INST_EXPANSION(inst) \
	enum token_string_idx STRING_UPPER_TOKEN_BY_IDX_VAR(DT_DRV_INST(inst))[] = { \
		DT_INST_STRING_UPPER_TOKEN_BY_IDX(inst, val, 0), \
		DT_INST_STRING_UPPER_TOKEN_BY_IDX(inst, val, 1), \
		DT_INST_STRING_UPPER_TOKEN_BY_IDX(inst, val, 2)  \
	};
	DT_INST_FOREACH_STATUS_OKAY(STRING_UPPER_TOKEN_BY_IDX_TEST_INST_EXPANSION);

	zassert_equal(STRING_UPPER_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_0))[0],
			TOKEN_FIRST_IDX_ZERO, "");
	zassert_equal(STRING_UPPER_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_0))[1],
			TOKEN_FIRST_IDX_ONE, "");
	zassert_equal(STRING_UPPER_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_0))[2],
			TOKEN_FIRST_IDX_TWO, "");
	zassert_equal(STRING_UPPER_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_1))[0],
			TOKEN_SECOND_IDX_ZERO, "");
	zassert_equal(STRING_UPPER_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_1))[1],
			TOKEN_SECOND_IDX_ONE, "");
	zassert_equal(STRING_UPPER_TOKEN_BY_IDX_VAR(DT_NODELABEL(test_str_array_token_1))[2],
			TOKEN_SECOND_IDX_TWO, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_string_unquoted
ZTEST(devicetree_api, test_string_unquoted)
{
#define XA 12.0
#define XB 34.0
#define XPLUS +
	const double f0_expected = 0.1234;
	const double f1_expected = 0.9e-3;
	const double delta = 0.1e-4;

	/* Test DT_STRING_UNQUOTED */
	zassert_within(DT_STRING_UNQUOTED(DT_NODELABEL(test_str_unquoted_f0), val),
		       f0_expected, delta, "");
	zassert_within(DT_STRING_UNQUOTED(DT_NODELABEL(test_str_unquoted_f1), val),
		       f1_expected, delta, "");
	zassert_within(DT_STRING_UNQUOTED(DT_NODELABEL(test_str_unquoted_t), val),
		       XA XPLUS XB, delta, "");
	/* Test DT_STRING_UNQUOTED_OR */
	zassert_within(DT_STRING_UNQUOTED_OR(DT_NODELABEL(test_str_unquoted_f0), val, (0.0)),
		       f0_expected, delta, "");
	zassert_within(DT_STRING_UNQUOTED_OR(DT_NODELABEL(test_str_unquoted_f1), val, (0.0)),
		       f1_expected, delta, "");
	zassert_within(DT_STRING_UNQUOTED_OR(DT_NODELABEL(test_str_unquoted_t), val, (0.0)),
		       XA XPLUS XB, delta, "");
	zassert_within(DT_STRING_UNQUOTED_OR(DT_NODELABEL(test_str_unquoted_f0), nak, (0.0)),
		       0.0, delta, "");
	zassert_within(DT_STRING_UNQUOTED_OR(DT_NODELABEL(test_str_unquoted_f1), nak, (0.0)),
		       0.0, delta, "");
	zassert_within(DT_STRING_UNQUOTED_OR(DT_NODELABEL(test_str_unquoted_t), nak, (0.0)),
		       0.0, delta, "");
	/* Test DT_INST_STRING_UNQUOTED */
#define STRING_UNQUOTED_VAR(node_id) _CONCAT(var_, node_id)
#define STRING_UNQUOTED_TEST_INST_EXPANSION(inst) \
	double STRING_UNQUOTED_VAR(DT_DRV_INST(inst)) = DT_INST_STRING_UNQUOTED(inst, val);
	DT_INST_FOREACH_STATUS_OKAY(STRING_UNQUOTED_TEST_INST_EXPANSION);

	zassert_within(STRING_UNQUOTED_VAR(DT_NODELABEL(test_str_unquoted_f0)),
		       f0_expected, delta, "");
	zassert_within(STRING_UNQUOTED_VAR(DT_NODELABEL(test_str_unquoted_f1)),
		       f1_expected, delta, "");
	zassert_within(STRING_UNQUOTED_VAR(DT_NODELABEL(test_str_unquoted_t)), XA XPLUS XB,
		       delta, "");

	/* Test DT_INST_STRING_UNQUOTED_OR */
#define STRING_UNQUOTED_OR_VAR(node_id) _CONCAT(var_or_, node_id)
#define STRING_UNQUOTED_OR_TEST_INST_EXPANSION(inst)       \
	double STRING_UNQUOTED_OR_VAR(DT_DRV_INST(inst))[2] = { \
		DT_INST_STRING_UNQUOTED_OR(inst, val, (1.0e10)),   \
		DT_INST_STRING_UNQUOTED_OR(inst, dummy, (1.0e10))  \
	};
	DT_INST_FOREACH_STATUS_OKAY(STRING_UNQUOTED_OR_TEST_INST_EXPANSION);

	zassert_within(STRING_UNQUOTED_OR_VAR(DT_NODELABEL(test_str_unquoted_f0))[0],
		       f0_expected, delta, "");
	zassert_within(STRING_UNQUOTED_OR_VAR(DT_NODELABEL(test_str_unquoted_f1))[0],
		       f1_expected, delta, "");
	zassert_within(STRING_UNQUOTED_OR_VAR(DT_NODELABEL(test_str_unquoted_t))[0],
		       XA XPLUS XB, delta, "");
	zassert_within(STRING_UNQUOTED_OR_VAR(DT_NODELABEL(test_str_unquoted_f0))[1],
		       1.0e10, delta, "");
	zassert_within(STRING_UNQUOTED_OR_VAR(DT_NODELABEL(test_str_unquoted_f1))[1],
		       1.0e10, delta, "");
	zassert_within(STRING_UNQUOTED_OR_VAR(DT_NODELABEL(test_str_unquoted_t))[1],
		       1.0e10, delta, "");
#undef XA
#undef XB
#undef XPLUS
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_string_array_unquoted
ZTEST(devicetree_api, test_string_idx_unquoted)
{
#define XA 12.0
#define XB 34.0
#define XC 56.0
#define XD 78.0
#define XPLUS +
#define XMINUS -
	const double delta = 0.1e-4;

	/* DT_STRING_UNQUOTED_BY_IDX */
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_f0), val, 0),
		       1.0e2, delta, "");
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_f0), val, 1),
		       2.0e2, delta, "");
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_f0), val, 2),
		       3.0e2, delta, "");
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_f0), val, 3),
		       4.0e2, delta, "");

	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_f1), val, 0),
		       0.01, delta, "");
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_f1), val, 1),
		       0.1,  delta, "");
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_f1), val, 2),
		       1.0,  delta, "");
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_f1), val, 3),
		       10.0, delta, "");

	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_t), val, 0),
		       XA XPLUS XB, delta, "");
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_t), val, 1),
		       XC XPLUS XD,  delta, "");
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_t), val, 2),
		       XA XMINUS XB,  delta, "");
	zassert_within(DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(test_stra_unquoted_t), val, 3),
		       XC XMINUS XD, delta, "");

#define STRING_UNQUOTED_BY_IDX_VAR(node_id) _CONCAT(var_, node_id)
#define STRING_UNQUOTED_BY_IDX_TEST_INST_EXPANSION(inst)       \
	double STRING_UNQUOTED_BY_IDX_VAR(DT_DRV_INST(inst))[] = { \
		DT_INST_STRING_UNQUOTED_BY_IDX(inst, val, 0),          \
		DT_INST_STRING_UNQUOTED_BY_IDX(inst, val, 1),          \
		DT_INST_STRING_UNQUOTED_BY_IDX(inst, val, 2),          \
		DT_INST_STRING_UNQUOTED_BY_IDX(inst, val, 3),          \
	};
	DT_INST_FOREACH_STATUS_OKAY(STRING_UNQUOTED_BY_IDX_TEST_INST_EXPANSION);

	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_f0))[0],
		       1.0e2, delta, "");
	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_f0))[1],
		       2.0e2, delta, "");
	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_f0))[2],
		       3.0e2, delta, "");
	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_f0))[3],
		       4.0e2, delta, "");

	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_f1))[0],
		       0.01, delta, "");
	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_f1))[1],
		       0.1,  delta, "");
	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_f1))[2],
		       1.0,  delta, "");
	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_f1))[3],
		       10.0, delta, "");

	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_t))[0],
		       XA XPLUS XB, delta, "");
	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_t))[1],
		       XC XPLUS XD,  delta, "");
	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_t))[2],
		       XA XMINUS XB,  delta, "");
	zassert_within(STRING_UNQUOTED_BY_IDX_VAR(DT_NODELABEL(test_stra_unquoted_t))[3],
		       XC XMINUS XD, delta, "");
#undef XA
#undef XB
#undef XC
#undef XD
#undef XPLUS
#undef XMINUS
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
ZTEST(devicetree_api, test_reset)
{
	/* DT_RESET_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_RESET_CTLR_BY_IDX(TEST_TEMP, 1),
				  DT_NODELABEL(test_reset)), "");

	/* DT_RESET_CTLR */
	zassert_true(DT_SAME_NODE(DT_RESET_CTLR(TEST_TEMP),
				  DT_NODELABEL(test_reset)), "");

	/* DT_RESET_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_RESET_CTLR_BY_NAME(TEST_TEMP, reset_b),
				  DT_NODELABEL(test_reset)), "");

	/* DT_RESET_CELL_BY_IDX */
	zassert_equal(DT_RESET_CELL_BY_IDX(TEST_TEMP, 1, id), 20, "");
	zassert_equal(DT_RESET_CELL_BY_IDX(TEST_TEMP, 0, id), 10, "");

	/* DT_RESET_CELL_BY_NAME */
	zassert_equal(DT_RESET_CELL_BY_NAME(TEST_TEMP, reset_a, id), 10, "");
	zassert_equal(DT_RESET_CELL_BY_NAME(TEST_TEMP, reset_b, id), 20, "");

	/* DT_RESET_CELL */
	zassert_equal(DT_RESET_CELL(TEST_TEMP, id), 10, "");

	/* reg-width on reset */
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_TEMP, resets, 1, reg_width), 4, "");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	/* DT_INST_RESET_CTLR_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_INST_RESET_CTLR_BY_IDX(0, 1),
				  DT_NODELABEL(test_reset)), "");

	/* DT_INST_RESET_CTLR */
	zassert_true(DT_SAME_NODE(DT_INST_RESET_CTLR(0),
				  DT_NODELABEL(test_reset)), "");

	/* DT_INST_RESET_CTLR_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_INST_RESET_CTLR_BY_NAME(0, reset_b),
				  DT_NODELABEL(test_reset)), "");

	/* DT_INST_RESET_CELL_BY_IDX */
	zassert_equal(DT_INST_RESET_CELL_BY_IDX(0, 1, id), 20, "");
	zassert_equal(DT_INST_RESET_CELL_BY_IDX(0, 0, id), 10, "");

	/* DT_INST_RESET_CELL_BY_NAME */
	zassert_equal(DT_INST_RESET_CELL_BY_NAME(0, reset_a, id), 10, "");
	zassert_equal(DT_INST_RESET_CELL_BY_NAME(0, reset_b, id), 20, "");

	/* DT_INST_RESET_CELL */
	zassert_equal(DT_INST_RESET_CELL(0, id), 10, "");

	/* reg-width on reset */
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, resets, 1, reg_width), 4, "");

	/* DT_RESET_ID_BY_IDX */
	zassert_equal(DT_RESET_ID_BY_IDX(TEST_TEMP, 0), 10, "");
	zassert_equal(DT_RESET_ID_BY_IDX(TEST_TEMP, 1), 20, "");

	/* DT_RESET_ID */
	zassert_equal(DT_RESET_ID(TEST_TEMP), 10, "");

	/* DT_INST_RESET_ID_BY_IDX */
	zassert_equal(DT_INST_RESET_ID_BY_IDX(0, 0), 10, "");
	zassert_equal(DT_INST_RESET_ID_BY_IDX(0, 1), 20, "");

	/* DT_INST_RESET_ID */
	zassert_equal(DT_INST_RESET_ID(0), 10, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_interrupt_holder_extended
ZTEST(devicetree_api, test_interrupt_controller)
{
	/* DT_IRQ_INTC_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_IRQ_INTC_BY_IDX(TEST_IRQ_EXT, 0), TEST_INTC), "");
	zassert_true(DT_SAME_NODE(DT_IRQ_INTC_BY_IDX(TEST_IRQ_EXT, 1), TEST_GPIO_4), "");

	/* DT_IRQ_INTC_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_IRQ_INTC_BY_NAME(TEST_IRQ_EXT, int1), TEST_INTC), "");
	zassert_true(DT_SAME_NODE(DT_IRQ_INTC_BY_NAME(TEST_IRQ_EXT, int2), TEST_GPIO_4), "");

	/* DT_IRQ_INTC */
	zassert_true(DT_SAME_NODE(DT_IRQ_INTC(TEST_IRQ_EXT), TEST_INTC), "");

	/* DT_INST_IRQ_INTC_BY_IDX */
	zassert_true(DT_SAME_NODE(DT_INST_IRQ_INTC_BY_IDX(0, 0), TEST_INTC), "");
	zassert_true(DT_SAME_NODE(DT_INST_IRQ_INTC_BY_IDX(0, 1), TEST_GPIO_4), "");

	/* DT_INST_IRQ_INTC_BY_NAME */
	zassert_true(DT_SAME_NODE(DT_INST_IRQ_INTC_BY_NAME(0, int1), TEST_INTC), "");
	zassert_true(DT_SAME_NODE(DT_INST_IRQ_INTC_BY_NAME(0, int2), TEST_GPIO_4), "");

	/* DT_INST_IRQ_INTC */
	zassert_true(DT_SAME_NODE(DT_INST_IRQ_INTC(0), TEST_INTC), "");
}

ZTEST_SUITE(devicetree_api, NULL, NULL, NULL, NULL, NULL);
