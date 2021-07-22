/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Override __DEPRECATED_MACRO so we don't get twister failures for
 * deprecated macros:
 * - DT_CLOCKS_LABEL_BY_IDX
 * - DT_CLOCKS_LABEL_BY_NAME
 * - DT_CLOCKS_LABEL
 * - DT_PWMS_LABEL_BY_IDX
 * - DT_PWMS_LABEL_BY_NAME
 * - DT_PWMS_LABEL
 * - DT_IO_CHANNELS_LABEL_BY_IDX
 * - DT_IO_CHANNELS_LABEL_BY_NAME
 * - DT_IO_CHANNELS_LABEL
 * - DT_DMAS_LABEL_BY_IDX
 * - DT_DMAS_LABEL_BY_NAME
 * - DT_INST_CLOCKS_LABEL_BY_IDX
 * - DT_INST_CLOCKS_LABEL_BY_NAME
 * - DT_INST_CLOCKS_LABEL
 * - DT_INST_PWMS_LABEL_BY_IDX
 * - DT_INST_PWMS_LABEL_BY_NAME
 * - DT_INST_PWMS_LABEL
 * - DT_INST_IO_CHANNELS_LABEL_BY_IDX
 * - DT_INST_IO_CHANNELS_LABEL_BY_NAME
 * - DT_INST_IO_CHANNELS_LABEL
 * - DT_INST_DMAS_LABEL_BY_IDX
 * - DT_INST_DMAS_LABEL_BY_NAME
 * - DT_ENUM_TOKEN
 * - DT_ENUM_UPPER_TOKEN
 */
#define __DEPRECATED_MACRO

#include <ztest.h>
#include <devicetree.h>
#include <device.h>
#include <drivers/gpio.h>

#define TEST_CHILDREN	DT_PATH(test, test_children)
#define TEST_DEADBEEF	DT_PATH(test, gpio_deadbeef)
#define TEST_ABCD1234	DT_PATH(test, gpio_abcd1234)
#define TEST_ALIAS	DT_ALIAS(test_alias)
#define TEST_NODELABEL	DT_NODELABEL(test_nodelabel)
#define TEST_INST	DT_INST(0, vnd_gpio)
#define TEST_ARRAYS	DT_NODELABEL(test_arrays)
#define TEST_PH	DT_NODELABEL(test_phandles)
#define TEST_IRQ	DT_NODELABEL(test_irq)
#define TEST_TEMP	DT_NODELABEL(test_temp_sensor)
#define TEST_REG	DT_NODELABEL(test_reg)
#define TEST_ENUM_0	DT_NODELABEL(test_enum_0)

#define TEST_I2C_DEV DT_PATH(test, i2c_11112222, test_i2c_dev_10)
#define TEST_I2C_BUS DT_BUS(TEST_I2C_DEV)

#define TEST_I2C_MUX DT_NODELABEL(test_i2c_mux)
#define TEST_I2C_MUX_CTLR_1 DT_CHILD(TEST_I2C_MUX, i2c_mux_ctlr_1)
#define TEST_I2C_MUX_CTLR_2 DT_CHILD(TEST_I2C_MUX, i2c_mux_ctlr_2)
#define TEST_MUXED_I2C_DEV_1 DT_NODELABEL(test_muxed_i2c_dev_1)
#define TEST_MUXED_I2C_DEV_2 DT_NODELABEL(test_muxed_i2c_dev_2)

#define TEST_SPI DT_NODELABEL(test_spi)

#define TEST_SPI_DEV_0 DT_PATH(test, spi_33334444, test_spi_dev_0)
#define TEST_SPI_BUS_0 DT_BUS(TEST_SPI_DEV_0)

#define TEST_SPI_DEV_1 DT_PATH(test, spi_33334444, test_spi_dev_1)
#define TEST_SPI_BUS_1 DT_BUS(TEST_SPI_DEV_1)

#define TEST_SPI_NO_CS DT_NODELABEL(test_spi_no_cs)
#define TEST_SPI_DEV_NO_CS DT_NODELABEL(test_spi_no_cs)

#define TEST_PWM_CTLR_1 DT_NODELABEL(test_pwm1)
#define TEST_PWM_CTLR_2 DT_NODELABEL(test_pwm2)

#define TEST_DMA_CTLR_1 DT_NODELABEL(test_dma1)
#define TEST_DMA_CTLR_2 DT_NODELABEL(test_dma2)

#define TEST_IO_CHANNEL_CTLR_1 DT_NODELABEL(test_adc_1)
#define TEST_IO_CHANNEL_CTLR_2 DT_NODELABEL(test_adc_2)

#define TA_HAS_COMPAT(compat) DT_NODE_HAS_COMPAT(TEST_ARRAYS, compat)

#define TO_STRING(x) TO_STRING_(x)
#define TO_STRING_(x) #x

