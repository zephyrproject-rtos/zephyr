/*
 * Copyright 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#define SPI_DEV_NODE DT_PROP(DT_PATH(zephyr_user), spi)

static const struct device *const spi_dev = DEVICE_DT_GET(SPI_DEV_NODE);
static const struct gpio_dt_spec cs_pin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), cs_gpios);

static uint8_t tx_buf[8];

static const struct spi_buf tx_buf_pool[1] = {
	{
		.buf = tx_buf,
		.len = sizeof(tx_buf),
	},
};

static const struct spi_buf_set spi_tx_buf = {
	.buffers = tx_buf_pool,
	.count = ARRAY_SIZE(tx_buf_pool),
};

static const struct spi_config config = {
	.frequency = DT_PROP_OR(SPI_DEV_NODE, max_frequency, 1000000),
	.operation = SPI_OP_MODE_MASTER | SPI_HOLD_ON_CS | SPI_WORD_SET(8),
	.slave = 0,
	.cs = {
		.gpio = GPIO_DT_SPEC_GET_OR(SPI_DEV_NODE, cs_gpios, {0}),
		.delay = 0,
	},
};

static int control_cs_pin_with_gpio(void)
{
	int ret;

	ret = gpio_pin_configure_dt(&cs_pin, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		printk("failed to configure CS pin\n");
		return 0;
	}

	k_msleep(1000);

	ret = gpio_pin_configure_dt(&cs_pin, GPIO_INPUT);
	if (ret) {
		printk("failed to configure CS pin\n");
		return 0;
	}

	k_msleep(1000);
	return 0;
}

static int control_cs_pin_with_spi(void)
{
	int ret;

	/*
	 * Perform SPI transaction and keep CS pin asserted after
	 * after transaction (see config.operation).
	 */
	ret = spi_transceive(spi_dev, &config, &spi_tx_buf, NULL);
	if (ret) {
		printk("failed to perform SPI transaction (ret %d)\n", ret);
		return 0;
	}

	k_msleep(1000);

	/* Release SPI device and thus CS pin shall be deasserted */
	ret = spi_release(spi_dev, &config);
	if (ret) {
		printk("failed to release SPI device (ret %d)\n", ret);
		return 0;
	}

	k_msleep(1000);
	return 0;
}

int main(void)
{
	int ret;

	/*
	 * As we have specified deferred init for the SPI device in the devicetree, the driver
	 * will not be initialized at this stage. We should have full control of the CS GPIO,
	 * let's test that.
	 */
	ret = control_cs_pin_with_gpio();
	if (ret) {
		printk("failed to control CS pin with GPIO device driver (ret %d)\n", ret);
		return 0;
	}

	/*
	 * Now lets initialize the SPI device driver.
	 */
	ret = device_init(spi_dev);
	if (ret) {
		printk("failed to init SPI device driver (ret %d)\n", ret);
		return 0;
	}

	/*
	 * To control the CS pin in "human speed", lets perform a mock
	 * transaction, holding the CS pin until we release the SPI device.
	 */
	ret = control_cs_pin_with_spi();
	if (ret) {
		printk("failed to control CS pin with SPI device driver (ret %d)\n", ret);
		return 0;
	}

	/*
	 * Lets deinit the SPI device driver an manually control the
	 * CS pin again.
	 */
	ret = device_deinit(spi_dev);
	if (ret) {
		printk("failed to deinit SPI device driver (ret %d)\n", ret);
		return 0;
	}

	ret = control_cs_pin_with_gpio();
	if (ret) {
		printk("failed to control CS pin with GPIO device driver (ret %d)\n", ret);
		return 0;
	}

	/*
	 * Lastly, lets check if we can bring back the SPI device driver.
	 */
	ret = device_init(spi_dev);
	if (ret) {
		printk("failed to init SPI device driver (ret %d)\n", ret);
		return 0;
	}

	/* One final blink */
	ret = control_cs_pin_with_spi();
	if (ret) {
		printk("failed to control CS pin with SPI device driver (ret %d)\n", ret);
		return 0;
	}

	printk("sample complete\n");
	return 0;
}
