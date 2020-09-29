/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <devicetree.h>
#include <device.h>
#include <drivers/gpio.h>

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

#define TEST_I2C_DEV DT_PATH(test, i2c_11112222, test_i2c_dev_10)
#define TEST_I2C_BUS DT_BUS(TEST_I2C_DEV)

#define TEST_SPI DT_NODELABEL(test_spi)

#define TEST_SPI_DEV_0 DT_PATH(test, spi_33334444, test_spi_dev_0)
#define TEST_SPI_BUS_0 DT_BUS(TEST_SPI_DEV_0)

#define TEST_SPI_DEV_1 DT_PATH(test, spi_33334444, test_spi_dev_1)
#define TEST_SPI_BUS_1 DT_BUS(TEST_SPI_DEV_1)

#define TEST_SPI_NO_CS DT_NODELABEL(test_spi_no_cs)
#define TEST_SPI_DEV_NO_CS DT_NODELABEL(test_spi_no_cs)

#define TA_HAS_COMPAT(compat) DT_NODE_HAS_COMPAT(TEST_ARRAYS, compat)

#define TO_STRING(x) TO_STRING_(x)
#define TO_STRING_(x) #x

static void test_path_props(void)
{
	zassert_true(!strcmp(DT_LABEL(TEST_DEADBEEF), "TEST_GPIO_1"),
		     "deadbeef label != TEST_GPIO_1");
	zassert_equal(DT_NUM_REGS(TEST_DEADBEEF), 1,
		      "deadbeef num reg");
	zassert_equal(DT_REG_ADDR(TEST_DEADBEEF), 0xdeadbeef,
		      "deadbeef reg[0] addr");
	zassert_equal(DT_REG_SIZE(TEST_DEADBEEF), 0x1000,
		      "deadbeef reg[0] len");
	zassert_equal(DT_PROP(TEST_DEADBEEF, gpio_controller), 1,
		      "deadbeef gpio-controller");
	zassert_equal(DT_PROP(TEST_DEADBEEF, ngpios), 32, "deadbeef ngpios");
	zassert_true(!strcmp(DT_PROP(TEST_DEADBEEF, status), "okay"),
		     "deadbeef status");
	zassert_equal(DT_PROP_LEN(TEST_DEADBEEF, compatible), 1,
		      "deadbeef compatible len");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_DEADBEEF, compatible, 0),
			     "vnd,gpio"),
		     "deadbeef compatible[0]");
	zassert_true(DT_NODE_HAS_PROP(TEST_DEADBEEF, status),
		     "deadbeef status");
	zassert_false(DT_NODE_HAS_PROP(TEST_DEADBEEF, foobar),
		      "deadbeef foobar");

	zassert_true(!strcmp(DT_LABEL(TEST_ABCD1234), "TEST_GPIO_2"),
		     "abcd1234 label != TEST_GPIO_2");
	zassert_equal(DT_NUM_REGS(TEST_ABCD1234), 2,
		      "abcd1234 num regs");
	zassert_equal(DT_PROP(TEST_ABCD1234, gpio_controller), 1,
		     "abcd1234 gpio-controller");
	zassert_equal(DT_PROP(TEST_ABCD1234, ngpios), 32, "abcd1234 ngpios");
	zassert_true(!strcmp(DT_PROP(TEST_ABCD1234, status), "okay"),
		     "abcd1234 status");
	zassert_equal(DT_PROP_LEN(TEST_ABCD1234, compatible), 1,
		      "abcd1234 compatible len");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ABCD1234, compatible, 0),
			     "vnd,gpio"),
		     "abcd1234 compatible[0]");
}

static void test_alias_props(void)
{
	zassert_equal(DT_NUM_REGS(TEST_ALIAS), 1, "num regs");
	zassert_equal(DT_REG_ADDR(TEST_ALIAS), 0xdeadbeef, "reg[0] addr");
	zassert_equal(DT_REG_SIZE(TEST_ALIAS), 0x1000, "reg[0] len");
	zassert_true(!strcmp(DT_LABEL(TEST_ALIAS), "TEST_GPIO_1"), "label");
	zassert_equal(DT_PROP(TEST_ALIAS, gpio_controller), 1,
		      "gpio-controller");
	zassert_equal(DT_PROP(TEST_ALIAS, ngpios), 32, "ngpios");
	zassert_true(!strcmp(DT_PROP(TEST_ALIAS, status), "okay"), "status");
	zassert_equal(DT_PROP_LEN(TEST_ALIAS, compatible), 1,
		      "compatible len");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ALIAS, compatible, 0),
			     "vnd,gpio"),
		     "compatible[0]");
}

static void test_nodelabel_props(void)
{
	zassert_equal(DT_NUM_REGS(TEST_NODELABEL), 1, "num regs");
	zassert_equal(DT_REG_ADDR(TEST_NODELABEL), 0xdeadbeef, "reg[0] addr");
	zassert_equal(DT_REG_SIZE(TEST_NODELABEL), 0x1000, "reg[0] len");
	zassert_true(!strcmp(DT_LABEL(TEST_NODELABEL), "TEST_GPIO_1"), "label");
	zassert_equal(DT_PROP(TEST_NODELABEL, gpio_controller), 1,
		      "gpio-controller");
	zassert_equal(DT_PROP(TEST_NODELABEL, ngpios), 32, "ngpios");
	zassert_true(!strcmp(DT_PROP(TEST_NODELABEL, status), "okay"),
		     "status");
	zassert_equal(DT_PROP_LEN(TEST_NODELABEL, compatible), 1,
		      "compatible len");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_NODELABEL, compatible, 0),
			     "vnd,gpio"),
		     "compatible[0]");
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

	zassert_equal(DT_PROP(TEST_INST, gpio_controller), 1,
		      "gpio-controller");
	zassert_true(!strcmp(DT_PROP(TEST_INST, status), "okay") ||
		     !strcmp(DT_PROP(TEST_INST, status), "disabled"),
		     "status");
	zassert_equal(DT_PROP_LEN(TEST_INST, compatible), 1,
		      "compatible len");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_INST, compatible, 0),
			     "vnd,gpio"),
		     "compatible[0]");

	zassert_equal(DT_INST_NODE_HAS_PROP(0, gpio_controller), 1,
		      "inst 0 has no gpio-controller");
	zassert_equal(DT_INST_PROP(0, gpio_controller), 1,
		      "inst 0 gpio-controller is not 1");
	zassert_equal(DT_INST_NODE_HAS_PROP(0, xxxx), 0,
		      "inst 0 has xxxx prop");
	zassert_true(!strcmp(DT_INST_PROP(0, status), "okay") ||
		     !strcmp(DT_PROP(TEST_INST, status), "disabled"),
		     "inst 0 status");
	zassert_equal(DT_INST_PROP_LEN(0, compatible), 1,
		      "inst 0 compatible len");
	zassert_true(!strcmp(DT_INST_PROP_BY_IDX(0, compatible, 0),
			     "vnd,gpio"),
		     "inst 0 compatible[0]");
	zassert_true(!strncmp(label_startswith, DT_INST_LABEL(0),
			      strlen(label_startswith)),
		     "inst 0 label");
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
	zassert_equal(DT_NODE_HAS_STATUS(DT_PATH(test, gpio_0), okay), 0,
		      "gpio@0");
	zassert_equal(DT_NODE_HAS_STATUS(DT_PATH(test, gpio_deadbeef), okay), 1,
		      "gpio@deadbeef");
	zassert_equal(DT_NODE_HAS_STATUS(DT_PATH(test, gpio_abcd1234), okay), 1,
		      "gpio@abcd1234");
}

static void test_has_alias(void)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_ALIAS(test_alias), okay), 1,
		      "test-alias");
	zassert_equal(DT_NODE_HAS_STATUS(DT_ALIAS(test_undef), okay), 0,
		      "test-undef");
}

static void test_inst_checks(void)
{
	zassert_equal(DT_NODE_EXISTS(DT_INST(0, vnd_gpio)), 1,
		      "vnd,gpio #0");
	zassert_equal(DT_NODE_EXISTS(DT_INST(1, vnd_gpio)), 1,
		      "vnd,gpio #1");
	zassert_equal(DT_NODE_EXISTS(DT_INST(2, vnd_gpio)), 1,
		      "vnd,gpio #2");

	zassert_equal(DT_NUM_INST_STATUS_OKAY(vnd_gpio), 2, "num. vnd,gpio");
	zassert_equal(DT_NUM_INST_STATUS_OKAY(xxxx), 0, "num. xxxx");
}

