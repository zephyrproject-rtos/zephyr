/*
 * Copyright (c) 202 Tridonic GmbH & Co KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT spi_4002c000


#include <shell/shell.h>
#include <stdlib.h>
#include <drivers/spi.h>
#include <string.h>
#include <sys/util.h>
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(SPI_shell, CONFIG_LOG_DEFAULT_LEVEL);

#define SPI_DEVICE_PREFIX "SPI_"
//#define GPIO_DEVICE_PREFIX "GPIO_"

/* Maximum bytes we can write or read at once */
#define MAX_SPI_BYTES	16

//TODO: parse GPIO for CS, put CS pin into output mode, set proper flags on CS pin
//TODO: make SPI frequency configurable? use default of peripheral?
//TODO: implement reading SPI

#define ARGS_BEFORE_BYTES 5 /* spi write <SPI> <GPIO> <pin>*/
/* spi write SPI_x GPIO_x pin [<byte1>, ...] */

static int cmd_spi_write(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t buf[MAX_SPI_BYTES];
	const struct device *spi_dev;
	int num_bytes;
	int i;
	uint8_t index = 0U;

	struct spi_config spi_cfg;
	struct spi_cs_control spi_cs;

	spi_dev = device_get_binding(argv[1]);
	if (!spi_dev) {
		shell_error(shell, "SPI: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	spi_cfg.slave = 0;
	spi_cfg.frequency = 1000000;

	spi_cs.gpio_dev = device_get_binding(argv[2]);
	if (!spi_cs.gpio_dev) {
		shell_error(shell, "GPIO: Device driver %s not found.", argv[2]);
		return -ENODEV;
	}

	index = (uint8_t)atoi(argv[3]);
	shell_print(shell, "Configuring %s pin %d", argv[2], index);
	gpio_pin_configure(spi_cs.gpio_dev, index, GPIO_OUTPUT);

    spi_cs.gpio_pin = index;
    spi_cs.gpio_dt_flags = (GPIO_PULL_UP | GPIO_ACTIVE_LOW);
    spi_cfg.cs = &spi_cs;

	num_bytes = argc - ARGS_BEFORE_BYTES;
	if (num_bytes < 0) {
		return 0;
	}
	if (num_bytes > MAX_SPI_BYTES) {
		num_bytes = MAX_SPI_BYTES;
	}
	for (i = 0; i < num_bytes; i++) {
		buf[i] = (uint8_t)strtol(argv[ARGS_BEFORE_BYTES + i], NULL, 16);
	}

	const struct spi_buf tx_buf[1] = {
		{
			.buf = &buf,
			.len = num_bytes
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 1
	};

	if (spi_write(spi_dev, &spi_cfg, &tx) < 0) {
		shell_error(shell, "Failed to write to device: %s", argv[1]);
		return -EIO;
	}

	return 0;
}


#define ARGS_BEFORE_BYTES 5 /* spi write <SPI> <GPIO> <pin>*/
/* spi read SPI_x GPIO_x pin [<numbytes>] */
/*
static int cmd_i2c_read(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t buf[MAX_I2C_BYTES];
	const struct device *dev;
	int num_bytes;
	int reg_addr;
	int dev_addr;

	dev = device_get_binding(argv[1]);
	if (!dev) {
		shell_error(shell, "I2C: Device driver %s not found.", argv[1]);
		return -ENODEV;
	}

	dev_addr = strtol(argv[2], NULL, 16);
	reg_addr = strtol(argv[3], NULL, 16);
	if (argc > 4) {
		num_bytes = strtol(argv[4], NULL, 16);
		if (num_bytes > MAX_I2C_BYTES)
			num_bytes = MAX_I2C_BYTES;
	} else {
		num_bytes = MAX_I2C_BYTES;
	}

	if (i2c_burst_read(dev, dev_addr, reg_addr, buf, num_bytes) < 0) {
		shell_error(shell, "Failed to read from device: %s", argv[1]);
		return -EIO;
	}

	shell_hexdump(shell, buf, num_bytes);

	return 0;
}
*/
static void device_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, SPI_DEVICE_PREFIX);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_spi_cmds,
/*
			       SHELL_CMD_ARG(read, &dsub_device_name,
					     "Read bytes from an SPI device",
					     cmd_spi_read, 3, MAX_SPI_BYTES),*/
			       SHELL_CMD_ARG(write, &dsub_device_name,
					     "Write bytes to an SPI device",
					     cmd_spi_write, 2, MAX_SPI_BYTES),
			       SHELL_SUBCMD_SET_END     /* Array terminated. */
			       );

SHELL_CMD_REGISTER(spi, &sub_spi_cmds, "SPI commands", NULL);