static void test_path_props(void)
{
	zassert_true(!strcmp(DT_LABEL(TEST_DEADBEEF), "TEST_GPIO_1"), "");
	zassert_equal(DT_NUM_REGS(TEST_DEADBEEF), 1, "");
	zassert_equal(DT_REG_ADDR(TEST_DEADBEEF), 0xdeadbeef, "");
	zassert_equal(DT_REG_SIZE(TEST_DEADBEEF), 0x1000, "");
	zassert_equal(DT_PROP(TEST_DEADBEEF, gpio_controller), 1, "");
	zassert_equal(DT_PROP(TEST_DEADBEEF, ngpios), 32, "");
	zassert_true(!strcmp(DT_PROP(TEST_DEADBEEF, status), "okay"), "");
	zassert_equal(DT_PROP_LEN(TEST_DEADBEEF, compatible), 1, "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_DEADBEEF, compatible, 0),
			     "vnd,gpio"), "");
	zassert_true(DT_NODE_HAS_PROP(TEST_DEADBEEF, status), "");
	zassert_false(DT_NODE_HAS_PROP(TEST_DEADBEEF, foobar), "");

	zassert_true(!strcmp(DT_LABEL(TEST_ABCD1234), "TEST_GPIO_2"), "");
	zassert_equal(DT_NUM_REGS(TEST_ABCD1234), 2, "");
	zassert_equal(DT_PROP(TEST_ABCD1234, gpio_controller), 1, "");
	zassert_equal(DT_PROP(TEST_ABCD1234, ngpios), 32, "");
	zassert_true(!strcmp(DT_PROP(TEST_ABCD1234, status), "okay"), "");
	zassert_equal(DT_PROP_LEN(TEST_ABCD1234, compatible), 1, "");
	zassert_equal(DT_PROP_LEN_OR(TEST_ABCD1234, compatible, 4), 1, "");
	zassert_equal(DT_PROP_LEN_OR(TEST_ABCD1234, invalid_property, 0), 0, "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ABCD1234, compatible, 0),
			     "vnd,gpio"), "");
}

static void test_alias_props(void)
{
	zassert_equal(DT_NUM_REGS(TEST_ALIAS), 1, "");
	zassert_equal(DT_REG_ADDR(TEST_ALIAS), 0xdeadbeef, "");
	zassert_equal(DT_REG_SIZE(TEST_ALIAS), 0x1000, "");
	zassert_true(!strcmp(DT_LABEL(TEST_ALIAS), "TEST_GPIO_1"), "");
	zassert_equal(DT_PROP(TEST_ALIAS, gpio_controller), 1, "");
	zassert_equal(DT_PROP(TEST_ALIAS, ngpios), 32, "");
	zassert_true(!strcmp(DT_PROP(TEST_ALIAS, status), "okay"), "");
	zassert_equal(DT_PROP_LEN(TEST_ALIAS, compatible), 1, "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ALIAS, compatible, 0),
			     "vnd,gpio"), "");
}

static void test_nodelabel_props(void)
{
	zassert_equal(DT_NUM_REGS(TEST_NODELABEL), 1, "");
	zassert_equal(DT_REG_ADDR(TEST_NODELABEL), 0xdeadbeef, "");
	zassert_equal(DT_REG_SIZE(TEST_NODELABEL), 0x1000, "");
	zassert_true(!strcmp(DT_LABEL(TEST_NODELABEL), "TEST_GPIO_1"), "");
	zassert_equal(DT_PROP(TEST_NODELABEL, gpio_controller), 1, "");
	zassert_equal(DT_PROP(TEST_NODELABEL, ngpios), 32, "");
	zassert_true(!strcmp(DT_PROP(TEST_NODELABEL, status), "okay"), "");
	zassert_equal(DT_PROP_LEN(TEST_NODELABEL, compatible), 1, "");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_NODELABEL, compatible, 0),
			     "vnd,gpio"), "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_gpio
static void test_inst_props(void)
{
	const char *label_startswith = "TEST_GPIO_";

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
			     "vnd,gpio"), "");

	zassert_equal(DT_INST_NODE_HAS_PROP(0, gpio_controller), 1, "");
	zassert_equal(DT_INST_PROP(0, gpio_controller), 1, "");
	zassert_equal(DT_INST_NODE_HAS_PROP(0, xxxx), 0, "");
	zassert_true(!strcmp(DT_INST_PROP(0, status), "okay") ||
		     !strcmp(DT_PROP(TEST_INST, status), "disabled"), "");
	zassert_equal(DT_INST_PROP_LEN(0, compatible), 1, "");
	zassert_true(!strcmp(DT_INST_PROP_BY_IDX(0, compatible, 0),
			     "vnd,gpio"), "");
	zassert_true(!strncmp(label_startswith, DT_INST_LABEL(0),
			      strlen(label_startswith)), "");
}

static void test_default_prop_access(void)
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

static void test_has_path(void)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_PATH(test, gpio_0), okay), 0, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_PATH(test, gpio_deadbeef), okay), 1,
		      "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_PATH(test, gpio_abcd1234), okay), 1,
		      "");
}

static void test_has_alias(void)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_ALIAS(test_alias), okay), 1, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_ALIAS(test_undef), okay), 0, "");
}

static void test_inst_checks(void)
{
	zassert_equal(DT_NODE_EXISTS(DT_INST(0, vnd_gpio)), 1, "");
	zassert_equal(DT_NODE_EXISTS(DT_INST(1, vnd_gpio)), 1, "");
	zassert_equal(DT_NODE_EXISTS(DT_INST(2, vnd_gpio)), 1, "");

	zassert_equal(DT_NUM_INST_STATUS_OKAY(vnd_gpio), 2, "");
	zassert_equal(DT_NUM_INST_STATUS_OKAY(xxxx), 0, "");
}