static void test_has_nodelabel(void)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), okay), 0,
		      "disabled_gpio");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_nodelabel), okay), 1,
		      "test_nodelabel");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_nodelabel_allcaps),
					 okay),
		      1,
		      "TEST_NODELABEL_ALLCAPS");
}

static void test_has_compat(void)
{
	unsigned int compats;

	zassert_true(DT_HAS_COMPAT_STATUS_OKAY(vnd_gpio), "vnd,gpio");
	zassert_true(DT_HAS_COMPAT_STATUS_OKAY(vnd_gpio), "vnd,i2c");
	zassert_false(DT_HAS_COMPAT_STATUS_OKAY(vnd_disabled_compat),
		      "vnd,disabled-compat");

	zassert_equal(TA_HAS_COMPAT(vnd_array_holder), 1, "vnd,array-holder");
	zassert_equal(TA_HAS_COMPAT(vnd_undefined_compat), 1,
		      "vnd,undefined-compat");
	zassert_equal(TA_HAS_COMPAT(vnd_not_a_test_array_compat), 0,
		      "not present");
	compats = ((TA_HAS_COMPAT(vnd_array_holder) << 0) |
		   (TA_HAS_COMPAT(vnd_undefined_compat) << 1) |
		   (TA_HAS_COMPAT(vnd_not_a_test_array_compat) << 2));
	zassert_equal(compats, 0x3, "as bit array");
}

static void test_has_status(void)
{
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_gpio_1), okay),
		      1, "vnd,gpio okay");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_gpio_1), disabled),
		      0, "vnd,gpio not disabled");

	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_no_status), okay),
		      1, "vnd,gpio okay");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(test_no_status), disabled),
		      0, "vnd,gpio not disabled");

	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), disabled),
		      1, "vnd,disabled-compat disabled");
	zassert_equal(DT_NODE_HAS_STATUS(DT_NODELABEL(disabled_gpio), okay),
		      0, "vnd,disabled-compat not okay");
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

	zassert_true(!strcmp(DT_LABEL(TEST_I2C_BUS), "TEST_I2C_CTLR"), "i2c");
	zassert_true(!strcmp(DT_LABEL(TEST_SPI_BUS_0), "TEST_SPI_CTLR"),
		     "spi 0");
	zassert_true(!strcmp(DT_LABEL(TEST_SPI_BUS_1), "TEST_SPI_CTLR"),
		     "spi 1");

	zassert_equal(DT_SPI_DEV_HAS_CS_GPIOS(TEST_SPI_DEV_0), 1, "no cs");
	zassert_equal(DT_SPI_DEV_HAS_CS_GPIOS(TEST_SPI_DEV_NO_CS), 0, "has cs");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_spi_device_2
	/* there is only one instance, and it has no CS */
	zassert_equal(DT_INST_SPI_DEV_HAS_CS_GPIOS(0), 0,
		      "inst of vnd,spi-device-2 with cs");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_spi_device
	/*
	 * DT_INST_SPI_DEV: use with care here. We could be matching
	 * either vnd,spi-device.
	 */
	zassert_equal(DT_INST_SPI_DEV_HAS_CS_GPIOS(0), 1,
		      "inst of vnd,spi-device without cs");

	zassert_true(!strncmp(gpio, DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
			      strlen(gpio)),
		     "inst 0 cs label");

	pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	zassert_true((pin == 0x10) || (pin == 0x30), "inst 0 cs pin");

	flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	zassert_true((flags == 0x20) || (flags == 0x40), "inst 0 cs flags");

	zassert_equal(DT_ON_BUS(TEST_SPI_DEV_0, spi), 1, "spidev not on spi");
	zassert_equal(DT_ON_BUS(TEST_SPI_DEV_0, i2c), 0, "spidev on i2c");

	zassert_equal(DT_ON_BUS(TEST_I2C_DEV, i2c), 1, "i2cdev not on i2c");
	zassert_equal(DT_ON_BUS(TEST_I2C_DEV, spi), 0, "i2cdev on spi");

	zassert_true(!strcmp(DT_BUS_LABEL(TEST_I2C_DEV), "TEST_I2C_CTLR"),
		     "bus label");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_spi_device
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 2,
		      "spi device count");

	zassert_equal(DT_INST_ON_BUS(0, spi), 1, "spi inst 0 not on spi");
	zassert_equal(DT_INST_ON_BUS(0, i2c), 0, "spi inst 0 on i2c");

	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(spi), 1, "no spi is on spi");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c), 0, "a spi is on i2c");

	zassert_true(!strncmp(spi_dev, DT_INST_LABEL(0), strlen(spi_dev)),
		     "inst 0 spi dev label");
	zassert_true(!strncmp(spi_bus, DT_INST_BUS_LABEL(0), strlen(spi_bus)),
		     "inst 0 spi bus label");

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_i2c_device
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 2,
		      "i2c device count");

	zassert_equal(DT_INST_ON_BUS(0, i2c), 1, "i2c inst 0 not on i2c");
	zassert_equal(DT_INST_ON_BUS(0, spi), 0, "i2c inst 0 on spi");

	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c), 1, "no i2c is on i2c");
	zassert_equal(DT_ANY_INST_ON_BUS_STATUS_OKAY(spi), 0, "an i2c is on spi");

	zassert_true(!strncmp(i2c_dev, DT_INST_LABEL(0), strlen(i2c_dev)),
		     "inst 0 i2c dev label");
	zassert_true(!strncmp(i2c_bus, DT_INST_BUS_LABEL(0), strlen(i2c_bus)),
		     "inst 0 i2c bus label");

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
	zassert_true(DT_REG_HAS_IDX(TEST_ABCD1234, 0), "has idx 0");
	zassert_true(DT_REG_HAS_IDX(TEST_ABCD1234, 1), "has idx 1");
	zassert_false(DT_REG_HAS_IDX(TEST_ABCD1234, 2), "!has idx 2");

	/* DT_REG_ADDR_BY_IDX */
	zassert_equal(DT_REG_ADDR_BY_IDX(TEST_ABCD1234, 0), 0xabcd1234,
		      "abcd1234 reg[1] addr");
	zassert_equal(DT_REG_ADDR_BY_IDX(TEST_ABCD1234, 1), 0x98765432,
		      "abcd1234 reg[1] addr");

	/* DT_REG_SIZE_BY_IDX */
	zassert_equal(DT_REG_SIZE_BY_IDX(TEST_ABCD1234, 0), 0x500,
		      "abcd1234 reg[1] size");
	zassert_equal(DT_REG_SIZE_BY_IDX(TEST_ABCD1234, 1), 0xff,
		      "abcd1234 reg[1] size");

	/* DT_REG_ADDR */
	zassert_equal(DT_REG_ADDR(TEST_ABCD1234), 0xabcd1234,
		      "abcd1234 reg[0] addr");

	/* DT_REG_SIZE */
	zassert_equal(DT_REG_SIZE(TEST_ABCD1234), 0x500,
		      "abcd1234 reg[0] size");

	/* DT_REG_ADDR_BY_NAME */
	zassert_equal(DT_REG_ADDR_BY_NAME(TEST_ABCD1234, one), 0xabcd1234,
		      "abcd1234 reg[one] addr");
	zassert_equal(DT_REG_ADDR_BY_NAME(TEST_ABCD1234, two), 0x98765432,
		      "abcd1234 reg[two] addr");

	/* DT_REG_SIZE_BY_NAME */
	zassert_equal(DT_REG_SIZE_BY_NAME(TEST_ABCD1234, one), 0x500,
		      "abcd1234 reg[one] size");
	zassert_equal(DT_REG_SIZE_BY_NAME(TEST_ABCD1234, two), 0xff,
		      "abcd1234 reg[two] size");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1,
		      "one instance");

	/* DT_INST_REG_HAS_IDX */
	zassert_true(DT_INST_REG_HAS_IDX(0, 0), "has idx 0");
	zassert_true(DT_INST_REG_HAS_IDX(0, 1), "has idx 1");
	zassert_false(DT_INST_REG_HAS_IDX(0, 2), "!has idx 2");

	/* DT_INST_REG_ADDR_BY_IDX */
	zassert_equal(DT_INST_REG_ADDR_BY_IDX(0, 0), 0x9999aaaa,
		      "abcd1234 reg[1] addr");
	zassert_equal(DT_INST_REG_ADDR_BY_IDX(0, 1), 0xbbbbcccc,
		      "abcd1234 reg[1] addr");

	/* DT_INST_REG_SIZE_BY_IDX */
	zassert_equal(DT_INST_REG_SIZE_BY_IDX(0, 0), 0x1000,
		      "abcd1234 reg[1] size");
	zassert_equal(DT_INST_REG_SIZE_BY_IDX(0, 1), 0x3f,
		      "abcd1234 reg[1] size");

	/* DT_INST_REG_ADDR */
	zassert_equal(DT_INST_REG_ADDR(0), 0x9999aaaa,
		      "abcd1234 reg[0] addr");

	/* DT_INST_REG_SIZE */
	zassert_equal(DT_INST_REG_SIZE(0), 0x1000,
		      "abcd1234 reg[0] size");

	/* DT_INST_REG_ADDR_BY_NAME */
	zassert_equal(DT_INST_REG_ADDR_BY_NAME(0, first), 0x9999aaaa,
		      "abcd1234 reg[one] addr");
	zassert_equal(DT_INST_REG_ADDR_BY_NAME(0, second), 0xbbbbcccc,
		      "abcd1234 reg[two] addr");

	/* DT_INST_REG_SIZE_BY_NAME */
	zassert_equal(DT_INST_REG_SIZE_BY_NAME(0, first), 0x1000,
		      "abcd1234 reg[one] size");
	zassert_equal(DT_INST_REG_SIZE_BY_NAME(0, second), 0x3f,
		      "abcd1234 reg[two] size");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_interrupt_holder
