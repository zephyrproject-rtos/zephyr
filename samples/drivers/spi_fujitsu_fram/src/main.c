/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <spi.h>

/**
 * @file Sample app using the Fujitsu MB85RS64V FRAM through SPI.
 */

#define MB85RS64V_MANUFACTURER_ID_CMD 0x9f
#define MAX_USER_DATA_LENGTH 1024

static uint8_t spi_buffer[MAX_USER_DATA_LENGTH + 3];
static uint8_t data[MAX_USER_DATA_LENGTH], cmp_data[MAX_USER_DATA_LENGTH];

static int mb85rs64v_read_id(struct device *dev)
{
	int err;

	spi_buffer[0] = MB85RS64V_MANUFACTURER_ID_CMD;

	err = spi_transceive(dev, spi_buffer, 5, spi_buffer, 5);
	if (err) {
		printk("Error during ID read\n");
		return -EIO;
	}

	if (spi_buffer[1] != 0x04) {
		return -EIO;
	}

	if (spi_buffer[2] != 0x7f) {
		return -EIO;
	}

	if (spi_buffer[3] != 0x03) {
		return -EIO;
	}

	if (spi_buffer[4] != 0x02) {
		return -EIO;
	}

	return 0;
}

static int write_bytes(struct device *dev, uint16_t addr,
		       uint8_t *data, uint32_t num_bytes)
{
	int err;

	/* write protect disable cmd */
	spi_buffer[0] = 0x06;

	/* disable write protect */
	err = spi_write(dev, spi_buffer, 1);
	if (err) {
		printk("unable to disable write protect\n");
		return -EIO;
	}

	/* write cmd */
	spi_buffer[0] = 0x02;
	spi_buffer[1] = (addr >> 8) & 0xFF;
	spi_buffer[2] = addr & 0xFF;

	for (uint32_t i = 0; i < num_bytes; i++) {
		spi_buffer[i + 3] = data[i];
	}

	err = spi_write(dev, spi_buffer, num_bytes + 3);
	if (err) {
		printk("Error during SPI write\n");
		return -EIO;
	}

	return 0;
}

static int read_bytes(struct device *dev, uint16_t addr,
		      uint8_t *data, uint32_t num_bytes)
{
	int err;

	/* read cmd */
	spi_buffer[0] = 0x03;
	spi_buffer[1] = (addr >> 8) & 0xFF;
	spi_buffer[2] = addr & 0xFF;

	err = spi_transceive(dev, spi_buffer, num_bytes + 3, spi_buffer,
			     num_bytes + 3);
	if (err) {
		printk("Error during SPI read\n");
		return -EIO;
	}

	for (uint32_t i = 0; i < num_bytes; i++) {
		data[i] = spi_buffer[i + 3];
	}

	return 0;
}

void main(void)
{
	struct spi_config config = { 0 };
	struct device *spi_mst_1 = device_get_binding(CONFIG_SPI_1_NAME);
	int err;

	printk("fujitsu FRAM example application\n");

	if (!spi_mst_1) {
		printk("Could not find SPI driver\n");
		return;
	}

	config.config = SPI_WORD(8);
	config.max_sys_freq = 256;

	err = spi_configure(spi_mst_1, &config);
	if (err) {
		printk("SPI configuration failed\n");
		return;
	}

	err = spi_slave_select(spi_mst_1, 2);
	if (err) {
		printk("SPI slave select failed\n");
		return;
	}

	err = mb85rs64v_read_id(spi_mst_1);
	if (err) {
		printk("Could not verify FRAM ID\n");
		return;
	}

	/* Do one-byte read/write */
	data[0] = 0xAE;
	err = write_bytes(spi_mst_1, 0x00, data, 1);
	if (err) {
		printk("Error writing to FRAM! errro code (%d)\n", err);
		return;
	} else {
		printk("Wrote 0xAE to address 0x00.\n");
	}

	data[0] = 0x86;
	err = write_bytes(spi_mst_1, 0x01, data, 1);
	if (err) {
		printk("Error writing to FRAM! error code (%d)\n", err);
		return;
	} else {
		printk("Wrote 0x86 to address 0x01.\n");
	}

	data[0] = 0x00;
	err = read_bytes(spi_mst_1, 0x00, data, 1);
	if (err) {
		printk("Error reading from FRAM! error code (%d)\n", err);
		return;
	} else {
		printk("Read 0x%X from address 0x00.\n", data[0]);
	}

	data[0] = 0x00;
	err = read_bytes(spi_mst_1, 0x01, data, 1);
	if (err) {
		printk("Error reading from FRAM! error code (%d)\n", err);
		return;
	} else {
		printk("Read 0x%X from address 0x01.\n", data[0]);
	}

	/* Do multi-byte read/write */
	/* get some random data, and clear out data[] */
	for (uint32_t i = 0; i < sizeof(cmp_data); i++) {
		cmp_data[i] = k_cycle_get_32() & 0xFF;
		data[i] = 0x00;
	}

	/* write them to the FRAM */
	err = write_bytes(spi_mst_1, 0x00, cmp_data, sizeof(cmp_data));
	if (err) {
		printk("Error writing to FRAM! error code (%d)\n", err);
		return;
	} else {
		printk("Wrote %d bytes to address 0x00.\n",
		       (uint32_t) sizeof(cmp_data));
	}

	err = read_bytes(spi_mst_1, 0x00, data, sizeof(data));
	if (err) {
		printk("Error reading from FRAM! error code (%d)\n", err);
		return;
	} else {
		printk("Read %d bytes from address 0x00.\n",
		       (uint32_t) sizeof(data));
	}

	err = 0;
	for (uint32_t i = 0; i < sizeof(cmp_data); i++) {
		if (cmp_data[i] != data[i]) {
			printk("Data comparison failed @ %d.\n", i);
			err = -EIO;
		}
	}
	if (err == 0) {
		printk("Data comparison successful.\n");
	}
}
