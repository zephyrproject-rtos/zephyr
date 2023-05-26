/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>

#define TEST_GPIO        DT_NODELABEL(test_gpio_0)
#define TEST_I2C         DT_NODELABEL(i2c)
#define TEST_I2C_DEVA    DT_NODELABEL(test_i2c_dev_a)
#define TEST_I2C_DEVB    DT_NODELABEL(test_i2c_dev_b)
#define TEST_I2C_DEVC    DT_NODELABEL(test_i2c_dev_c)
#define TEST_I2C_NOLABEL DT_PATH(test, i2c_11112222, test_i2c_dev_14)
#define TEST_SPI         DT_NODELABEL(spi)
#define TEST_SPI_DEVA    DT_NODELABEL(test_spi_dev_a)
#define TEST_SPI_DEVB    DT_NODELABEL(test_spi_dev_b)
#define TEST_SPI_DEVC    DT_NODELABEL(test_spi_dev_c)

static const struct device *init_order[20];
static uint8_t init_idx;

#define DEF_DRV_INIT(node_id)                                                                      \
	static int DEV_INIT_##node_id(const struct device *dev)                                    \
	{                                                                                          \
		printk("%s %d\n", __func__, init_idx);                                             \
		__ASSERT_NO_MSG(init_idx < ARRAY_SIZE(init_order));                                \
		init_order[init_idx++] = dev;                                                      \
		return 0;                                                                          \
	}

#define DEFINE_DRV(node_id, level)                                                                 \
	DEVICE_DT_DEFINE(node_id, DEV_INIT_##node_id, NULL, NULL, NULL, level, 0, NULL)

DEF_DRV_INIT(TEST_GPIO)
DEF_DRV_INIT(TEST_I2C)
DEF_DRV_INIT(TEST_I2C_DEVA)
DEF_DRV_INIT(TEST_I2C_DEVB)
DEF_DRV_INIT(TEST_I2C_DEVC)
DEF_DRV_INIT(TEST_I2C_NOLABEL)
DEF_DRV_INIT(TEST_SPI)
DEF_DRV_INIT(TEST_SPI_DEVA)
DEF_DRV_INIT(TEST_SPI_DEVB)
DEF_DRV_INIT(TEST_SPI_DEVC)

DEFINE_DRV(TEST_GPIO, PRE_KERNEL_2);
DEFINE_DRV(TEST_I2C, POST_KERNEL);
DEFINE_DRV(TEST_I2C_DEVB, APPLICATION);
DEFINE_DRV(TEST_I2C_DEVC, POST_KERNEL);
DEFINE_DRV(TEST_I2C_DEVA, POST_KERNEL);
DEFINE_DRV(TEST_I2C_NOLABEL, PRE_KERNEL_1);
DEFINE_DRV(TEST_SPI, PRE_KERNEL_2);
DEFINE_DRV(TEST_SPI_DEVB, PRE_KERNEL_1);
DEFINE_DRV(TEST_SPI_DEVA, APPLICATION);
DEFINE_DRV(TEST_SPI_DEVC, PRE_KERNEL_1);

#define DEV_HDL(node_id)   DEVICE_DT_GET(node_id)
#define DEV_HDL_NAME(name) DEVICE_GET(name)

ZTEST(devicetree_devices, test_init_order)
{
	zassert_equal(init_order[0], DEV_HDL(TEST_GPIO));
	zassert_equal(init_order[1], DEV_HDL(TEST_SPI));
	zassert_equal(init_order[2], DEV_HDL(TEST_SPI_DEVB));
	zassert_equal(init_order[3], DEV_HDL(TEST_SPI_DEVC));
	zassert_equal(init_order[4], DEV_HDL(TEST_I2C));
	zassert_equal(init_order[5], DEV_HDL(TEST_I2C_DEVA));
	zassert_equal(init_order[6], DEV_HDL(TEST_I2C_DEVC));
	zassert_equal(init_order[7], DEV_HDL(TEST_I2C_NOLABEL));
	zassert_equal(init_order[8], DEV_HDL(TEST_I2C_DEVB));
	zassert_equal(init_order[9], DEV_HDL(TEST_SPI_DEVA));
}

ZTEST(devicetree_devices, test_get_or_null)
{
	const struct device *dev;

	dev = DEVICE_DT_GET_OR_NULL(TEST_I2C_DEVA);
	zassert_not_equal(dev, NULL, NULL);

	dev = DEVICE_DT_GET_OR_NULL(non_existing_node);
	zassert_is_null(dev);
}

ZTEST_SUITE(devicetree_devices, NULL, NULL, NULL, NULL, NULL);
