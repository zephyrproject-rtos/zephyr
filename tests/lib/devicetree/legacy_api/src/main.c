/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <devicetree.h>

/*
 * We test most common properties (label, reg, interrupts) on just the
 * GPIO node, since they work the same way on all nodes.
 */

static void test_gpio(void)
{
	/* label */
	zassert_equal(DT_VND_GPIO_1000_LABEL,
		      DT_LABEL(DT_PATH(migration, gpio_1000)),
		      "");

	zassert_equal(DT_INST_0_VND_GPIO_LABEL,
		      DT_LABEL(DT_INST(0, vnd_gpio)),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_LABEL,
		      DT_LABEL(DT_ALIAS(mgr_gpio)),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_LABEL,
		      DT_LABEL(DT_NODELABEL(migration_gpio)),
		      "");

	/* reg base address */
	zassert_equal(DT_VND_GPIO_1000_BASE_ADDRESS,
		      DT_REG_ADDR(DT_PATH(migration, gpio_1000)),
		      "");

	zassert_equal(DT_INST_0_VND_GPIO_BASE_ADDRESS,
		      DT_REG_ADDR(DT_INST(0, vnd_gpio)),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_BASE_ADDRESS,
		      DT_REG_ADDR(DT_ALIAS(mgr_gpio)),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_BASE_ADDRESS,
		      DT_REG_ADDR(DT_NODELABEL(migration_gpio)),
		      "");

	/* reg size */
	zassert_equal(DT_VND_GPIO_1000_SIZE,
		      DT_REG_SIZE(DT_PATH(migration, gpio_1000)),
		      "");

	zassert_equal(DT_INST_0_VND_GPIO_SIZE,
		      DT_REG_SIZE(DT_INST(0, vnd_gpio)),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_SIZE,
		      DT_REG_SIZE(DT_ALIAS(mgr_gpio)),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_SIZE,
		      DT_REG_SIZE(DT_NODELABEL(migration_gpio)),
		      "");

	/* irq number */
	zassert_equal(DT_VND_GPIO_1000_IRQ_0,
		      DT_IRQN(DT_PATH(migration, gpio_1000)),
		      "");

	zassert_equal(DT_INST_0_VND_GPIO_IRQ_0,
		      DT_IRQN(DT_INST(0, vnd_gpio)),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_IRQ_0,
		      DT_IRQN(DT_ALIAS(mgr_gpio)),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_IRQ_0,
		      DT_IRQN(DT_NODELABEL(migration_gpio)),
		      "");

	/* irq priority */
	zassert_equal(DT_VND_GPIO_1000_IRQ_0_PRIORITY,
		      DT_IRQ(DT_PATH(migration, gpio_1000), priority),
		      "");

	zassert_equal(DT_INST_0_VND_GPIO_IRQ_0_PRIORITY,
		      DT_IRQ(DT_INST(0, vnd_gpio), priority),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_IRQ_0_PRIORITY,
		      DT_IRQ(DT_ALIAS(mgr_gpio), priority),
		      "");

	zassert_equal(DT_ALIAS_MGR_GPIO_IRQ_0_PRIORITY,
		      DT_IRQ(DT_NODELABEL(migration_gpio), priority),
		      "");
}

/*
 * The serial device is how we test specific properties.
 */

static void test_serial(void)
{
	zassert_equal(DT_VND_SERIAL_3000_BAUD_RATE,
		      DT_PROP(DT_PATH(migration, serial_3000), baud_rate),
		      "");
	zassert_equal(DT_ALIAS_MGR_SERIAL_BAUD_RATE,
		      DT_PROP(DT_ALIAS(mgr_serial), baud_rate),
		      "");
	zassert_equal(DT_ALIAS_MGR_SERIAL_BAUD_RATE,
		      DT_PROP(DT_NODELABEL(migration_serial), baud_rate),
		      "");
	zassert_equal(DT_INST_0_VND_SERIAL_BAUD_RATE,
		      DT_PROP(DT_INST(0, vnd_serial), baud_rate),
		      "");
}

/*
 * The I2C and SPI devices are used to test inter-device relationships.
 */