static void test_irq(void)
{
	/* DT_NUM_IRQS */
	zassert_equal(DT_NUM_IRQS(TEST_DEADBEEF), 1, "deadbeef num_irqs");
	zassert_equal(DT_NUM_IRQS(TEST_I2C_BUS), 2, "TEST_I2C_BUS num_irqs");
	zassert_equal(DT_NUM_IRQS(TEST_SPI), 3, "TEST_SPI num_irqs");

	/* DT_IRQ_HAS_IDX */
	zassert_true(DT_IRQ_HAS_IDX(TEST_SPI_BUS_0, 0), "spi has idx 0");
	zassert_true(DT_IRQ_HAS_IDX(TEST_SPI_BUS_0, 1), "spi has idx 1");
	zassert_true(DT_IRQ_HAS_IDX(TEST_SPI_BUS_0, 2), "spi has idx 2");
	zassert_false(DT_IRQ_HAS_IDX(TEST_SPI_BUS_0, 3), "!spi has idx 3");

	zassert_true(DT_IRQ_HAS_IDX(TEST_DEADBEEF, 0), "deadbeef has idx 0");
	zassert_false(DT_IRQ_HAS_IDX(TEST_DEADBEEF, 1), "!deadbeef has idx 1");

	zassert_true(DT_IRQ_HAS_IDX(TEST_I2C_BUS, 0), "i2c has idx 0");
	zassert_true(DT_IRQ_HAS_IDX(TEST_I2C_BUS, 1), "i2c has idx 1");
	zassert_false(DT_IRQ_HAS_IDX(TEST_I2C_BUS, 2), "!i2c has idx 2");

	/* DT_IRQ_BY_IDX */
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 0, irq), 8, "irq 0");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 1, irq), 9, "irq 1");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 2, irq), 10, "irq 2");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 0, priority), 3, "irq 0 pri");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 1, priority), 0, "irq 1 pri");
	zassert_equal(DT_IRQ_BY_IDX(TEST_SPI_BUS_0, 2, priority), 1, "irq 2 pri");

	/* DT_IRQ_BY_NAME */
	zassert_equal(DT_IRQ_BY_NAME(TEST_I2C_BUS, status, irq), 6, "irq status");
	zassert_equal(DT_IRQ_BY_NAME(TEST_I2C_BUS, error, irq), 7, "irq error");
	zassert_equal(DT_IRQ_BY_NAME(TEST_I2C_BUS, status, priority), 2,
		      "irq status pri");
	zassert_equal(DT_IRQ_BY_NAME(TEST_I2C_BUS, error, priority), 1,
		       "irq error pri");

	/* DT_IRQ_HAS_CELL_AT_IDX */
	zassert_true(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 0, irq), "no irq 0");
	zassert_true(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 0, priority),
		     "no irq 0 pri");
	zassert_false(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 0, foo), 0,
		      "has irq 0 foo");
	zassert_true(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 2, irq), "no irq 2");
	zassert_true(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 2, priority),
		     "no irq 2 pri");
	zassert_false(DT_IRQ_HAS_CELL_AT_IDX(TEST_IRQ, 2, foo),
		      "has irq 2 foo");

	/* DT_IRQ_HAS_CELL */
	zassert_true(DT_IRQ_HAS_CELL(TEST_IRQ, irq), "no irq");
	zassert_true(DT_IRQ_HAS_CELL(TEST_IRQ, priority), "no irq pri");
	zassert_false(DT_IRQ_HAS_CELL(TEST_IRQ, foo), "has irq foo");

	/* DT_IRQ_HAS_NAME */
	zassert_true(DT_IRQ_HAS_NAME(TEST_IRQ, err), "no err irq");
	zassert_true(DT_IRQ_HAS_NAME(TEST_IRQ, stat), "no stat irq");
	zassert_true(DT_IRQ_HAS_NAME(TEST_IRQ, done), "no done irq");
	zassert_false(DT_IRQ_HAS_NAME(TEST_IRQ, alpha), "no err irq");

	/* DT_IRQ */
	zassert_equal(DT_IRQ(TEST_I2C_BUS, irq), 6, "irq");
	zassert_equal(DT_IRQ(TEST_I2C_BUS, priority), 2, "irq pri");

	/* DT_IRQN */
	zassert_equal(DT_IRQN(TEST_I2C_BUS), 6, "irqn");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1,
		      "one instance");

	/* DT_INST_IRQ_HAS_IDX */
	zassert_equal(DT_INST_IRQ_HAS_IDX(0, 0), 1, "inst 0 irq 0 missing");
	zassert_equal(DT_INST_IRQ_HAS_IDX(0, 1), 1, "inst 0 irq 1 missing");
	zassert_equal(DT_INST_IRQ_HAS_IDX(0, 2), 1, "inst 0 irq 2 missing");
	zassert_equal(DT_INST_IRQ_HAS_IDX(0, 3), 0, "inst 0 irq 3 present");

	/* DT_INST_IRQ_BY_IDX */
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 0, irq), 30, "inst 0 irq 0");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 1, irq), 40, "inst 0 irq 1");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 2, irq), 60, "inst 0 irq 2");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 0, priority), 3, "inst 0 irq 0 pri");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 1, priority), 5, "inst 0 irq 1 pri");
	zassert_equal(DT_INST_IRQ_BY_IDX(0, 2, priority), 7, "inst 0 irq 2 pri");

	/* DT_INST_IRQ_BY_NAME */
	zassert_equal(DT_INST_IRQ_BY_NAME(0, err, irq), 30, "inst 0 irq error");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, stat, irq), 40, "inst 0 irq status");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, done, irq), 60, "inst 0 done error");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, err, priority), 3,
		       "inst 0 irq error pri");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, stat, priority), 5,
		      "inst 0 irq status pri");
	zassert_equal(DT_INST_IRQ_BY_NAME(0, done, priority), 7,
		       "inst 0 irq done pri");

	/* DT_INST_IRQ */
	zassert_equal(DT_INST_IRQ(0, irq), 30, "inst 0 irq");
	zassert_equal(DT_INST_IRQ(0, priority), 3, "inst 0 irq pri");

	/* DT_INST_IRQN */
	zassert_equal(DT_INST_IRQN(0), 30, "inst 0 irqn");

	/* DT_INST_IRQ_HAS_CELL_AT_IDX */
	zassert_true(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 0, irq), "no inst irq 0");
	zassert_true(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 0, priority),
		      "no inst irq 0 pri");
	zassert_false(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 0, foo),
		      "has inst irq 0 foo");
	zassert_true(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 2, irq), "no inst irq 2");
	zassert_true(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 2, priority),
		      "no inst irq 2 pri");
	zassert_false(DT_INST_IRQ_HAS_CELL_AT_IDX(0, 2, foo),
		      "has inst irq 2 foo");

	/* DT_INST_IRQ_HAS_CELL */
	zassert_true(DT_INST_IRQ_HAS_CELL(0, irq), "no inst 0 irq");
	zassert_true(DT_INST_IRQ_HAS_CELL(0, priority), "no inst 0 irq pri");
	zassert_false(DT_INST_IRQ_HAS_CELL(0, foo), "has inst 0 irq foo");

	/* DT_INST_IRQ_HAS_NAME */
	zassert_true(DT_INST_IRQ_HAS_NAME(0, err), "no inst err irq");
	zassert_true(DT_INST_IRQ_HAS_NAME(0, stat), "no inst err irq");
	zassert_true(DT_INST_IRQ_HAS_NAME(0, done), "no inst err irq");
	zassert_false(DT_INST_IRQ_HAS_NAME(0, alpha), "no inst err irq");

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
	zassert_true(DT_NODE_HAS_PROP(TEST_PH, ph), "ph");
	/* DT_PROP_BY_PHANDLE */
	zassert_true(!strcmp(ph_label, "TEST_GPIO_1"), "ph label");

	/* phandles */
	zassert_true(DT_NODE_HAS_PROP(TEST_PH, phs), "phs");
	zassert_equal(ARRAY_SIZE(phs_labels), 3, "phs size");
	zassert_equal(DT_PROP_LEN(TEST_PH, phs), 3, "phs len");

	/* DT_PROP_BY_PHANDLE_IDX */
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, phs, 0, label),
		     "TEST_GPIO_1"), "phs 0");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, phs, 1, label),
		     "TEST_GPIO_2"), "phs 1");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, phs, 2, label),
		     "TEST_I2C_CTLR"), "phs 2");
	zassert_true(!strcmp(phs_labels[0], "TEST_GPIO_1"), "phs_labels[0]");
	zassert_true(!strcmp(phs_labels[1], "TEST_GPIO_2"), "phs_labels[1]");
	zassert_true(!strcmp(phs_labels[2], "TEST_I2C_CTLR"), "phs_labels[2]");
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 0,
					     gpio_controller),
		      1,
		      "gpios[0].gpio_controller");
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 1,
					     gpio_controller),
		      1,
		      "gpios[1].gpio_controller");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 0, label),
			     "TEST_GPIO_1"),
		     "gpios[0].label");
	zassert_true(!strcmp(DT_PROP_BY_PHANDLE_IDX(TEST_PH, gpios, 1, label),
			     "TEST_GPIO_2"),
		     "gpios[1].label");

	/* phandle-array */
	zassert_true(DT_NODE_HAS_PROP(TEST_PH, gpios), "gpios");
	zassert_equal(ARRAY_SIZE(gps), 2, "gpios size");
	zassert_equal(DT_PROP_LEN(TEST_PH, gpios), 2, "pha len");

	/* DT_PROP_HAS_IDX */
	zassert_true(DT_PROP_HAS_IDX(TEST_PH, gpios, 0), "has idx 0");
	zassert_true(DT_PROP_HAS_IDX(TEST_PH, gpios, 1), "has idx 1");
	zassert_false(DT_PROP_HAS_IDX(TEST_PH, gpios, 2), "!has idx 2");

	/* DT_PHA_HAS_CELL_AT_IDX */
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, gpios, 1, pin),
		     "has gpio 1 pin");
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, gpios, 1, flags),
		     "has gpio 1 flags");
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 1, pin),
		      "no pha-gpio 1 pin");
	/* index 1 only has pin, no flags */
	zassert_false(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 1, flags),
		      "no pha-gpio 1 flags");
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 2, pin),
		      "no pha-gpio 2 pin");
	zassert_true(DT_PHA_HAS_CELL_AT_IDX(TEST_PH, pha_gpios, 2, flags),
		      "no pha-gpio 2 flags");

	/* DT_PHA_HAS_CELL */
	zassert_true(DT_PHA_HAS_CELL(TEST_PH, gpios, flags), "has gpio flags");
	zassert_false(DT_PHA_HAS_CELL(TEST_PH, gpios, bar), "no gpio bar");

	/* DT_PHANDLE_BY_IDX */
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE_BY_IDX(TEST_PH, gpios, 0)),
			     "TEST_GPIO_1"),
		     "gpios 0");
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE_BY_IDX(TEST_PH, gpios, 1)),
			     "TEST_GPIO_2"),
		     "gpios 1");

	/* DT_PHANDLE */
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE(TEST_PH, gpios)),
			     "TEST_GPIO_1"),
		     "gpios");

	/* DT_PHA */
	zassert_equal(DT_PHA(TEST_PH, gpios, pin), 10, "pin 0 implicit");
	zassert_equal(DT_PHA(TEST_PH, gpios, flags), 20, "flags 0 implicit");

	/* DT_PHA_BY_IDX */
	zassert_equal(DT_PHA_BY_IDX(TEST_PH, gpios, 0, pin), 10, "pin 0");
	zassert_equal(DT_PHA_BY_IDX(TEST_PH, gpios, 0, flags), 20, "flags 0");

	zassert_equal(DT_PHA_BY_IDX(TEST_PH, gpios, 1, pin), 30, "pin 1");
	zassert_equal(DT_PHA_BY_IDX(TEST_PH, gpios, 1, flags), 40, "flags 1");

	/* DT_PHA_BY_NAME */
	zassert_equal(DT_PHA_BY_NAME(TEST_PH, foos, a, foocell), 100,
		      "foocell A");
	zassert_equal(DT_PHA_BY_NAME(TEST_PH, foos, b_c, foocell), 110,
		      "foocell b-c");

	/* DT_PHANDLE_BY_NAME */
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE_BY_NAME(TEST_PH, foos, a)),
			     "TEST_GPIO_1"),
		      "phandle A");
	zassert_true(!strcmp(DT_LABEL(DT_PHANDLE_BY_NAME(TEST_PH, foos, b_c)),
			     "TEST_GPIO_2"),
		      "phandle b-c");

	/* array initializers */
	zassert_true(!strcmp(gps[0].name, "TEST_GPIO_1"), "gps[0].name");
	zassert_equal(gps[0].pin, 10, "gps[0].pin");
	zassert_equal(gps[0].flags, 20, "gps[0].flags");

	zassert_true(!strcmp(gps[1].name, "TEST_GPIO_2"), "gps[1].name");
	zassert_equal(gps[1].pin, 30, "gps[1].pin");
	zassert_equal(gps[1].flags, 40, "gps[1].flags");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1,
		      "one instance");

	/* DT_INST_PROP_BY_PHANDLE */
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE(0, ph, label),
			     "TEST_GPIO_1"), "ph label");

	zassert_true(!strcmp(ph_label, "TEST_GPIO_1"), "ph label");

	/* DT_INST_PROP_BY_PHANDLE_IDX */
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, phs, 0, label),
		     "TEST_GPIO_1"), "phs 0");
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, phs, 1, label),
		     "TEST_GPIO_2"), "phs 1");
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, phs, 2, label),
		     "TEST_I2C_CTLR"), "phs 2");
	zassert_true(!strcmp(phs_labels[0], "TEST_GPIO_1"), "phs_labels[0]");
	zassert_true(!strcmp(phs_labels[1], "TEST_GPIO_2"), "phs_labels[1]");
	zassert_true(!strcmp(phs_labels[2], "TEST_I2C_CTLR"), "phs_labels[2]");
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 0,
					     gpio_controller),
		      1,
		      "gpios[0].gpio_controller");
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 1,
					     gpio_controller),
		      1,
		      "gpios[1].gpio_controller");
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 0, label),
			     "TEST_GPIO_1"),
		     "gpios[0].label");
	zassert_true(!strcmp(DT_INST_PROP_BY_PHANDLE_IDX(0, gpios, 1, label),
			     "TEST_GPIO_2"),
		     "gpios[1].label");

	/* DT_INST_PROP_HAS_IDX */
	zassert_true(DT_INST_PROP_HAS_IDX(0, gpios, 0), "has idx 0");
	zassert_true(DT_INST_PROP_HAS_IDX(0, gpios, 1), "has idx 1");
	zassert_false(DT_INST_PROP_HAS_IDX(0, gpios, 2), "!has idx 2");

	/* DT_INST_PHA_HAS_CELL_AT_IDX */
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, gpios, 1, pin),
		     "has gpio 1 pin");
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, gpios, 1, flags),
		     "has gpio 1 flags");
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 1, pin),
		      "no pha-gpio 1 pin");
	/* index 1 only has pin, no flags */
	zassert_false(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 1, flags),
		      "no pha-gpio 1 flags");
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 2, pin),
		      "no pha-gpio 2 pin");
	zassert_true(DT_INST_PHA_HAS_CELL_AT_IDX(0, pha_gpios, 2, flags),
		      "no pha-gpio 2 flags");

	/* DT_INST_PHA_HAS_CELL */
	zassert_true(DT_INST_PHA_HAS_CELL(0, gpios, flags), "has gpio flags");
	zassert_false(DT_INST_PHA_HAS_CELL(0, gpios, bar), "no gpio bar");

	/* DT_INST_PHANDLE_BY_IDX */
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE_BY_IDX(0, gpios, 0)),
			     "TEST_GPIO_1"),
		     "gpios 0");
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE_BY_IDX(0, gpios, 1)),
			     "TEST_GPIO_2"),
		     "gpios 1");

	/* DT_INST_PHANDLE */
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE(0, gpios)),
			     "TEST_GPIO_1"),
		     "gpios");

	/* DT_INST_PHA */
	zassert_equal(DT_INST_PHA(0, gpios, pin), 10, "pin 0 implicit");
	zassert_equal(DT_INST_PHA(0, gpios, flags), 20, "flags 0 implicit");

	/* DT_INST_PHA_BY_IDX */
	zassert_equal(DT_INST_PHA_BY_IDX(0, gpios, 0, pin), 10, "pin 0");
	zassert_equal(DT_INST_PHA_BY_IDX(0, gpios, 0, flags), 20, "flags 0");

	zassert_equal(DT_INST_PHA_BY_IDX(0, gpios, 1, pin), 30, "pin 1");
	zassert_equal(DT_INST_PHA_BY_IDX(0, gpios, 1, flags), 40, "flags 1");

	/* DT_INST_PHA_BY_NAME */
	zassert_equal(DT_INST_PHA_BY_NAME(0, foos, a, foocell), 100,
		      "foocell A");
	zassert_equal(DT_INST_PHA_BY_NAME(0, foos, b_c, foocell), 110,
		      "foocell b-c");

	/* DT_INST_PHANDLE_BY_NAME */
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE_BY_NAME(0, foos, a)),
			     "TEST_GPIO_1"),
		      "phandle A");
	zassert_true(!strcmp(DT_LABEL(DT_INST_PHANDLE_BY_NAME(0, foos, b_c)),
			     "TEST_GPIO_2"),
		      "phandle b-c");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_phandle_holder