static void test_has_nodelabel(void)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), okay), 0,
		      "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_nodelabel), okay), 1,
		      "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_nodelabel_allcaps),
					 okay),
		      1, "");
}

static void test_has_compat(void)
{
	unsigned int compats;

	zassert_true(DT_HAS_COMPAT_STATUS_OKAY(vnd_gpio), "");
	zassert_true(DT_HAS_COMPAT_STATUS_OKAY(vnd_gpio), "");
	zassert_false(DT_HAS_COMPAT_STATUS_OKAY(vnd_disabled_compat), "");

	zassert_equal(TA_HAS_COMPAT(vnd_array_holder), 1, "");
	zassert_equal(TA_HAS_COMPAT(vnd_undefined_compat), 1, "");
	zassert_equal(TA_HAS_COMPAT(vnd_not_a_test_array_compat), 0, "");
	compats = ((TA_HAS_COMPAT(vnd_array_holder) << 0) |
		   (TA_HAS_COMPAT(vnd_undefined_compat) << 1) |
		   (TA_HAS_COMPAT(vnd_not_a_test_array_compat) << 2));
	zassert_equal(compats, 0x3, "");
}

static void test_has_status(void)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_gpio_1), okay),
		      1, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_gpio_1), disabled),
		      0, "");

	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_no_status), okay),
		      1, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_no_status), disabled),
		      0, "");

	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), disabled),
		      1, "");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), okay),
		      0, "");
}

static void test_bus(void)
{
	/* common prefixes of expected labels: */
	const char *i2c_bus = "TEST_I2C_CTLR";
	const char *i2c_dev = "TEST_I2C_DEV";
	const char *spi_bus = "TEST_SPI_CTLR";
	const char *spi_dev = "TEST_SPI_DEV";
	const char *gpio = "TEST_GPIO_";
	int pin, flags;

	zassert_true(!strcmp(DT_LABEL(TEST_I2C_BUS), "TEST_I2C_CTLR"), "");
	zassert_true(!strcmp(DT_LABEL(TEST_SPI_BUS_0), "TEST_SPI_CTLR"), "");
	zassert_true(!strcmp(DT_LABEL(TEST_SPI_BUS_1), "TEST_SPI_CTLR"), "");

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

	zassert_true(!strncmp(gpio, DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
			      strlen(gpio)), "");

	pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	zassert_true((pin == 0x10) || (pin == 0x30), "");

	flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	zassert_true((flags == 0x20) || (flags == 0x40), "");

	zassert_equal(DT_ON_BUS(TEST_SPI_DEV_0, spi), 1, "");
	zassert_equal(DT_ON_BUS(TEST_SPI_DEV_0, i2c), 0, "");

	zassert_equal(DT_ON_BUS(TEST_I2C_DEV, i2c), 1, "");
	zassert_equal(DT_ON_BUS(TEST_I2C_DEV, spi), 0, "");

	zassert_true(!strcmp(DT_BUS_LABEL(TEST_I2C_DEV), "TEST_I2C_CTLR"), "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_spi_device
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 2, "");

	zassert_equal(DT_INST_ON_BUS(0, spi), 1, "");
	zassert_equal(DT_INST_ON_BUS(0, i2c), 0, "");

	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(spi), 1, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c), 0, "");

	zassert_true(!strncmp(spi_dev, DT_INST_LABEL(0), strlen(spi_dev)), "");
	zassert_true(!strncmp(spi_bus, DT_INST_BUS_LABEL(0), strlen(spi_bus)),
		     "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_i2c_device
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 2, "");

	zassert_equal(DT_INST_ON_BUS(0, i2c), 1, "");
	zassert_equal(DT_INST_ON_BUS(0, spi), 0, "");

	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c), 1, "");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(spi), 0, "");

	zassert_true(!strncmp(i2c_dev, DT_INST_LABEL(0), strlen(i2c_dev)), "");
	zassert_true(!strncmp(i2c_bus, DT_INST_BUS_LABEL(0), strlen(i2c_bus)),
		     "");

#undef DT_DRV_COMPAT
	/*
	 * Make sure the underlying DT_COMPAT_ON_BUS_INTERNAL used by
	 * DT_ANY_INST_ON_BUS works without DT_DRV_COMPAT defined.
	 */
	zassert_equal(DT_COMPAT_ON_BUS_INTERNAL(vnd_spi_device, spi), 1, NULL);
	zassert_equal(DT_COMPAT_ON_BUS_INTERNAL(vnd_spi_device, i2c), 0, NULL);

	zassert_equal(DT_COMPAT_ON_BUS_INTERNAL(vnd_i2c_device, i2c), 1, NULL);
	zassert_equal(DT_COMPAT_ON_BUS_INTERNAL(vnd_i2c_device, spi), 0, NULL);

	zassert_equal(DT_COMPAT_ON_BUS_INTERNAL(vnd_gpio_expander, i2c), 1,
		      NULL);
	zassert_equal(DT_COMPAT_ON_BUS_INTERNAL(vnd_gpio_expander, spi), 1,
		      NULL);
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_reg_holder
static void test_reg(void)
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

	/* DT_REG_SIZE */
	zassert_equal(DT_REG_SIZE(TEST_ABCD1234), 0x500, "");

	/* DT_REG_ADDR_BY_NAME */
	zassert_equal(DT_REG_ADDR_BY_NAME(TEST_ABCD1234, one), 0xabcd1234, "");
	zassert_equal(DT_REG_ADDR_BY_NAME(TEST_ABCD1234, two), 0x98765432, "");

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

	/* DT_INST_REG_SIZE */
	zassert_equal(DT_INST_REG_SIZE(0), 0x1000, "");

	/* DT_INST_REG_ADDR_BY_NAME */
	zassert_equal(DT_INST_REG_ADDR_BY_NAME(0, first), 0x9999aaaa, "");
	zassert_equal(DT_INST_REG_ADDR_BY_NAME(0, second), 0xbbbbcccc, "");

	/* DT_INST_REG_SIZE_BY_NAME */
	zassert_equal(DT_INST_REG_SIZE_BY_NAME(0, first), 0x1000, "");
	zassert_equal(DT_INST_REG_SIZE_BY_NAME(0, second), 0x3f, "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_interrupt_holder
static void test_irq(void)
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
	zassert_equal(DT_IRQN(TEST_I2C_BUS), 6, "");

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
	zassert_equal(DT_INST_IRQN(0), 30, "");

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
}

