/*
 * Copyright (c) 2024 Astroligt
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#define TXRX_ARGV_BYTES     (1)
#define CONF_ARGV_DEV       (1)
#define CONF_ARGV_FREQUENCY (2)
#define CONF_ARGV_SETTINGS  (3)

/* Maximum bytes we can write and read at once */
#define MAX_SPI_BYTES MIN((CONFIG_SHELL_ARGC_MAX - TXRX_ARGV_BYTES), 32)

static struct device *spi_device;
static struct spi_config config = {.frequency = 1000000,
				   .operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8)};

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, "spi");

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static int cmd_spi_transceive(const struct shell *ctx, size_t argc, char **argv)
{
	uint8_t rx_buffer[MAX_SPI_BYTES] = {0};
	uint8_t tx_buffer[MAX_SPI_BYTES] = {0};

	if (spi_device == NULL) {
		shell_error(ctx, "SPI device isn't configured. Use `spi conf`");
		return -ENODEV;
	}

	int bytes_to_send = argc - TXRX_ARGV_BYTES;

	for (int i = 0; i < bytes_to_send; i++) {
		tx_buffer[i] = strtol(argv[TXRX_ARGV_BYTES + i], NULL, 16);
	}

	const struct spi_buf tx_buffers = {.buf = tx_buffer, .len = bytes_to_send};
	const struct spi_buf rx_buffers = {.buf = rx_buffer, .len = bytes_to_send};

	const struct spi_buf_set tx_buf_set = {.buffers = &tx_buffers, .count = 1};
	const struct spi_buf_set rx_buf_set = {.buffers = &rx_buffers, .count = 1};

	int ret = spi_transceive(spi_device, &config, &tx_buf_set, &rx_buf_set);

	if (ret < 0) {
		shell_error(ctx, "spi_transceive returned %d", ret);
		return ret;
	}

	shell_print(ctx, "TX:");
	shell_hexdump(ctx, tx_buffer, bytes_to_send);

	shell_print(ctx, "RX:");
	shell_hexdump(ctx, rx_buffer, bytes_to_send);

	return ret;
}

static int cmd_spi_conf(const struct shell *ctx, size_t argc, char **argv)
{
	spi_operation_t operation = SPI_WORD_SET(8) | SPI_OP_MODE_MASTER;

	/* warning: initialization discards 'const' qualifier from pointer */
	/* target type */
	struct device *dev = (struct device *)device_get_binding(argv[CONF_ARGV_DEV]);

	if (dev == NULL) {
		shell_error(ctx, "device %s not found.", argv[CONF_ARGV_DEV]);
		return -ENODEV;
	}

	uint32_t frequency = strtol(argv[CONF_ARGV_FREQUENCY], NULL, 10);

	if (!IN_RANGE(frequency, 100 * 1000, 80 * 1000 * 1000)) {
		shell_error(ctx, "frequency must be between 100000  and 80000000");
		return -EINVAL;
	}

	/* no settings */
	if (argc == (CONF_ARGV_FREQUENCY + 1)) {
		goto out;
	}

	char *opts = argv[CONF_ARGV_SETTINGS];
	bool all_opts_is_valid = true;

	while (*opts != '\0') {
		switch (*opts) {
		case 'o':
			operation |= SPI_MODE_CPOL;
			break;
		case 'h':
			operation |= SPI_MODE_CPHA;
			break;
		case 'l':
			operation |= SPI_TRANSFER_LSB;
			break;
		case 'T':
			operation |= SPI_FRAME_FORMAT_TI;
			break;
		default:
			all_opts_is_valid = false;
			shell_error(ctx, "invalid setting %c", *opts);
		}
		opts++;
	}

	if (!all_opts_is_valid) {
		return -EINVAL;
	}

out:
	config.frequency = frequency;
	config.operation = operation;
	spi_device = dev;

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_spi_cmds,
			       SHELL_CMD_ARG(conf, &dsub_device_name,
					     "Configure SPI\n"
					     "Usage: spi conf <device> <frequency> [<settings>]\n"
					     "<settings> - any sequence of letters:"
					     "o - SPI_MODE_CPOL\n"
					     "h - SPI_MODE_CPHA\n"
					     "l - SPI_TRANSFER_LSB\n"
					     "T - SPI_FRAME_FORMAT_TI\n"
					     "example: spi conf spi1 1000000 ol",
					     cmd_spi_conf, 3, 1),
			       SHELL_CMD_ARG(transceive, NULL,
					     "Transceive data to and from an SPI device\n"
					     "Usage: spi transceive <TX byte 1> [<TX byte 2> ...]",
					     cmd_spi_transceive, 2, MAX_SPI_BYTES - 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(spi, &sub_spi_cmds, "SPI commands", NULL);
