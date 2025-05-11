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

int cmd52_read(const struct device *dev, struct sdhc_command *cmd, enum sdio_func_num func,
	       uint32_t reg_addr)
{
	cmd->opcode = SDIO_RW_DIRECT;
	cmd->arg = (func << SDIO_CMD_ARG_FUNC_NUM_SHIFT) |
		   ((reg_addr & SDIO_CMD_ARG_REG_ADDR_MASK) << SDIO_CMD_ARG_REG_ADDR_SHIFT);
	cmd->response_type = (SD_RSP_TYPE_R5);
	cmd->timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	return sdhc_request(dev, cmd, NULL);
}

int cmd52_write(const struct device *dev, struct sdhc_command *cmd, enum sdio_func_num func,
		uint32_t reg_addr, uint8_t data_in)
{
	cmd->opcode = SDIO_RW_DIRECT;
	cmd->arg = (func << SDIO_CMD_ARG_FUNC_NUM_SHIFT) |
		   ((reg_addr & SDIO_CMD_ARG_REG_ADDR_MASK) << SDIO_CMD_ARG_REG_ADDR_SHIFT);
	cmd->arg |= data_in & SDIO_DIRECT_CMD_DATA_MASK;
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
	uint8_t bus_width_data = 1;
	uint8_t read_data = 0, expected = 0;
	static struct sd_card card;
	struct sdhc_command cmd = {0};
	enum sdio_func_num func_num = SDIO_FUNC_NUM_0;

	const struct device *sdmmc_dev = DEVICE_DT_GET(DT_NODELABEL(sdmmc1));

	if (!device_is_ready(sdmmc_dev)) {
		LOG_ERR("sdhc dev is not ready!\n");
		return -1;
	}

	res = sd_is_card_present(sdmmc_dev);
	if (res == 0) {
		LOG_ERR("SDIO card not present in slot.\n");
		return res;
	}

	res = sd_init(sdmmc_dev, &card);
	if (res) {
		LOG_ERR("sd init failed\n");
		return res;
	}

	res = cmd52_read(sdmmc_dev, &cmd, func_num, SDIO_CCCR_BUS_IF);
	if (res == 0) {
		LOG_INF("CMD52 READ passed: data = 0x%02X", cmd.response[0U]);
	} else {
		LOG_ERR("CMD52 READ failed");
	}
	k_msleep(100);

	/* Writing a new bus_width*/
	res = cmd52_write(sdmmc_dev, &cmd, func_num, SDIO_CCCR_BUS_IF, bus_width_data);
	if (res == 0) {
		LOG_INF("CMD52 WRITE passed: wrote 0x%02X", bus_width_data);
	} else {
		LOG_ERR("CMD52 WRITE failed for data 0x%02X", bus_width_data);
	}
	k_msleep(100);

	res = cmd52_read(sdmmc_dev, &cmd, func_num, SDIO_CCCR_BUS_IF);
	if (res == 0) {
		read_data = cmd.response[0U];
		expected = bus_width_data & SDIO_CCCR_BUS_IF_WIDTH_MASK;
		if ((read_data & SDIO_CCCR_BUS_IF_WIDTH_MASK) == expected) {
			LOG_INF("read data after write was as expected, data = 0x%02X", read_data);
		} else {
			LOG_ERR("read data was not as expected, expected 0x%02X, got 0x%02X",
				expected, read_data);
		}
	} else {
		LOG_ERR("CMD52 READ (post-write) failed");
	}

	return 0;
}