static void test_gpio(void)
{

	/* DT_GPIO_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_GPIO_LABEL_BY_IDX(TEST_PH, gpios, 0),
			     "TEST_GPIO_1"),
		     "gpio 0 name idx");
	zassert_true(!strcmp(DT_GPIO_LABEL_BY_IDX(TEST_PH, gpios, 1),
			     "TEST_GPIO_2"),
		     "gpio 1 name idx");

	/* DT_GPIO_LABEL */
	zassert_true(!strcmp(DT_GPIO_LABEL(TEST_PH, gpios), "TEST_GPIO_1"),
		     "gpio 0 name");

	/* DT_GPIO_PIN_BY_IDX */
	zassert_equal(DT_GPIO_PIN_BY_IDX(TEST_PH, gpios, 0), 10, "gpio 0 pin idx");
	zassert_equal(DT_GPIO_PIN_BY_IDX(TEST_PH, gpios, 1), 30, "gpio 1 pin idx");

	/* DT_GPIO_PIN */
	zassert_equal(DT_GPIO_PIN(TEST_PH, gpios), 10, "gpio 0 pin");

	/* DT_GPIO_FLAGS_BY_IDX */
	zassert_equal(DT_GPIO_FLAGS_BY_IDX(TEST_PH, gpios, 0),
		      20, "gpio 0 flags idx");
	zassert_equal(DT_GPIO_FLAGS_BY_IDX(TEST_PH, gpios, 1),
		      40, "gpio 1 flags idx");

	/* DT_GPIO_FLAGS */
	zassert_equal(DT_GPIO_FLAGS(TEST_PH, gpios), 20, "gpio 0 flags");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1,
		      "one instance");

	/* DT_INST_GPIO_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_INST_GPIO_LABEL_BY_IDX(0, gpios, 0),
			     "TEST_GPIO_1"),
		     "gpio inst 0 name idx");
	zassert_true(!strcmp(DT_INST_GPIO_LABEL_BY_IDX(0, gpios, 1),
			     "TEST_GPIO_2"),
		     "gpio inst 1 name idx");

	/* DT_INST_GPIO_LABEL */
	zassert_true(!strcmp(DT_INST_GPIO_LABEL(0, gpios), "TEST_GPIO_1"),
		     "gpio inst 0 name");

	/* DT_INST_GPIO_PIN_BY_IDX */
	zassert_equal(DT_INST_GPIO_PIN_BY_IDX(0, gpios, 0),
		      10, "gpio inst 0 pin idx");
	zassert_equal(DT_INST_GPIO_PIN_BY_IDX(0, gpios, 1),
		      30, "gpio inst 1 pin idx");

	/* DT_INST_GPIO_PIN */
	zassert_equal(DT_INST_GPIO_PIN(0, gpios), 10, "gpio inst 0 pin");

	/* DT_INST_GPIO_FLAGS_BY_IDX */
	zassert_equal(DT_INST_GPIO_FLAGS_BY_IDX(0, gpios, 0),
		      20, "gpio inst 0 flags idx");
	zassert_equal(DT_INST_GPIO_FLAGS_BY_IDX(0, gpios, 1),
		      40, "gpio inst 1 flags idx");

	/* DT_INST_GPIO_FLAGS */
	zassert_equal(DT_INST_GPIO_FLAGS(0, gpios), 20, "gpio inst 0 flags");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