struct gpios_struct {
	const char *name;
	gpio_pin_t pin;
	gpio_flags_t flags;
};

/* Helper macro that UTIL_LISTIFY can use and produces an element with comma */
#define DT_PROP_ELEM_BY_PHANDLE(idx, node_id, ph_prop, prop) \
	DT_PROP_BY_PHANDLE_IDX(node_id, ph_prop, idx, prop),
#define DT_PHANDLE_LISTIFY(node_id, ph_prop, prop) \
	{ \
	  UTIL_LISTIFY(DT_PROP_LEN(node_id, ph_prop), \
		       DT_PROP_ELEM_BY_PHANDLE, \
		       node_id, \
		       ph_prop, \
		       label) \
	}

/* Helper macro that UTIL_LISTIFY can use and produces an element with comma */
#define DT_GPIO_ELEM(idx, node_id, prop) \
	{ \
		DT_PROP(DT_PHANDLE_BY_IDX(node_id, prop, idx), label), \
		DT_PHA_BY_IDX(node_id, prop, idx, pin),\
		DT_PHA_BY_IDX(node_id, prop, idx, flags),\
	},
#define DT_GPIO_LISTIFY(node_id, prop) \
	{ UTIL_LISTIFY(DT_PROP_LEN(node_id, prop), DT_GPIO_ELEM, \
		       node_id, prop) }

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_phandle_holder
static void test_phandles(void)
{
	const char *ph_label = DT_PROP_BY_PHANDLE(TEST_PH, ph, label);
	const char *phs_labels[] = DT_PHANDLE_LISTIFY(TEST_PH, phs, label);
	struct gpios_struct gps[] = DT_GPIO_LISTIFY(TEST_PH, gpios);

	/* phandle */
	zassert_true(DT_NODE_HAS_PROP(TEST_PH, ph), "");
	/* DT_PROP_BY_PHANDLE */
	zassert_true(!strcmp(ph_label, "TEST_GPIO_1"), "");

	/* phandles */
	zassert_true(DT_NODE_HAS_PROP(TEST_PH, phs), "");
	zassert_equal(ARRAY_SIZE(phs_labels), 3, "");
	zassert_equal(DT_PROP_LEN(TEST_PH, phs), 3, "");

	/* DT_PROP_BY_PHANDLE_IDX */
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, phs, 0, label),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, phs, 1, label),
			     "TEST_GPIO_2"), "");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, phs, 2, label),
			     "TEST_I2C_CTLR"), "");
	zassert_true(!strcmp(phs_labels[0], "TEST_GPIO_1"), "");
	zassert_true(!strcmp(phs_labels[1], "TEST_GPIO_2"), "");
	zassert_true(!strcmp(phs_labels[2], "TEST_I2C_CTLR"), "");
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 0,
					     gpio_controller), 1, "");
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 1,
					     gpio_controller), 1, "");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 0, label),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 1, label),
			     "TEST_GPIO_2"), "");

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
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE_BY_IDX(TEST_PH, gpios, 0)),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE_BY_IDX(TEST_PH, gpios, 1)),
			     "TEST_GPIO_2"), "");

	/* DT_PHANDLE */
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE(TEST_PH, gpios)),
			     "TEST_GPIO_1"), "");

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
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE_BY_NAME(TEST_PH, foos, a)),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE_BY_NAME(TEST_PH, foos, b_c)),
			     "TEST_GPIO_2"), "");

	/* array initializers */
	zassert_true(!strcmp(gps[0].name, "TEST_GPIO_1"), "");
	zassert_equal(gps[0].pin, 10, "");
	zassert_equal(gps[0].flags, 20, "");

	zassert_true(!strcmp(gps[1].name, "TEST_GPIO_2"), "");
	zassert_equal(gps[1].pin, 30, "");
	zassert_equal(gps[1].flags, 40, "");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	/* DT_INST_PROP_BY_PHANDLE */
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE(0, ph, label),
			     "TEST_GPIO_1"), "");

	zassert_true(!strcmp(ph_label, "TEST_GPIO_1"), "");

	/* DT_INST_PROP_BY_PHANDLE_IDX */
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, phs, 0, label),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, phs, 1, label),
			     "TEST_GPIO_2"), "");
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, phs, 2, label),
			     "TEST_I2C_CTLR"), "");
	zassert_true(!strcmp(phs_labels[0], "TEST_GPIO_1"), "");
	zassert_true(!strcmp(phs_labels[1], "TEST_GPIO_2"), "");
	zassert_true(!strcmp(phs_labels[2], "TEST_I2C_CTLR"), "");
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 0,
					     gpio_controller),
		      1, "");
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 1,
					     gpio_controller),
		      1, "");
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 0, label),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 1, label),
			     "TEST_GPIO_2"), "");

	/* DT_INST_PROP_HAS_IDX */
	zassert_true(DT_INST_PROP_HAS_IDX(0, gpios, 0), "");
	zassert_true(DT_INST_PROP_HAS_IDX(0, gpios, 1), "");
	zassert_false(DT_INST_PROP_HAS_IDX(0, gpios, 2), "");

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
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE_BY_IDX(0, gpios, 0)),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE_BY_IDX(0, gpios, 1)),
			     "TEST_GPIO_2"), "");

	/* DT_INST_PHANDLE */
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE(0, gpios)),
			     "TEST_GPIO_1"), "");

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
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE_BY_NAME(0, foos, a)),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE_BY_NAME(0, foos, b_c)),
			     "TEST_GPIO_2"), "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_phandle_holder
