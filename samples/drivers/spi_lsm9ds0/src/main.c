/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>
#include <spi.h>

#include <misc/printk.h>

#define LSM9DS0_WHOAMI_G 0xf
#define LSM9DS0_READ_MASK 0x80

static uint8_t rx_buffer[2], tx_buffer[2];

static uint8_t lsm9ds0_read_whoami_g(struct device *dev)
{
	int err;

	tx_buffer[1] = LSM9DS0_WHOAMI_G | LSM9DS0_READ_MASK;

	err = spi_transceive(dev, tx_buffer, sizeof(tx_buffer),
			     rx_buffer, sizeof(rx_buffer));
	if (err) {
		printk("Error during SPI transfer\n");
		return 0;
	}

	return rx_buffer[0];
}

void main(void)
{
	struct spi_config config = { 0 };
	struct device *spi_mst_0 = device_get_binding("SPI_0");
	uint8_t id;
	int err;

	printk("SPI Example application\n");

	if (!spi_mst_0)
		return;

	config.config = SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD(16);
	config.max_sys_freq = 256;

	err = spi_configure(spi_mst_0, &config);
	if (err) {
		printk("Could not configure SPI device\n");
		return;
	}

	err = spi_slave_select(spi_mst_0, 1);
	if (err) {
		printk("Could not select SPI slave\n");
		return;
	}

	id = lsm9ds0_read_whoami_g(spi_mst_0);

	printk("LSM9DS0 Who Am I: 0x%x\n", id);
}