static void test_io_channels(void)
{
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL_BY_IDX(TEST_TEMP, 0),
			     "TEST_ADC_1"),
		     "label 0");
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL_BY_IDX(TEST_TEMP, 1),
			     "TEST_ADC_2"),
		     "label 1");
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL_BY_NAME(TEST_TEMP, ch1),
			     "TEST_ADC_1"),
		     "label ch1");
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL_BY_NAME(TEST_TEMP, ch2),
			     "TEST_ADC_2"),
		     "label ch2");
	zassert_true(!strcmp(DT_IO_CHANNELS_LABEL(TEST_TEMP),
			     "TEST_ADC_1"),
		     "label 0 implicit");

	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL_BY_IDX(0, 0),
			     "TEST_ADC_1"),
		     "inst label 0");
	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL_BY_IDX(0, 1),
			     "TEST_ADC_2"),
		     "inst label 1");
	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL_BY_NAME(0, ch1),
			     "TEST_ADC_1"),
		     "inst ch1");
	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL_BY_NAME(0, ch2),
			     "TEST_ADC_2"),
		     "inst ch2");
	zassert_true(!strcmp(DT_INST_IO_CHANNELS_LABEL(0),
			     "TEST_ADC_1"),
		     "inst ch1 implicit");

	zassert_equal(DT_IO_CHANNELS_INPUT_BY_IDX(TEST_TEMP, 0), 10, "input 0");
	zassert_equal(DT_IO_CHANNELS_INPUT_BY_IDX(TEST_TEMP, 1), 20, "input 1");
	zassert_equal(DT_IO_CHANNELS_INPUT_BY_NAME(TEST_TEMP, ch1), 10,
		      "input ch1");
	zassert_equal(DT_IO_CHANNELS_INPUT_BY_NAME(TEST_TEMP, ch2), 20,
		      "input ch2");
	zassert_equal(DT_IO_CHANNELS_INPUT(TEST_TEMP), 10, "input 0 implicit");

	zassert_equal(DT_INST_IO_CHANNELS_INPUT_BY_IDX(0, 0), 10, "input 0");
	zassert_equal(DT_INST_IO_CHANNELS_INPUT_BY_IDX(0, 1), 20, "input 1");
	zassert_equal(DT_INST_IO_CHANNELS_INPUT_BY_NAME(0, ch1), 10, "input ch1");
	zassert_equal(DT_INST_IO_CHANNELS_INPUT_BY_NAME(0, ch2), 20, "input ch2");
	zassert_equal(DT_INST_IO_CHANNELS_INPUT(0), 10, "input 0 implicit");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