static void test_gpio(void)
{
	/* DT_GPIO_CTLR_BY_IDX */
	zassert_true(!strcmp(TO_STRING(DT_GPIO_CTLR_BY_IDX(TEST_PH, gpios, 0)),
			     TO_STRING(DT_NODELABEL(test_gpio_1))), "");
	zassert_true(!strcmp(TO_STRING(DT_GPIO_CTLR_BY_IDX(TEST_PH, gpios, 1)),
			     TO_STRING(DT_NODELABEL(test_gpio_2))), "");

	/* DT_GPIO_CTLR */
	zassert_true(!strcmp(TO_STRING(DT_GPIO_CTLR(TEST_PH, gpios)),
			     TO_STRING(DT_NODELABEL(test_gpio_1))), "");

	/* DT_GPIO_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_GPIO_LABEL_BY_IDX(TEST_PH, gpios, 0),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_GPIO_LABEL_BY_IDX(TEST_PH, gpios, 1),
			     "TEST_GPIO_2"), "");

	/* DT_GPIO_LABEL */
	zassert_true(!strcmp(DT_GPIO_LABEL(TEST_PH, gpios), "TEST_GPIO_1"), "");

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

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1, "");

	/* DT_INST_GPIO_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_INST_GPIO_LABEL_BY_IDX(0, gpios, 0),
			     "TEST_GPIO_1"), "");
	zassert_true(!strcmp(DT_INST_GPIO_LABEL_BY_IDX(0, gpios, 1),
			     "TEST_GPIO_2"), "");

	/* DT_INST_GPIO_LABEL */
	zassert_true(!strcmp(DT_INST_GPIO_LABEL(0, gpios), "TEST_GPIO_1"), "");

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
static void test_io_channels(void)
{
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL_BY_IDX(TEST_TEMP, 0),
			     "TEST_ADC_1"), "");
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL_BY_IDX(TEST_TEMP, 1),
			     "TEST_ADC_2"), "");
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL_BY_NAME(TEST_TEMP, ch1),
			     "TEST_ADC_1"), "");
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL_BY_NAME(TEST_TEMP, ch2),
			     "TEST_ADC_2"), "");
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL(TEST_TEMP),
			     "TEST_ADC_1"), "");

	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL_BY_IDX(0, 0),
			     "TEST_ADC_1"), "");
	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL_BY_IDX(0, 1),
			     "TEST_ADC_2"), "");
	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL_BY_NAME(0, ch1),
			     "TEST_ADC_1"), "");
	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL_BY_NAME(0, ch2),
			     "TEST_ADC_2"), "");
	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL(0),
			     "TEST_ADC_1"), "");

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
static void test_dma(void)
{
	zassert_true(!strcmp(DT_DMAS_LABEL_BY_NAME(TEST_TEMP, rx),
			     "TEST_DMA_CTRL_2"), "");
	zassert_true(!strcmp(DT_INST_DMAS_LABEL_BY_NAME(0, rx),
			     "TEST_DMA_CTRL_2"), "");
	zassert_true(!strcmp(DT_DMAS_LABEL_BY_NAME(TEST_TEMP, tx),
			     "TEST_DMA_CTRL_1"), "");
	zassert_true(!strcmp(DT_INST_DMAS_LABEL_BY_NAME(0, tx),
			     "TEST_DMA_CTRL_1"), "");

	zassert_true(!strcmp(DT_DMAS_LABEL_BY_IDX(TEST_TEMP, 1),
			     "TEST_DMA_CTRL_2"), "");
	zassert_true(!strcmp(DT_INST_DMAS_LABEL_BY_IDX(0, 1),
			     "TEST_DMA_CTRL_2"), "");
	zassert_true(!strcmp(DT_DMAS_LABEL_BY_IDX(TEST_TEMP, 0),
			     "TEST_DMA_CTRL_1"), "");
	zassert_true(!strcmp(DT_INST_DMAS_LABEL_BY_IDX(0, 0),
			     "TEST_DMA_CTRL_1"), "");

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
static void test_pwms(void)
{
	/* DT_PWMS_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_PWMS_LABEL_BY_IDX(TEST_PH, 0),
			     "TEST_PWM_CTRL_1"), "");

	/* DT_PWMS_LABEL_BY_NAME */
	zassert_true(!strcmp(DT_PWMS_LABEL_BY_NAME(TEST_PH, red),
			     "TEST_PWM_CTRL_1"), "");

	/* DT_PWMS_LABEL */
	zassert_true(!strcmp(DT_PWMS_LABEL(TEST_PH), "TEST_PWM_CTRL_1"), "");

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

	/* DT_INST_PWMS_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_INST_PWMS_LABEL_BY_IDX(0, 0),
			     "TEST_PWM_CTRL_1"), "");

	/* DT_INST_PWMS_LABEL_BY_NAME */
	zassert_true(!strcmp(DT_INST_PWMS_LABEL_BY_NAME(0, green),
			     "TEST_PWM_CTRL_2"), "");

	/* DT_INST_PWMS_LABEL */
	zassert_true(!strcmp(DT_INST_PWMS_LABEL(0), "TEST_PWM_CTRL_1"), "");

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

static void test_macro_names(void)
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

static void test_arrays(void)
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

static void test_foreach_prop_elem(void)
{
#define TIMES_TWO(node_id, prop, idx) \
	(2 * DT_PROP_BY_IDX(node_id, prop, idx)),

	int array[] = {
		DT_FOREACH_PROP_ELEM(TEST_ARRAYS, a, TIMES_TWO)
	};

	zassert_equal(ARRAY_SIZE(array), 3, "");
	zassert_equal(array[0], 2000, "");
	zassert_equal(array[1], 4000, "");
	zassert_equal(array[2], 6000, "");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_array_holder

	int inst_array[] = {
		DT_INST_FOREACH_PROP_ELEM(0, a, TIMES_TWO)
	};

	zassert_equal(ARRAY_SIZE(inst_array), ARRAY_SIZE(array), "");
	zassert_equal(inst_array[0], array[0], "");
	zassert_equal(inst_array[1], array[1], "");
	zassert_equal(inst_array[2], array[2], "");
#undef TIMES_TWO
}

static void test_foreach_prop_elem_varg(void)
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

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_array_holder

	int inst_array[] = {
		DT_INST_FOREACH_PROP_ELEM_VARGS(0, a, TIMES_TWO_ADD, 3)
	};

	zassert_equal(ARRAY_SIZE(inst_array), ARRAY_SIZE(array), "");
	zassert_equal(inst_array[0], array[0], "");
	zassert_equal(inst_array[1], array[1], "");
	zassert_equal(inst_array[2], array[2], "");
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

#define INST(num) DT_INST(num, vnd_gpio)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_gpio

static const struct gpio_driver_api test_api;

#define TEST_GPIO_INIT(num)					\
	static struct test_gpio_data gpio_data_##num = {	\
		.is_gpio_ctlr = DT_PROP(INST(num),		\
					gpio_controller),	\
	};							\
	static const struct test_gpio_info gpio_info_##num = {	\
		.reg_addr = DT_REG_ADDR(INST(num)),		\
		.reg_len = DT_REG_SIZE(INST(num)),		\
	};							\
	DEVICE_DT_DEFINE(INST(num),				\
			    test_gpio_init,			\
			    NULL,				\
			    &gpio_data_##num,			\
			    &gpio_info_##num,			\
			    POST_KERNEL,			\
			    CONFIG_APPLICATION_INIT_PRIORITY,	\
			    &test_api);

