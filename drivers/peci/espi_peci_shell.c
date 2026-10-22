/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/peci.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_DECLARE(espi_peci, CONFIG_PECI_LOG_LEVEL);
#define ARGV_DEV 1

/**
 * @brief Shell command: issue a PECI Ping over eSPI OOB.
 *
 * Usage: espi_peci ping <device> <OOB addr> <PECI addr>
 *
 */
static int cmd_espi_peci_ping(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct peci_msg msg;
	uint8_t ping_resp[2];
	char *e;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "ESPI_PECI: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	msg.oob_addr = strtoul(argv[2], &e, 16);
	if (*e) {
		return -EINVAL;
	}
	msg.addr = strtoul(argv[3], &e, 16);
	if (*e) {
		return -EINVAL;
	}
	msg.cmd_code = PECI_CMD_PING;
	msg.tx_buffer.len = 0;
	msg.rx_buffer.len = 2;
	msg.rx_buffer.buf = ping_resp;

	ret = peci_enable(dev);
	if (ret) {
		shell_error(sh, "ESPI_PECI: Cannot enable device.");
		return ret;
	}

	ret = peci_transfer(dev, &msg);
	if (ret) {
		shell_error(sh, "ESPI_PECI: Transfer error.");
		return ret;
	}

	shell_fprintf(sh, SHELL_INFO, "Rx Data = [");
	for (int i = 0; i < msg.rx_buffer.len; i++) {
		shell_fprintf(sh, SHELL_INFO, " 0x%X ", msg.rx_buffer.buf[i]);
	}
	shell_fprintf(sh, SHELL_INFO, "]\n");

	return 0;
}

/**
 * @brief Shell command: issue a PECI GetDIB over eSPI OOB.
 *
 * Usage: espi_peci get_dib <OOB addr> <PECI addr>
 *
 */
static int cmd_espi_peci_get_dib(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	struct peci_msg msg;
	uint8_t rx_buf[PECI_GET_DIB_RD_LEN] = {0};
	char *e;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "ESPI_PECI: Device driver %s not found.", argv[ARGV_DEV]);
		return -ENODEV;
	}
	msg.oob_addr = strtoul(argv[2], &e, 0);
	if (*e) {
		return -EINVAL;
	}
	msg.addr = strtoul(argv[3], &e, 0);
	if (*e) {
		return -EINVAL;
	}
	msg.cmd_code = PECI_CMD_GET_DIB;
	msg.tx_buffer.len = 0;
	msg.rx_buffer.len = PECI_GET_DIB_RD_LEN;
	msg.rx_buffer.buf = rx_buf;

	ret = peci_enable(dev);
	if (ret) {
		shell_error(sh, "ESPI_PECI: Cannot enable device.");
		return ret;
	}

	ret = peci_transfer(dev, &msg);
	if (ret) {
		shell_error(sh, "ESPI_PECI: Transfer error.");
		return ret;
	}

	shell_fprintf(sh, SHELL_INFO, "Rx Data = [");
	for (int i = 0; i < msg.rx_buffer.len; i++) {
		shell_fprintf(sh, SHELL_INFO, " 0x%X ", msg.rx_buffer.buf[i]);
	}
	shell_fprintf(sh, SHELL_INFO, "]\n");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_espi_peci_cmds,
	SHELL_CMD_ARG(ping, NULL,
		      SHELL_HELP("Ping device in PECI bus", "<device> <OOB addr> <PECI addr>"),
		      cmd_espi_peci_ping, 4, 0),
	SHELL_CMD_ARG(get_dib, NULL,
		      SHELL_HELP("Get DIB from PECI device", "<device> <OOB addr> <PECI addr>"),
		      cmd_espi_peci_get_dib, 4, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(espi_peci, &sub_espi_peci_cmds, "PECI over eSPI commands", NULL);