static void test_dma(void)
{
	zassert_true(!strcmp(DT_DMAS_LABEL_BY_NAME(TEST_TEMP, rx),
			     "TEST_DMA_CTRL_2"),
		     "dma-controller label rx channel");
	zassert_true(!strcmp(DT_INST_DMAS_LABEL_BY_NAME(0, rx),
			     "TEST_DMA_CTRL_2"),
		     "dma-controller label rx channel");
	zassert_true(!strcmp(DT_DMAS_LABEL_BY_NAME(TEST_TEMP, tx),
			     "TEST_DMA_CTRL_1"),
		     "dma-controller label tx channel");
	zassert_true(!strcmp(DT_INST_DMAS_LABEL_BY_NAME(0, tx),
			     "TEST_DMA_CTRL_1"),
		     "dma-controller label tx channel");

	zassert_true(!strcmp(DT_DMAS_LABEL_BY_IDX(TEST_TEMP, 1),
			     "TEST_DMA_CTRL_2"),
		     "dma-controller label channel idx 1");
	zassert_true(!strcmp(DT_INST_DMAS_LABEL_BY_IDX(0, 1),
			     "TEST_DMA_CTRL_2"),
		     "dma-controller label channel idx 1");
	zassert_true(!strcmp(DT_DMAS_LABEL_BY_IDX(TEST_TEMP, 0),
			     "TEST_DMA_CTRL_1"),
		     "dma-controller label channel idx 0");
	zassert_true(!strcmp(DT_INST_DMAS_LABEL_BY_IDX(0, 0),
			     "TEST_DMA_CTRL_1"),
		     "dma-controller label channel idx 0");

	zassert_equal(DT_DMAS_CELL_BY_NAME(TEST_TEMP, rx, channel), 3,
		      "channel cell of rx dma channel");
	zassert_equal(DT_INST_DMAS_CELL_BY_NAME(0, rx, channel), 3,
		      "channel cell of rx dma channel");
	zassert_equal(DT_DMAS_CELL_BY_NAME(TEST_TEMP, rx, slot), 4,
		      "slot cell of rx dma channel");
	zassert_equal(DT_INST_DMAS_CELL_BY_NAME(0, rx, slot), 4,
		      "slot cell of rx dma channel");

	zassert_equal(DT_DMAS_CELL_BY_IDX(TEST_TEMP, 1, channel), 3,
		      "channel cell of idx 1 dma channel");
	zassert_equal(DT_INST_DMAS_CELL_BY_IDX(0, 1, channel), 3,
		      "channel cell of idx 1 dma channel");
	zassert_equal(DT_DMAS_CELL_BY_IDX(TEST_TEMP, 1, slot), 4,
		      "slot cell of idx 1 dma channel");
	zassert_equal(DT_INST_DMAS_CELL_BY_IDX(0, 1, slot), 4,
		      "slot cell of idx 1 dma channel");

	zassert_true(DT_DMAS_HAS_NAME(TEST_TEMP, tx),
		     "tx dma channel available");
	zassert_true(DT_INST_DMAS_HAS_NAME(0, tx),
		     "tx dma channel available");
	zassert_false(DT_DMAS_HAS_NAME(TEST_TEMP, output),
		      "output dma channel not available");
	zassert_false(DT_INST_DMAS_HAS_NAME(0, output),
		      "output dma channel not available");

	zassert_true(DT_DMAS_HAS_IDX(TEST_TEMP, 1),
		     "Idx 1 dma channel available");
	zassert_true(DT_INST_DMAS_HAS_IDX(0, 1),
		     "Idx 1 dma channel available");
	zassert_false(DT_DMAS_HAS_IDX(TEST_TEMP, 2),
		      "Idx 2 dma channel not available");
	zassert_false(DT_INST_DMAS_HAS_IDX(0, 2),
		      "Idx 2 dma channel not available");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_phandle_holder
static void test_pwms(void)
{
	/* DT_PWMS_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_PWMS_LABEL_BY_IDX(TEST_PH, 0),
			     "TEST_PWM_CTRL_1"),
		     "label 0");

	/* DT_PWMS_LABEL_BY_NAME */
	zassert_true(!strcmp(DT_PWMS_LABEL_BY_NAME(TEST_PH, red),
			     "TEST_PWM_CTRL_1"),
		     "label red");

	/* DT_PWMS_LABEL */
	zassert_true(!strcmp(DT_PWMS_LABEL(TEST_PH), "TEST_PWM_CTRL_1"),
		     "label 0");

	/* DT_PWMS_CELL_BY_IDX */
	zassert_equal(DT_PWMS_CELL_BY_IDX(TEST_PH, 1, channel), 5,
		      "pwm 2 channel");
	zassert_equal(DT_PWMS_CELL_BY_IDX(TEST_PH, 1, period), 100,
		      "pwm 2 period");
	zassert_equal(DT_PWMS_CELL_BY_IDX(TEST_PH, 1, flags), 1,
		      "pwm 2 flags");

	/* DT_PWMS_CELL_BY_NAME */
	zassert_equal(DT_PWMS_CELL_BY_NAME(TEST_PH, red, channel), 8,
		      "pwm-red channel");
	zassert_equal(DT_PWMS_CELL_BY_NAME(TEST_PH, red, period), 200,
		      "pwm-red period");
	zassert_equal(DT_PWMS_CELL_BY_NAME(TEST_PH, red, flags), 3,
		      "pwm-red flags");

	/* DT_PWMS_CELL */
	zassert_equal(DT_PWMS_CELL(TEST_PH, channel), 8, "pwm channel");
	zassert_equal(DT_PWMS_CELL(TEST_PH, period), 200, "pwm period");
	zassert_equal(DT_PWMS_CELL(TEST_PH, flags), 3, "pwm flags");

	/* DT_PWMS_CHANNEL_BY_IDX */
	zassert_equal(DT_PWMS_CHANNEL_BY_IDX(TEST_PH, 1), 5, "pwm channel");

	/* DT_PWMS_CHANNEL_BY_NAME */
	zassert_equal(DT_PWMS_CHANNEL_BY_NAME(TEST_PH, green), 5,
		      "pwm channel");

	/* DT_PWMS_CHANNEL */
	zassert_equal(DT_PWMS_CHANNEL(TEST_PH), 8, "pwm channel");

	/* DT_PWMS_PERIOD_BY_IDX */
	zassert_equal(DT_PWMS_PERIOD_BY_IDX(TEST_PH, 1), 100, "pwm period");

	/* DT_PWMS_PERIOD_BY_NAME */
	zassert_equal(DT_PWMS_PERIOD_BY_NAME(TEST_PH, green), 100,
		      "pwm period");

	/* DT_PWMS_PERIOD */
	zassert_equal(DT_PWMS_PERIOD(TEST_PH), 200, "pwm period");

	/* DT_PWMS_FLAGS_BY_IDX */
	zassert_equal(DT_PWMS_FLAGS_BY_IDX(TEST_PH, 1), 1, "pwm flags");

	/* DT_PWMS_FLAGS_BY_NAME */
	zassert_equal(DT_PWMS_FLAGS_BY_NAME(TEST_PH, green), 1,
		      "pwm flags");

	/* DT_PWMS_FLAGS */
	zassert_equal(DT_PWMS_FLAGS(TEST_PH), 3, "pwm flags");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1,
		      "one instance");

	/* DT_INST_PWMS_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_INST_PWMS_LABEL_BY_IDX(0, 0),
			     "TEST_PWM_CTRL_1"),
		     "label 0");

	/* DT_INST_PWMS_LABEL_BY_NAME */
	zassert_true(!strcmp(DT_INST_PWMS_LABEL_BY_NAME(0, green),
			     "TEST_PWM_CTRL_2"),
		     "label green");

	/* DT_INST_PWMS_LABEL */
	zassert_true(!strcmp(DT_INST_PWMS_LABEL(0), "TEST_PWM_CTRL_1"),
		     "label 0");

	/* DT_INST_PWMS_CELL_BY_IDX */
	zassert_equal(DT_INST_PWMS_CELL_BY_IDX(0, 1, channel), 5,
		      "pwm 2 channel");
	zassert_equal(DT_INST_PWMS_CELL_BY_IDX(0, 1, period), 100,
		      "pwm 2 period");
	zassert_equal(DT_INST_PWMS_CELL_BY_IDX(0, 1, flags), 1,
		      "pwm 2 flags");

	/* DT_INST_PWMS_CELL_BY_NAME */
	zassert_equal(DT_INST_PWMS_CELL_BY_NAME(0, green, channel), 5,
		      "pwm-green channel");
	zassert_equal(DT_INST_PWMS_CELL_BY_NAME(0, green, period), 100,
		      "pwm-green period");
	zassert_equal(DT_INST_PWMS_CELL_BY_NAME(0, green, flags), 1,
		      "pwm-green flags");

	/* DT_INST_PWMS_CELL */
	zassert_equal(DT_INST_PWMS_CELL(0, channel), 8, "pwm channel");
	zassert_equal(DT_INST_PWMS_CELL(0, period), 200, "pwm period");
	zassert_equal(DT_INST_PWMS_CELL(0, flags), 3, "pwm flags");

	/* DT_INST_PWMS_CHANNEL_BY_IDX */
	zassert_equal(DT_INST_PWMS_CHANNEL_BY_IDX(0, 1), 5, "pwm channel");

	/* DT_INST_PWMS_CHANNEL_BY_NAME */
	zassert_equal(DT_INST_PWMS_CHANNEL_BY_NAME(0, green), 5, "pwm channel");

	/* DT_INST_PWMS_CHANNEL */
	zassert_equal(DT_INST_PWMS_CHANNEL(0), 8, "pwm channel");

	/* DT_INST_PWMS_PERIOD_BY_IDX */
	zassert_equal(DT_INST_PWMS_PERIOD_BY_IDX(0, 1), 100, "pwm period");

	/* DT_INST_PWMS_PERIOD_BY_NAME */
	zassert_equal(DT_INST_PWMS_PERIOD_BY_NAME(0, red), 200, "pwm periodx");

	/* DT_INST_PWMS_PERIOD */
	zassert_equal(DT_INST_PWMS_PERIOD(0), 200, "pwm period");

	/* DT_INST_PWMS_FLAGS_BY_IDX */
	zassert_equal(DT_INST_PWMS_FLAGS_BY_IDX(0, 1), 1, "pwm channel");

	/* DT_INST_PWMS_FLAGS_BY_NAME */
	zassert_equal(DT_INST_PWMS_FLAGS_BY_NAME(0, red), 3, "pwm channel");

	/* DT_INST_PWMS_FLAGS */
	zassert_equal(DT_INST_PWMS_FLAGS(0), 3, "pwm channel");
}

