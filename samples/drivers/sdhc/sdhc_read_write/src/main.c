/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sd/sdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sdhc.h>

LOG_MODULE_REGISTER(sdio_sample, LOG_LEVEL_INF);

static int cmd52_read(const struct device *dev, struct sdhc_command *cmd, enum sdio_func_num func,
						uint32_t reg_addr)
{
	cmd->opcode = SDIO_RW_DIRECT;
	cmd->arg = (func << SDIO_CMD_ARG_FUNC_NUM_SHIFT) |
		((reg_addr & SDIO_CMD_ARG_REG_ADDR_MASK) << SDIO_CMD_ARG_REG_ADDR_SHIFT);
	cmd->response_type = (SD_RSP_TYPE_R5);
	cmd->timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	return sdhc_request(dev, cmd, NULL);
}

static int cmd52_write(const struct device *dev, struct sdhc_command *cmd, enum sdio_func_num func,
						uint32_t reg_addr, uint8_t data_in)
{
	cmd->opcode = SDIO_RW_DIRECT;
	cmd->arg = (func << SDIO_CMD_ARG_FUNC_NUM_SHIFT) |
		((reg_addr & SDIO_CMD_ARG_REG_ADDR_MASK) << SDIO_CMD_ARG_REG_ADDR_SHIFT);
	cmd->arg |= data_in;
	cmd->arg |= BIT(SDIO_CMD_ARG_RW_SHIFT);
	if (data_in) {
		cmd->arg |= BIT(SDIO_DIRECT_CMD_ARG_RAW_SHIFT);
	}
	cmd->response_type = (SD_RSP_TYPE_R5);
	cmd->timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	return sdhc_request(dev, cmd, NULL);
}

int main(void)
{
	int res;
	uint8_t data;
	uint8_t read_data = 0;
	struct sdhc_command cmd = {0};
	enum sdio_func_num func_num = SDIO_FUNC_NUM_0;

	const struct device *sdmmc_dev = DEVICE_DT_GET(DT_NODELABEL(sdhc));

	if (!device_is_ready(sdmmc_dev)) {
		LOG_ERR("SDHC %s is not ready", sdmmc_dev->name);
		return -1;
	}

	res = cmd52_read(sdmmc_dev, &cmd, func_num, SDIO_CCCR_INT_EN);
	if (res == 0) {
		data = cmd.response[0U];
		LOG_INF("CMD52 READ passed: data = 0x%02X", cmd.response[0U]);
	} else {
		LOG_ERR("CMD52 READ failed");
		return -1;
	}

	data = data | BIT(SDIO_FUNC_NUM_1);
	/* Enabling function 1 interrupt by writing to SDIO_CCCR_INT_EN */
	res = cmd52_write(sdmmc_dev, &cmd, func_num, SDIO_CCCR_INT_EN, data);
	if (res == 0) {
		LOG_INF("CMD52 WRITE passed: wrote 0x%02X", data);
	} else {
		LOG_ERR("CMD52 WRITE failed for data 0x%02X", data);
		return -1;
	}

	res = cmd52_read(sdmmc_dev, &cmd, func_num, SDIO_CCCR_INT_EN);
	if (res == 0) {
		read_data = cmd.response[0U];
		if (read_data == data) {
			LOG_INF("read data after write was as expected, data = 0x%02X", read_data);
		} else {
			LOG_ERR("read data was not as expected, expected 0x%02X, got 0x%02X",
				data, read_data);
			return -1;
		}
	} else {
		LOG_ERR("CMD52 READ (post-write) failed");
		return -1;
	}

	return 0;
}
