/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/spi.h>

/**
 * @file Sample app using the Fujitsu MB85RS64V FRAM through SPI.
 */

#define MB85RS64V_MANUFACTURER_ID_CMD 0x9f
#define MB85RS64V_WRITE_ENABLE_CMD 0x06
#define MB85RS64V_READ_CMD 0x03
#define MB85RS64V_WRITE_CMD 0x02
#define MAX_USER_DATA_LENGTH 1024

static uint8_t data[MAX_USER_DATA_LENGTH], cmp_data[MAX_USER_DATA_LENGTH];

static int mb85rs64v_access(struct device *spi, struct spi_config *spi_cfg,
			    uint8_t cmd, uint16_t addr, void *data, size_t len)
{
	uint8_t access[3];
	struct spi_buf bufs[] = {
		{
			.buf = access,
		},
		{
			.buf = data,
			.len = len
		}
	};
	struct spi_buf_set tx = {
		.buffers = bufs
	};

	access[0] = cmd;

	if (cmd == MB85RS64V_WRITE_CMD || cmd == MB85RS64V_READ_CMD) {
		access[1] = (addr >> 8) & 0xFF;
		access[2] = addr & 0xFF;

		bufs[0].len = 3;
		tx.count = 2;

		if (cmd == MB85RS64V_READ_CMD) {
			struct spi_buf_set rx = {
				.buffers = bufs,
				.count = 2
			};

			return spi_transceive(spi, spi_cfg, &tx, &rx);
		}
	} else {
		tx.count = 1;
	}

	return spi_write(spi, spi_cfg, &tx);
}


static int mb85rs64v_read_id(struct device *spi, struct spi_config *spi_cfg)
{
	uint8_t id[4];
	int err;

	err = mb85rs64v_access(spi, spi_cfg,
			       MB85RS64V_MANUFACTURER_ID_CMD, 0, &id, 4);
	if (err) {
		printk("Error during ID read\n");
		return -EIO;
	}

	if (id[0] != 0x04) {
		return -EIO;
	}

	if (id[1] != 0x7f) {
		return -EIO;
	}

	if (id[2] != 0x03) {
		return -EIO;
	}

	if (id[3] != 0x02) {
		return -EIO;
	}

	return 0;
}

static int write_bytes(struct device *spi, struct spi_config *spi_cfg,
		       uint16_t addr, uint8_t *data, uint32_t num_bytes)
{
	int err;

	/* disable write protect */
	err = mb85rs64v_access(spi, spi_cfg,
			       MB85RS64V_WRITE_ENABLE_CMD, 0, NULL, 0);
	if (err) {
		printk("unable to disable write protect\n");
		return -EIO;
	}

	/* write cmd */
	err = mb85rs64v_access(spi, spi_cfg,
			       MB85RS64V_WRITE_CMD, addr, data, num_bytes);
	if (err) {
		printk("Error during SPI write\n");
		return -EIO;
	}

	return 0;
}

static int read_bytes(struct device *spi, struct spi_config *spi_cfg,
		      uint16_t addr, uint8_t *data, uint32_t num_bytes)
{
	int err;

	/* read cmd */
	err = mb85rs64v_access(spi, spi_cfg,
			       MB85RS64V_READ_CMD, addr, data, num_bytes);
	if (err) {
		printk("Error during SPI read\n");
		return -EIO;
	}

	return 0;
}

void main(void)
{
	struct device *spi;
	struct spi_config spi_cfg = {0};
	int err;

	printk("fujitsu FRAM example application\n");

	spi = device_get_binding(DT_LABEL(DT_ALIAS(spi_1)));
	if (!spi) {
		printk("Could not find SPI driver\n");
		return;
	}

	spi_cfg.operation = SPI_WORD_SET(8);
	spi_cfg.frequency = 256000U;


	err = mb85rs64v_read_id(spi, &spi_cfg);
	if (err) {
		printk("Could not verify FRAM ID\n");
		return;
	}

	/* Do one-byte read/write */
	data[0] = 0xAE;
	err = write_bytes(spi, &spi_cfg, 0x00, data, 1);
	if (err) {
		printk("Error writing to FRAM! errro code (%d)\n", err);
		return;
	} else {
		printk("Wrote 0xAE to address 0x00.\n");
	}

	data[0] = 0x86;
	err = write_bytes(spi, &spi_cfg, 0x01, data, 1);
	if (err) {
		printk("Error writing to FRAM! error code (%d)\n", err);
		return;
	} else {
		printk("Wrote 0x86 to address 0x01.\n");
	}

	data[0] = 0x00;
	err = read_bytes(spi, &spi_cfg, 0x00, data, 1);
	if (err) {
		printk("Error reading from FRAM! error code (%d)\n", err);
		return;
	} else {
		printk("Read 0x%X from address 0x00.\n", data[0]);
	}

	data[0] = 0x00;
	err = read_bytes(spi, &spi_cfg, 0x01, data, 1);
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
	err = write_bytes(spi, &spi_cfg, 0x00, cmp_data, sizeof(cmp_data));
	if (err) {
		printk("Error writing to FRAM! error code (%d)\n", err);
		return;
	} else {
		printk("Wrote %d bytes to address 0x00.\n",
		       (uint32_t) sizeof(cmp_data));
	}

	err = read_bytes(spi, &spi_cfg, 0x00, data, sizeof(data));
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