static void test_macro_names(void)
{
	/* white box */
	zassert_true(!strcmp(TO_STRING(DT_PATH(test, gpio_deadbeef)),
			     "DT_N_S_test_S_gpio_deadbeef"),
		     "path");
	zassert_true(!strcmp(TO_STRING(DT_ALIAS(test_alias)),
			     "DT_N_S_test_S_gpio_deadbeef"),
		     "alias");
	zassert_true(!strcmp(TO_STRING(DT_NODELABEL(test_nodelabel)),
			     "DT_N_S_test_S_gpio_deadbeef"),
		     "nodelabel");
	zassert_true(!strcmp(TO_STRING(DT_NODELABEL(test_nodelabel_allcaps)),
			     "DT_N_S_test_S_gpio_deadbeef"),
		     "nodelabel (all caps)");

#define CHILD_NODE_ID DT_CHILD(DT_PATH(test, i2c_11112222), test_i2c_dev_10)
#define FULL_PATH_ID DT_PATH(test, i2c_11112222, test_i2c_dev_10)

	zassert_true(!strcmp(TO_STRING(CHILD_NODE_ID), TO_STRING(FULL_PATH_ID)),
		     "child");

#undef CHILD_NODE_ID
#undef FULL_PATH_ID
}

static int a[] = DT_PROP(TEST_ARRAYS, a);
static unsigned char b[] = DT_PROP(TEST_ARRAYS, b);
static char *c[] = DT_PROP(TEST_ARRAYS, c);