DT_INST_FOREACH_STATUS_OKAY(TEST_GPIO_INIT)

static inline struct test_gpio_data *to_data(const struct device *dev)
{
	return (struct test_gpio_data *)dev->data;
}

static inline const struct test_gpio_info *to_info(const struct device *dev)
{
	return (const struct test_gpio_info *)dev->config;
}

static void test_devices(void)
{
	const struct device *devs[3];
	int i = 0;
	const struct device *dev_abcd;
	unsigned int val;

	zassert_equal(DT_NUM_INST_STATUS_OKAY(vnd_gpio), 2, "");

	devs[i] = device_get_binding(DT_LABEL(INST(0)));
	if (devs[i]) {
		i++;
	}
	devs[i] = device_get_binding(DT_LABEL(INST(1)));
	if (devs[i]) {
		i++;
	}
	devs[i] = device_get_binding(DT_LABEL(INST(2)));
	if (devs[i]) {
		i++;
	}

	zassert_not_null(devs[0], "");
	zassert_not_null(devs[1], "");
	zassert_true(devs[2] == NULL, "");

	zassert_true(to_data(devs[0])->is_gpio_ctlr, "");
	zassert_true(to_data(devs[1])->is_gpio_ctlr, "");
	zassert_true(to_data(devs[0])->init_called, "");
	zassert_true(to_data(devs[1])->init_called, "");

	dev_abcd = device_get_binding(DT_LABEL(TEST_ABCD1234));
	zassert_not_null(dev_abcd, "");
	zassert_equal(to_info(dev_abcd)->reg_addr, 0xabcd1234, "");
	zassert_equal(to_info(dev_abcd)->reg_len, 0x500, "");

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

static void test_cs_gpios(void)
{
	zassert_equal(DT_SPI_HAS_CS_GPIOS(TEST_SPI_NO_CS), 0, "");
	zassert_equal(DT_SPI_NUM_CS_GPIOS(TEST_SPI_NO_CS), 0, "");

	zassert_equal(DT_SPI_HAS_CS_GPIOS(TEST_SPI), 1, "");
	zassert_equal(DT_SPI_NUM_CS_GPIOS(TEST_SPI), 3, "");

	zassert_equal(DT_DEP_ORD(DT_SPI_DEV_CS_GPIOS_CTLR(TEST_SPI_DEV_0)),
		      DT_DEP_ORD(DT_NODELABEL(test_gpio_1)),
		     "dev 0 cs gpio controller");
	zassert_true(!strcmp(DT_SPI_DEV_CS_GPIOS_LABEL(TEST_SPI_DEV_0),
			     "TEST_GPIO_1"), "");
	zassert_equal(DT_SPI_DEV_CS_GPIOS_PIN(TEST_SPI_DEV_0), 0x10, "");
	zassert_equal(DT_SPI_DEV_CS_GPIOS_FLAGS(TEST_SPI_DEV_0), 0x20, "");
}

static void test_chosen(void)
{
	zassert_equal(DT_HAS_CHOSEN(ztest_xxxx), 0, "");
	zassert_equal(DT_HAS_CHOSEN(ztest_gpio), 1, "");
	zassert_true(!strcmp(TO_STRING(DT_CHOSEN(ztest_gpio)),
			     "DT_N_S_test_S_gpio_deadbeef"), "");
}

#define TO_MY_ENUM(token) TO_MY_ENUM_2(token) /* force another expansion */
#define TO_MY_ENUM_2(token) MY_ENUM_ ## token
static void test_enums(void)
{
	enum {
		MY_ENUM_zero = 0xff,
		MY_ENUM_ZERO = 0xaa,
	};

	zassert_equal(DT_ENUM_IDX(TEST_ENUM_0, val), 0, "0");
	zassert_equal(TO_MY_ENUM(DT_ENUM_TOKEN(TEST_ENUM_0, val)), 0xff, "");
	zassert_equal(TO_MY_ENUM(DT_ENUM_UPPER_TOKEN(TEST_ENUM_0, val)),
		      0xaa, "");
}
#undef TO_MY_ENUM
#undef TO_MY_ENUM_2

static void test_enums_required_false(void)
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

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
static void test_clocks(void)
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

	/* DT_CLOCKS_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_CLOCKS_LABEL_BY_IDX(TEST_TEMP, 0),
			     "TEST_CLOCK"), "");

	/* DT_CLOCKS_LABEL_BY_NAME */
	zassert_true(!strcmp(DT_CLOCKS_LABEL_BY_NAME(TEST_TEMP, clk_b),
			     "TEST_CLOCK"), "");

	/* DT_CLOCKS_LABEL */
	zassert_true(!strcmp(DT_CLOCKS_LABEL(TEST_TEMP), "TEST_CLOCK"), "");

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

	/* DT_INST_CLOCKS_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_INST_CLOCKS_LABEL_BY_IDX(0, 0),
			     "TEST_CLOCK"), "");

	/* DT_INST_CLOCKS_LABEL_BY_NAME */
	zassert_true(!strcmp(DT_INST_CLOCKS_LABEL_BY_NAME(0, clk_b),
			     "TEST_CLOCK"), "");

	/* DT_INST_CLOCKS_LABEL */
	zassert_true(!strcmp(DT_INST_CLOCKS_LABEL(0), "TEST_CLOCK"), "");

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

static void test_parent(void)
{
	/* The label of a child node's parent is the label of the parent. */
	zassert_true(!strcmp(DT_LABEL(DT_PARENT(TEST_SPI_DEV_0)),
			     DT_LABEL(TEST_SPI_BUS_0)), "");
	/*
	 * We should be able to use DT_PARENT() even with nodes, like /test,
	 * that have no matching compatible.
	 */
	zassert_true(!strcmp(DT_LABEL(DT_CHILD(DT_PARENT(TEST_SPI_BUS_0),
					       spi_33334444)),
			     DT_LABEL(TEST_SPI_BUS_0)), "");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_child_bindings
static void test_child_nodes_list(void)
{
	#define TEST_FUNC(child) { DT_PROP(child, val) },
	#define TEST_PARENT DT_PARENT(DT_NODELABEL(test_child_a))

	struct vnd_child_binding {
		int val;
	};

	struct vnd_child_binding vals[] = {
		DT_FOREACH_CHILD(TEST_PARENT, TEST_FUNC)
	};

	struct vnd_child_binding vals_inst[] = {
		DT_INST_FOREACH_CHILD(0, TEST_FUNC)
	};

	struct vnd_child_binding vals_status_okay[] = {
		DT_FOREACH_CHILD_STATUS_OKAY(TEST_PARENT, TEST_FUNC)
	};

	zassert_equal(ARRAY_SIZE(vals), 3, "");
	zassert_equal(ARRAY_SIZE(vals_inst), 3, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay), 2, "");

	zassert_equal(vals[0].val, 0, "");
	zassert_equal(vals[1].val, 1, "");
	zassert_equal(vals[2].val, 2, "");
	zassert_equal(vals_inst[0].val, 0, "");
	zassert_equal(vals_inst[1].val, 1, "");
	zassert_equal(vals_inst[2].val, 2, "");
	zassert_equal(vals_status_okay[0].val, 0, "");
	zassert_equal(vals_status_okay[1].val, 1, "");

	#undef TEST_PARENT
	#undef TEST_FUNC
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_child_bindings
static void test_child_nodes_list_varg(void)
{
	#define TEST_FUNC(child, arg) { DT_PROP(child, val) + arg },
	#define TEST_PARENT DT_PARENT(DT_NODELABEL(test_child_a))

	struct vnd_child_binding {
		int val;
	};

	struct vnd_child_binding vals[] = {
		DT_FOREACH_CHILD_VARGS(TEST_PARENT, TEST_FUNC, 1)
	};

	struct vnd_child_binding vals_inst[] = {
		DT_INST_FOREACH_CHILD_VARGS(0, TEST_FUNC, 1)
	};

	struct vnd_child_binding vals_status_okay[] = {
		DT_FOREACH_CHILD_STATUS_OKAY_VARGS(TEST_PARENT, TEST_FUNC, 1)
	};

	zassert_equal(ARRAY_SIZE(vals), 3, "");
	zassert_equal(ARRAY_SIZE(vals_inst), 3, "");
	zassert_equal(ARRAY_SIZE(vals_status_okay), 2, "");

	zassert_equal(vals[0].val, 1, "");
	zassert_equal(vals[1].val, 2, "");
	zassert_equal(vals[2].val, 3, "");
	zassert_equal(vals_inst[0].val, 1, "");
	zassert_equal(vals_inst[1].val, 2, "");
	zassert_equal(vals_inst[2].val, 3, "");
	zassert_equal(vals_status_okay[0].val, 1, "");
	zassert_equal(vals_status_okay[1].val, 2, "");

	#undef TEST_PARENT
	#undef TEST_FUNC
}

static void test_great_grandchild(void)
{
	zassert_equal(DT_PROP(DT_NODELABEL(test_ggc), ggc_prop), 42, "");
}

static void test_compat_get_any_status_okay(void)
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

static void test_dep_ord(void)
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
		      "%u", ARRAY_SIZE(children_combined_ords));
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
		      "%u", ARRAY_SIZE(child_a_combined_ords));
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

