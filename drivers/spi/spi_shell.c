/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_shell, CONFIG_LOG_DEFAULT_LEVEL);

struct args_index {
	uint8_t ss;
	uint8_t freq;
	uint8_t data;
	uint8_t len;
};

static const struct args_index args_indx = {
	.ss = 1,
	.freq = 2,
	.data = 3,
	.len = 4
};

/* Update device node to test in overlay file */
#define SPI_DEV_NODE DT_ALIAS(spim)

#define GPIO_GET(node_id, prop, idx) \
	GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx),

struct gpio_dt_spec gpio_dt[DT_PROP_LEN(SPI_DEV_NODE, cs_gpios)] = {
	DT_FOREACH_PROP_ELEM(SPI_DEV_NODE, cs_gpios, GPIO_GET)
};

/* Funtional Implementation */
static int hextobytes(const struct shell *shell_ctx, char *arg)
{
	char buf[5] = {'0', 'x', 0, 0, '\0'};
	uint32_t len = strlen(arg);
	unsigned long temp;
	int err = 0;

	if (!len) {
		shell_error(shell_ctx, " Input Data length too small");
		return -EINVAL;
	}

	if (len % 2) {
		shell_error(shell_ctx, " Input Data should be multiple of Byte granularity");
		return -EINVAL;
	}

	for (int i = 0; i < len; i = i+2) {
		buf[2] = arg[i];
		buf[3] = arg[i+1];
		temp = shell_strtoul(buf, 16, &err);
		if (err != 0) {
			shell_error(shell_ctx, "Could not parse data parameter %d", err);
			return err;
		}
		arg[i/2] = temp;
		shell_print(shell_ctx, "TX Byte%d = %02x", i/2, arg[i/2]);
	}

	return 0;
}

/* spi send <ss> <freq> <data> [<len>] */
static int cmd_spi_send(const struct shell *shell_ctx, size_t argc, char **argv)
{
	static struct spi_cs_control spi_dev_cs_ctrl[DT_PROP_LEN(SPI_DEV_NODE, cs_gpios)];
	const struct device *const dev = DEVICE_DT_GET(SPI_DEV_NODE);
	uint8_t rx_buffer[CONFIG_SPI_SHELL_MAX_TRANSFER] = {0};
	static struct spi_config spi_dev_cfg;
	static struct spi_buf tx_buf;
	static struct spi_buf rx_buf;
	uint32_t rx_len = 0;
	uint32_t tx_len = 0;
	int slave_select = 0;

	unsigned long temp;
	int ret = 0;


	static struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	static struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	for (int i = 0; i < DT_PROP_LEN(SPI_DEV_NODE, cs_gpios); i++) {
		spi_dev_cs_ctrl[i].gpio.port = gpio_dt[i].port;
		spi_dev_cs_ctrl[i].gpio.pin = gpio_dt[i].pin;
		spi_dev_cs_ctrl[i].gpio.dt_flags = gpio_dt[i].dt_flags;
	}

	if (argc > args_indx.len) {
		temp = shell_strtoul(argv[args_indx.len], 10, &ret);
		if (ret != 0) {
			shell_error(shell_ctx, "Could not parse len parameter %d", ret);
			return ret;
		} else if ((temp <= 0) || (temp > CONFIG_SPI_SHELL_MAX_TRANSFER)) {
			shell_error(shell_ctx, "Invalid Receive Length : %ld", temp);
			return -EINVAL;
		}
		rx_len = temp;
	}

	tx_len = strlen(argv[args_indx.data])/2;
	shell_info(shell_ctx, "TX len %d RX len is %d", tx_len, rx_len);

	temp = shell_strtoul(argv[args_indx.ss], 10, &ret);
	if (ret != 0) {
		shell_error(shell_ctx, "Could not parse slave select: %d", ret);
		return ret;
	} else if (temp >= DT_PROP_LEN(SPI_DEV_NODE, cs_gpios)) {
		shell_error(shell_ctx, "Invalid slave select parameter %ld", temp);
		return -EINVAL;
	}
	slave_select = temp;

	spi_dev_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	temp = shell_strtoul(argv[args_indx.freq], 10, &ret);
	if (ret != 0) {
		shell_error(shell_ctx, "Could not parse frequency parameter %d", ret);
		return ret;
	} else if (temp <= 0 || temp > CONFIG_SPI_SHELL_MAX_FREQ) {
		shell_error(shell_ctx, "Invalid frequency parameter :%ld", temp);
		return -EINVAL;
	}
	spi_dev_cfg.frequency = temp;
	spi_dev_cfg.cs = spi_dev_cs_ctrl[slave_select];

	ret = hextobytes(shell_ctx, argv[args_indx.data]);

	if (ret) {
		shell_error(shell_ctx, "Invalid data parameter %d", ret);
		return ret;
	}

	tx_buf.buf = (void *)argv[args_indx.data];
	tx_buf.len = tx_len;

	rx_buf.buf = rx_buffer;
	rx_buf.len = tx_len + rx_len;

	ret = spi_transceive(dev, &spi_dev_cfg, &tx, &rx);

	if (ret < 0) {
		shell_error(shell_ctx, "Failed to complete Transfer");
		return ret;
	}
	for (int i = 0; i < rx_len+tx_len; i++) {
		shell_print(shell_ctx, "RX Byte%d = %02x", i, rx_buffer[i]);
	}
	shell_print(shell_ctx, "SPI Transfer completed : %d", ret);

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_spi_cmds,
	SHELL_CMD_ARG(send, NULL,
		"Send SPI transaction to SPI flash device\n"
		"Usage	: send <ss> <freq> <data> [<len>]\n"
		"ss	: slave to be selected starting from 0\n"
		"freq	: SPI transaction frequency\n"
		"data	: data in HEX without 0x or 0X\n"
		"len	: number of bytes to receive after TX bytes",
		cmd_spi_send, 4, 1),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(spi, &sub_spi_cmds, "spi commands", NULL);
