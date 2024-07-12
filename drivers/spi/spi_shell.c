/*
 * Copyright (c) 2024 Astrolight
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

#define TXRX_ARGV_DEV   (1)
#define TXRX_ARGV_BYTES (2)

/* Maximum bytes we can write and read at once */
#define MAX_SPI_BYTES MIN((CONFIG_SHELL_ARGC_MAX - TXRX_ARGV_BYTES), 32)

#define SPIDEV_INST(node_id)                                                                       \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.spi = SPI_DT_SPEC_GET(node_id, SPI_WORD_SET(8) | SPI_OP_MODE_MASTER, 0),          \
	},

#define IS_SPIDEV_NODE(node_id)                                                                    \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, spi_max_frequency), (SPIDEV_INST(node_id)), ())

static struct spidev {
	const struct device *dev;
	const struct spi_dt_spec spi;

} spidev_list[] = {DT_FOREACH_STATUS_OKAY_NODE(IS_SPIDEV_NODE)};

static void get_spidev_comp(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(spidev_list)) {
		entry->syntax = spidev_list[idx].dev->name;
		entry->handler = NULL;
		entry->subcmd = NULL;
		entry->help = "Select spi device.";
	} else {
		entry->syntax = NULL;
	}
}
SHELL_DYNAMIC_CMD_CREATE(dsub_spidev, get_spidev_comp);

static struct spidev *get_spidev(const char *device_label)
{
	for (int i = 0; i < ARRAY_SIZE(spidev_list); i++) {
		if (!strcmp(device_label, spidev_list[i].dev->name)) {
			return &spidev_list[i];
		}
	}

	/* This will never happen because was prompted by shell */
	__ASSERT_NO_MSG(false);
	return NULL;
}
static int cmd_spi_transceive_dt(const struct shell *ctx, size_t argc, char **argv)
{
	uint8_t rx_buffer[MAX_SPI_BYTES] = {0};
	uint8_t tx_buffer[MAX_SPI_BYTES] = {0};

	struct spidev *spidev = get_spidev(argv[TXRX_ARGV_DEV]);

	int bytes_to_send = argc - TXRX_ARGV_BYTES;
	for (int i = 0; i < bytes_to_send; i++) {
		tx_buffer[i] = strtol(argv[TXRX_ARGV_BYTES + i], NULL, 16);
	}

	const struct spi_buf tx_buffers = {.buf = tx_buffer, .len = bytes_to_send};
	const struct spi_buf rx_buffers = {.buf = rx_buffer, .len = bytes_to_send};

	const struct spi_buf_set tx_buf_set = {.buffers = &tx_buffers, .count = 1};
	const struct spi_buf_set rx_buf_set = {.buffers = &rx_buffers, .count = 1};

	int ret = spi_transceive_dt(&spidev->spi, &tx_buf_set, &rx_buf_set);
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

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_spi_cmds,
	SHELL_CMD_ARG(transceive, &dsub_spidev,
		      "Transceive data to and from an SPI device\n"
		      "Usage: spi transceive <node> <TX byte 1> [<TX byte 2> ...]",
		      cmd_spi_transceive_dt, 3, MAX_SPI_BYTES - 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(spi, &sub_spi_cmds, "SPI commands", NULL);