#define I2C_DEV_PATH		DT_PATH(migration, i2c_10000, i2c_dev_10)
#define I2C_DEV_ALIAS		DT_ALIAS(mgr_i2c_dev)
#define I2C_DEV_NODELABEL	DT_NODELABEL(mgr_i2c_device)
#define I2C_DEV_INST		DT_INST(0, vnd_i2c_device)
static void test_i2c_device(void)
{
	/* Bus controller name */
	zassert_true(!strcmp(DT_VND_I2C_10000_VND_I2C_DEVICE_10_BUS_NAME,
			     DT_LABEL(DT_BUS(I2C_DEV_PATH))),
		     "");
	zassert_true(!strcmp(DT_ALIAS_MGR_I2C_DEV_BUS_NAME,
			     DT_LABEL(DT_BUS(I2C_DEV_ALIAS))),
		     "");
	zassert_true(!strcmp(DT_ALIAS_MGR_I2C_DEV_BUS_NAME,
			     DT_LABEL(DT_BUS(I2C_DEV_NODELABEL))),
		     "");
	zassert_true(!strcmp(DT_INST_0_VND_I2C_DEVICE_BUS_NAME,
			     DT_LABEL(DT_BUS(I2C_DEV_INST))),
		     "");
}

#define SPI_DEV_PATH		DT_PATH(migration, spi_20000, spi_dev_0)
#define SPI_DEV_ALIAS		DT_ALIAS(mgr_spi_dev)
#define SPI_DEV_NODELABEL	DT_NODELABEL(mgr_spi_device)
#define SPI_DEV_INST		DT_INST(0, vnd_spi_device)
static void test_spi_device(void)
{
	/* cs-gpios controller label */
	zassert_true(
		!strcmp(DT_VND_SPI_20000_VND_SPI_DEVICE_0_CS_GPIOS_CONTROLLER,
			DT_SPI_DEV_CS_GPIOS_LABEL(SPI_DEV_PATH)),
		"");
	zassert_true(!strcmp(DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_CONTROLLER,
			     DT_SPI_DEV_CS_GPIOS_LABEL(SPI_DEV_ALIAS)),
		     "");
	zassert_true(!strcmp(DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_CONTROLLER,
			     DT_SPI_DEV_CS_GPIOS_LABEL(SPI_DEV_NODELABEL)),
		     "");
	zassert_true(!strcmp(DT_INST_0_VND_SPI_DEVICE_CS_GPIOS_CONTROLLER,
			     DT_SPI_DEV_CS_GPIOS_LABEL(SPI_DEV_INST)),
		     "");

	/* cs-gpios pin number */
	zassert_equal(DT_VND_SPI_20000_VND_SPI_DEVICE_0_CS_GPIOS_PIN,
		      DT_SPI_DEV_CS_GPIOS_PIN(SPI_DEV_PATH),
		      "");
	zassert_equal(DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_PIN,
		      DT_SPI_DEV_CS_GPIOS_PIN(SPI_DEV_ALIAS),
		      "");
	zassert_equal(DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_PIN,
		      DT_SPI_DEV_CS_GPIOS_PIN(SPI_DEV_NODELABEL),
		      "");
	zassert_equal(DT_INST_0_VND_SPI_DEVICE_CS_GPIOS_PIN,
		      DT_SPI_DEV_CS_GPIOS_PIN(SPI_DEV_INST),
		      "");

	/* cs-gpios GPIO flags */
	zassert_equal(DT_VND_SPI_20000_VND_SPI_DEVICE_0_CS_GPIOS_FLAGS,
		      DT_SPI_DEV_CS_GPIOS_FLAGS(SPI_DEV_PATH),
		      "");
	zassert_equal(DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_FLAGS,
		      DT_SPI_DEV_CS_GPIOS_FLAGS(SPI_DEV_ALIAS),
		      "");
	zassert_equal(DT_ALIAS_MGR_SPI_DEV_CS_GPIOS_FLAGS,
		      DT_SPI_DEV_CS_GPIOS_FLAGS(SPI_DEV_NODELABEL),
		      "");
	zassert_equal(DT_INST_0_VND_SPI_DEVICE_CS_GPIOS_FLAGS,
		      DT_SPI_DEV_CS_GPIOS_FLAGS(SPI_DEV_INST),
		      "");
}

void test_main(void)
{
	ztest_test_suite(devicetree_legacy_api,
			 ztest_unit_test(test_gpio),
			 ztest_unit_test(test_serial),
			 ztest_unit_test(test_i2c_device),
			 ztest_unit_test(test_spi_device)
		);
	ztest_run_test_suite(devicetree_legacy_api);
}