static void test_path(void)
{
	zassert_true(!strcmp(DT_NODE_PATH(DT_ROOT), "/"), "");
	zassert_true(!strcmp(DT_NODE_PATH(TEST_DEADBEEF),
			     "/test/gpio@deadbeef"), "");
}

static void test_node_name(void)
{
	zassert_true(!strcmp(DT_NODE_FULL_NAME(DT_ROOT), "/"), "");
	zassert_true(!strcmp(DT_NODE_FULL_NAME(TEST_DEADBEEF),
			     "gpio@deadbeef"), "");
	zassert_true(!strcmp(DT_NODE_FULL_NAME(TEST_TEMP),
			     "temperature-sensor"), "");
	zassert_true(strcmp(DT_NODE_FULL_NAME(TEST_REG),
			     "reg-holder"), "");
}

static void test_same_node(void)
{
	zassert_true(DT_SAME_NODE(TEST_DEADBEEF, TEST_DEADBEEF), "");
	zassert_false(DT_SAME_NODE(TEST_DEADBEEF, TEST_ABCD1234), "");
}

void test_main(void)
{
	ztest_test_suite(devicetree_api,
			 ztest_unit_test(test_path_props),
			 ztest_unit_test(test_alias_props),
			 ztest_unit_test(test_nodelabel_props),
			 ztest_unit_test(test_inst_props),
			 ztest_unit_test(test_default_prop_access),
			 ztest_unit_test(test_has_path),
			 ztest_unit_test(test_has_alias),
			 ztest_unit_test(test_inst_checks),
			 ztest_unit_test(test_has_nodelabel),
			 ztest_unit_test(test_has_compat),
			 ztest_unit_test(test_has_status),
			 ztest_unit_test(test_bus),
			 ztest_unit_test(test_reg),
			 ztest_unit_test(test_irq),
			 ztest_unit_test(test_phandles),
			 ztest_unit_test(test_gpio),
			 ztest_unit_test(test_io_channels),
			 ztest_unit_test(test_dma),
			 ztest_unit_test(test_pwms),
			 ztest_unit_test(test_macro_names),
			 ztest_unit_test(test_arrays),
			 ztest_unit_test(test_foreach_prop_elem),
			 ztest_unit_test(test_foreach_prop_elem_varg),
			 ztest_unit_test(test_devices),
			 ztest_unit_test(test_cs_gpios),
			 ztest_unit_test(test_chosen),
			 ztest_unit_test(test_enums),
			 ztest_unit_test(test_enums_required_false),
			 ztest_unit_test(test_clocks),
			 ztest_unit_test(test_parent),
			 ztest_unit_test(test_child_nodes_list),
			 ztest_unit_test(test_child_nodes_list_varg),
			 ztest_unit_test(test_great_grandchild),
			 ztest_unit_test(test_compat_get_any_status_okay),
			 ztest_unit_test(test_dep_ord),
			 ztest_unit_test(test_path),
			 ztest_unit_test(test_node_name),
			 ztest_unit_test(test_same_node)
		);
	ztest_run_test_suite(devicetree_api);
}