static void test_arrays(void)
{
	zassert_equal(ARRAY_SIZE(a), 3, "a size");
	zassert_equal(ARRAY_SIZE(b), 4, "b size");
	zassert_equal(ARRAY_SIZE(c), 2, "c size");

	zassert_equal(a[0], 1000, "a[0]");
	zassert_equal(a[1], 2000, "a[1]");
	zassert_equal(a[2], 3000, "a[2]");

	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 0), "a idx 0");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 1), "a idx 1");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 2), "a idx 2");
	zassert_false(DT_PROP_HAS_IDX(TEST_ARRAYS, a, 3), "!a idx 3");

	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, a, 0), a[0], "a 0");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, a, 1), a[1], "a 1");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, a, 2), a[2], "a 2");

	zassert_equal(DT_PROP_LEN(TEST_ARRAYS, a), 3, "a len");

	zassert_equal(b[0], 0xaa, "b[0]");
	zassert_equal(b[1], 0xbb, "b[1]");
	zassert_equal(b[2], 0xcc, "b[2]");
	zassert_equal(b[3], 0xdd, "b[3]");

	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 0), "b idx 0");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 1), "b idx 1");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 2), "b idx 2");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 3), "b idx 3");
	zassert_false(DT_PROP_HAS_IDX(TEST_ARRAYS, b, 4), "!b idx 4");

	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, b, 0), b[0], "b 0");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, b, 1), b[1], "b 0");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, b, 2), b[2], "b 0");
	zassert_equal(DT_PROP_BY_IDX(TEST_ARRAYS, b, 3), b[3], "b 0");

	zassert_equal(DT_PROP_LEN(TEST_ARRAYS, b), 4, "b len");

	zassert_true(!strcmp(c[0], "bar"), "c[0]");
	zassert_true(!strcmp(c[1], "baz"), "c[1]");

	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, c, 0), "c idx 0");
	zassert_true(DT_PROP_HAS_IDX(TEST_ARRAYS, c, 1), "c idx 1");
	zassert_false(DT_PROP_HAS_IDX(TEST_ARRAYS, c, 2), "!c idx 2");

	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ARRAYS, c, 0), c[0]), "c 0");
	zassert_true(!strcmp(DT_PROP_BY_IDX(TEST_ARRAYS, c, 1), c[1]), "c 1");

	zassert_equal(DT_PROP_LEN(TEST_ARRAYS, c), 2, "c len");
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
	DEVICE_AND_API_INIT(test_gpio_dev_##num,		\
			    DT_LABEL(INST(num)),		\
			    test_gpio_init,			\
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

	zassert_equal(DT_NUM_INST_STATUS_OKAY(vnd_gpio), 2,
		      "wrong number of gpio devs");

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

	zassert_not_null(devs[0], "null devs[0]");
	zassert_not_null(devs[1], "null devs[1]");
	zassert_true(devs[2] == NULL, "non-null devs[2]");

	zassert_true(to_data(devs[0])->is_gpio_ctlr, "devs[0] not a gpio");
	zassert_true(to_data(devs[1])->is_gpio_ctlr, "devs[1] not a gpio");
	zassert_true(to_data(devs[0])->init_called, "devs[0] not initialized");
	zassert_true(to_data(devs[1])->init_called, "devs[1] not initialized");

	dev_abcd = device_get_binding(DT_LABEL(TEST_ABCD1234));
	zassert_not_null(dev_abcd, "abcd");
	zassert_equal(to_info(dev_abcd)->reg_addr, 0xabcd1234, "abcd addr");
	zassert_equal(to_info(dev_abcd)->reg_len, 0x500, "abcd len");

	/*
	 * Make sure DT_INST_FOREACH_STATUS_OKAY can be called from functions
	 * using macros with side effects in the current scope.
	 */
	val = 0;
#define INC(inst_ignored) do { val++; } while (0);
	DT_INST_FOREACH_STATUS_OKAY(INC)
	zassert_equal(val, 2, "foreach okay vnd_gpio");

	/*
	 * Make sure DT_INST_FOREACH_STATUS_OKAY works with 0 instances, and does
	 * not expand its argument at all.
	 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT xxxx
#define BUILD_BUG_ON_EXPANSION (there is a bug in devicetree.h)
	DT_INST_FOREACH_STATUS_OKAY(BUILD_BUG_ON_EXPANSION)
}

static void test_cs_gpios(void)
{
	zassert_equal(DT_SPI_HAS_CS_GPIOS(TEST_SPI_NO_CS), 0, "unexpected cs");
	zassert_equal(DT_SPI_NUM_CS_GPIOS(TEST_SPI_NO_CS), 0, "wrong no. of cs");

	zassert_equal(DT_SPI_HAS_CS_GPIOS(TEST_SPI), 1, "missing cs");
	zassert_equal(DT_SPI_NUM_CS_GPIOS(TEST_SPI), 3, "wrong no. of cs");

	zassert_true(!strcmp(DT_SPI_DEV_CS_GPIOS_LABEL(TEST_SPI_DEV_0),
			     "TEST_GPIO_1"),
		     "dev 0 cs gpio name");
	zassert_equal(DT_SPI_DEV_CS_GPIOS_PIN(TEST_SPI_DEV_0), 0x10,
		      "dev 0 cs gpio pin");
	zassert_equal(DT_SPI_DEV_CS_GPIOS_FLAGS(TEST_SPI_DEV_0), 0x20,
		      "dev 0 cs gpio flags");
}

static void test_chosen(void)
{
	zassert_equal(DT_HAS_CHOSEN(ztest_xxxx), 0, "ztest_xxxx");
	zassert_equal(DT_HAS_CHOSEN(ztest_gpio), 1, "ztest_gpio");
	zassert_true(!strcmp(TO_STRING(DT_CHOSEN(ztest_gpio)),
			     "DT_N_S_test_S_gpio_deadbeef"),
		     "chosen");
}

static void test_enums(void)
{
	zassert_equal(DT_ENUM_IDX(DT_NODELABEL(test_enum_0), val), 0, "0");
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT vnd_adc_temp_sensor
static void test_clocks(void)
{
	/* DT_CLOCKS_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_CLOCKS_LABEL_BY_IDX(TEST_TEMP, 0),
			     "TEST_CLOCK"),
		     "label 0");

	/* DT_CLOCKS_LABEL_BY_NAME */
	zassert_true(!strcmp(DT_CLOCKS_LABEL_BY_NAME(TEST_TEMP, clk_b),
			     "TEST_CLOCK"),
		     "label b");

	/* DT_CLOCKS_LABEL */
	zassert_true(!strcmp(DT_CLOCKS_LABEL(TEST_TEMP), "TEST_CLOCK"),
		     "label 0");

	/* DT_CLOCKS_CELL_BY_IDX */
	zassert_equal(DT_CLOCKS_CELL_BY_IDX(TEST_TEMP, 2, bits), 2,
		      "clk 2 bits");
	zassert_equal(DT_CLOCKS_CELL_BY_IDX(TEST_TEMP, 2, bus), 8, "clk 2 bus");

	/* DT_CLOCKS_CELL_BY_NAME */
	zassert_equal(DT_CLOCKS_CELL_BY_NAME(TEST_TEMP, clk_a, bits), 7,
		      "clk-a bits");
	zassert_equal(DT_CLOCKS_CELL_BY_NAME(TEST_TEMP, clk_b, bus), 8,
		      "clk-b bus");

	/* DT_CLOCKS_CELL */
	zassert_equal(DT_CLOCKS_CELL(TEST_TEMP, bits), 7, "clk bits");
	zassert_equal(DT_CLOCKS_CELL(TEST_TEMP, bus), 3, "clk bus");

	/* clock-freq on fixed clock */
	zassert_equal(DT_PROP_BY_PHANDLE_IDX(TEST_TEMP, clocks, 1,
					     clock_frequency), 25000000,
		      "fixed clk freq");

	/* DT_INST */
	zassert_equal(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT), 1,
		      "one instance");

	/* DT_INST_CLOCKS_LABEL_BY_IDX */
	zassert_true(!strcmp(DT_INST_CLOCKS_LABEL_BY_IDX(0, 0),
			     "TEST_CLOCK"),
		     "label 0");

	/* DT_INST_CLOCKS_LABEL_BY_NAME */
	zassert_true(!strcmp(DT_INST_CLOCKS_LABEL_BY_NAME(0, clk_b),
			     "TEST_CLOCK"),
		     "label b");

	/* DT_INST_CLOCKS_LABEL */
	zassert_true(!strcmp(DT_INST_CLOCKS_LABEL(0), "TEST_CLOCK"),
		     "label 0");

	/* DT_INST_CLOCKS_CELL_BY_IDX */
	zassert_equal(DT_INST_CLOCKS_CELL_BY_IDX(0, 2, bits), 2,
		      "clk 2 bits");
	zassert_equal(DT_INST_CLOCKS_CELL_BY_IDX(0, 2, bus), 8, "clk 2 bus");

	/* DT_INST_CLOCKS_CELL_BY_NAME */
	zassert_equal(DT_INST_CLOCKS_CELL_BY_NAME(0, clk_a, bits), 7,
		      "clk-a bits");
	zassert_equal(DT_INST_CLOCKS_CELL_BY_NAME(0, clk_b, bus), 8,
		      "clk-b bus");

	/* DT_INST_CLOCKS_CELL */
	zassert_equal(DT_INST_CLOCKS_CELL(0, bits), 7, "clk bits");
	zassert_equal(DT_INST_CLOCKS_CELL(0, bus), 3, "clk bus");

	/* clock-freq on fixed clock */
	zassert_equal(DT_INST_PROP_BY_PHANDLE_IDX(0, clocks, 1,
					     clock_frequency), 25000000,
		      "fixed clk freq");
}

static void test_parent(void)
{
	/* The label of a child node's parent is the label of the parent. */
	zassert_true(!strcmp(DT_LABEL(DT_PARENT(TEST_SPI_DEV_0)),
			     DT_LABEL(TEST_SPI_BUS_0)),
		     "dev 0 parent");
	/*
	 * We should be able to use DT_PARENT() even with nodes, like /test,
	 * that have no matching compatible.
	 */
	zassert_true(!strcmp(DT_LABEL(DT_CHILD(DT_PARENT(TEST_SPI_BUS_0),
					       spi_33334444)),
			     DT_LABEL(TEST_SPI_BUS_0)),
		     "round trip through node with no compatible");
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

	zassert_equal(ARRAY_SIZE(vals), 3,
		      "Bad number of children");
	zassert_equal(ARRAY_SIZE(vals_inst), 3,
		      "Bad number of children");

	zassert_false(strlen(STRINGIFY(TEST_PARENT)) == 0,
		      "TEST_PARENT evaluated to empty string");

	zassert_equal(vals[0].val, 0, "Child 0 did not match");
	zassert_equal(vals[1].val, 1, "Child 1 did not match");
	zassert_equal(vals[2].val, 2, "Child 2 did not match");
	zassert_equal(vals_inst[0].val, 0, "Child 0 did not match");
	zassert_equal(vals_inst[1].val, 1, "Child 1 did not match");
	zassert_equal(vals_inst[2].val, 2, "Child 2 did not match");

	#undef TEST_PARENT
	#undef TEST_FUNC
}

static void test_great_grandchild(void)
{
	zassert_equal(DT_PROP(DT_NODELABEL(test_ggc), ggc_prop),
		      42, "great-grandchild bindings returned wrong value");
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
			 ztest_unit_test(test_devices),
			 ztest_unit_test(test_cs_gpios),
			 ztest_unit_test(test_chosen),
			 ztest_unit_test(test_enums),
			 ztest_unit_test(test_clocks),
			 ztest_unit_test(test_parent),
			 ztest_unit_test(test_child_nodes_list),
			 ztest_unit_test(test_great_grandchild)
		);
	ztest_run_test_suite(devicetree_api);
}
