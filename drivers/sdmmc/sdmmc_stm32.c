/*
 * Copyright (c) 2019 Yurii Hamann.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SD/MMC driver for STM32F7 series.
 */

/* Note: SD/MMC driver for STM32 MCUs isn't ready for production use */

#include <kernel.h>
#include <soc.h>
#include <gpio.h>
#include <clock_control.h>
#include <clock_control/stm32_clock_control.h>
#include <sdmmc/sdmmc.h>
#include "sdmmc_stm32.h"

#define RESP_RETRY_COUNT 10

/* convenience defines */
#define DEV_CFG(dev)                                                           \
	((const struct sdmmc_stm32_config *const)(dev)->config->config_info)
#define DEV_DATA(dev) ((struct sdmmc_stm32_data *const)(dev)->driver_data)

void sdmmc_enable_clock(struct device *dev)
{
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	const struct sdmmc_stm32_config *cfg = DEV_CFG(dev);

	clock_control_on(clk, (clock_control_subsys_t *)&cfg->pclken);
}

int sdmmc_stm32_init(struct device *dev)
{
	struct sdmmc_stm32_data *data = DEV_DATA(dev);
	SDMMC_TypeDef *sdmmcx = (SDMMC_TypeDef *)data->base;

	sdmmc_enable_clock(dev);
	MODIFY_REG(sdmmcx->CLKCR, CLKCR_CLEAR_MASK,
		   SDMMC_CLOCK_EDGE_RISING | SDMMC_CLOCK_BYPASS_DISABLE |
			   SDMMC_CLOCK_POWER_SAVE_DISABLE | SDMMC_BUS_WIDE_1B |
			   SDMMC_HARDWARE_FLOW_CONTROL_DISABLE |
			   SDMMC_INIT_CLK_DIV);
	__SDMMC_DISABLE(sdmmcx);
	sdmmcx->POWER = SDMMC_POWER_PWRCTRL;
	__SDMMC_ENABLE(sdmmcx);

	return 0;
}

static inline u32_t
sdmmc_stm32_get_response_format(enum sdmmc_resp_index resp_index)
{
	switch (resp_index) {
	case SDMMC_RESP_NO_RESPONSE_E:
		return SDMMC_RESPONSE_NO;
	case SDMMC_RESP_R1_E:
	case SDMMC_RESP_R1b_E:
	case SDMMC_RESP_R3_E:
	case SDMMC_RESP_R6_E:
	case SDMMC_RESP_R7_E:
		return SDMMC_RESPONSE_SHORT;
	case SDMMC_RESP_R2_E:
		return SDMMC_RESPONSE_LONG;
	default:
		return SDMMC_RESPONSE_NO;
	}
}

int sdmmc_stm32_cmd_sent_wait(struct device *dev)
{
	struct sdmmc_stm32_data *data = DEV_DATA(dev);
	SDMMC_TypeDef *sdmmcx = (SDMMC_TypeDef *)data->base;
	u32_t count = 0;

	do {
		if (count++ == RESP_RETRY_COUNT) {
			return -EIO;
		}
		k_sleep(K_MSEC(1));

	} while (!__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_CMDSENT));

	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);

	return 0;
}

int sdmmc_stm32_check_resp_flags(struct device *dev)
{
	struct sdmmc_stm32_data *data = DEV_DATA(dev);
	SDMMC_TypeDef *sdmmcx = (SDMMC_TypeDef *)data->base;
	u32_t count = 0;

	do {
		if (count++ == RESP_RETRY_COUNT) {
			data->generic.err_code = SDMMC_ERROR_TIMEOUT_E;
			return -EIO;
		}
		k_sleep(K_MSEC(1));

	} while (!__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_CCRCFAIL |
						   SDMMC_FLAG_CMDREND |
						   SDMMC_FLAG_CTIMEOUT));

	if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_CTIMEOUT)) {
		__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_FLAG_CTIMEOUT);
		data->generic.err_code = SDMMC_ERROR_TIMEOUT_E;
		return -EIO;
	} else if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_CCRCFAIL)) {
		__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_FLAG_CCRCFAIL);
		data->generic.err_code = SDMMC_ERROR_CRC_E;
		return -EIO;
	}

	/* No error flag set */
	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);

	return 0;
}

int sdmmc_stm32_write_cmd(struct device *dev, struct sdmmc_cmd *cmd)
{
	struct sdmmc_stm32_data *data = DEV_DATA(dev);
	SDMMC_TypeDef *sdmmcx = (SDMMC_TypeDef *)data->base;

	sdmmcx->ARG = cmd->argument;
	u32_t cmd_reg = sdmmc_stm32_get_response_format(cmd->resp_index) |
			   cmd->cmd_index | SDMMC_WAIT_NO | SDMMC_CPSM_ENABLE;

	MODIFY_REG(sdmmcx->CMD, CMD_CLEAR_MASK, cmd_reg);

	return 0;
}

static int sdmmc_stm32_get_short_cmd_resp(struct device *dev, u32_t *resp)
{
	struct sdmmc_stm32_data *data = DEV_DATA(dev);
	SDMMC_TypeDef *sdmmcx = (SDMMC_TypeDef *)data->base;

	*resp = sdmmcx->RESP1;

	return 0;
}

static int sdmmc_stm32_get_long_cmd_resp(struct device *dev, u32_t *resp)
{
	struct sdmmc_stm32_data *data = DEV_DATA(dev);
	SDMMC_TypeDef *sdmmcx = (SDMMC_TypeDef *)data->base;

	resp[0] = sdmmcx->RESP4;
	resp[1] = sdmmcx->RESP3;
	resp[2] = sdmmcx->RESP2;
	resp[3] = sdmmcx->RESP1;

	return 0;
}

static int sdmmc_stm32_get_power_state(struct device *dev,
				       enum sdmmc_power_state *state)
{
	u32_t reg_power_state;
	struct sdmmc_stm32_data *data = DEV_DATA(dev);
	SDMMC_TypeDef *sdmmcx = (SDMMC_TypeDef *)data->base;

	reg_power_state = sdmmcx->POWER & SDMMC_POWER_PWRCTRL;
	if (reg_power_state == 3) {
		*state = SDMMC_POWER_ON;
	} else {
		*state = SDMMC_POWER_OFF;
	}

	return 0;
}

/* NOTE: Currently read/write operations don't use DMA
 * and only single block write is supported
 */
int sdmmc_stm32_write_block_data(struct device *dev, u32_t block_addr,
				 u32_t *data, u32_t datalen)
{
	SDMMC_TypeDef *sdmmcx = (SDMMC_TypeDef *)DEV_DATA(dev)->base;
	int count;
	int ret;
	int n = 0;

	sdmmcx->DCTRL = 0;

	/* Currently only single block write is supported */
	if (datalen != 512) {
		return -EIO;
	}

	ret = sdmmc_set_block_length_cmd(dev, 512);
	if (ret) {
		return ret;
	}

	sdmmcx->DTIMER = 0xFFFFFFFF;
	sdmmcx->DLEN = 512;
	MODIFY_REG(sdmmcx->DCTRL, DCTRL_CLEAR_MASK,
		   SDMMC_DATABLOCK_SIZE_512B | SDMMC_TRANSFER_DIR_TO_CARD |
			   SDMMC_TRANSFER_MODE_BLOCK | SDMMC_DPSM_ENABLE);

	ret = sdmmc_write_block(dev, block_addr);
	if (ret) {
		return ret;
	}

	/* Write block(s) in polling mode */
	while (!__SDMMC_GET_FLAG(
		sdmmcx, SDMMC_FLAG_TXUNDERR | SDMMC_FLAG_DCRCFAIL |
				SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)) {
		if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_TXFIFOHE)) {
			/* Write data to SDMMC Tx FIFO */
			for (count = 0U; count < 8U; count++) {
				if (n >= 128) {
					__SDMMC_CLEAR_FLAG(sdmmcx,
							   SDMMC_STATIC_FLAGS);
					return -EIO;
				}
				sdmmcx->FIFO = data[n];
				n++;
			}
		}
	}

	/* Get error state */
	if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_DTIMEOUT)) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);
		return -EIO;
	} else if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_DCRCFAIL)) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);
		return -EIO;
	} else if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_TXUNDERR)) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);
		return -EIO;
	}

	if (!__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_DATAEND)) {
		return -EIO;
	}

	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);

	return 0;
}

/* NOTE: Currently read/write operations don't use DMA
 * and only single block read is supported
 */
int sdmmc_stm32_read_block_data(struct device *dev, u32_t block_addr,
				u32_t datalen, u32_t *data)
{
	int ret;
	SDMMC_TypeDef *sdmmcx = (SDMMC_TypeDef *)DEV_DATA(dev)->base;
	int count;
	u32_t n = 0;

	sdmmcx->DCTRL = 0;

	ret = sdmmc_set_block_length_cmd(dev, 512);
	if (ret) {
		return ret;
	}

	sdmmcx->DTIMER = 0xFFFFFFFF;
	sdmmcx->DLEN = 512;
	MODIFY_REG(sdmmcx->DCTRL, DCTRL_CLEAR_MASK,
		   SDMMC_DATABLOCK_SIZE_512B | SDMMC_TRANSFER_DIR_TO_SDMMC |
			   SDMMC_TRANSFER_MODE_BLOCK | SDMMC_DPSM_ENABLE);

	ret = sdmmc_read_block(dev, 0);
	if (ret) {
		return ret;
	}

	/* Poll on SDMMC flags */
	while (!__SDMMC_GET_FLAG(
		sdmmcx, SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL |
				SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)) {
		if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_RXFIFOHF)) {
			/* Read data from SDMMC Rx FIFO */
			for (count = 0U; count < 8U; count++) {
				data[n] = sdmmcx->FIFO;
				n++;
			}
		}
	}

	/* Get error state */
	if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_DTIMEOUT)) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);
		return -EIO;
	} else if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_DCRCFAIL)) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);
		return -EIO;
	} else if (__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_RXOVERR)) {
		/* Clear all the static flags */
		__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);
		return -EIO;
	}

	if (!__SDMMC_GET_FLAG(sdmmcx, SDMMC_FLAG_DATAEND)) {
		return -EIO;
	}

	/* Clear all the static flags */
	__SDMMC_CLEAR_FLAG(sdmmcx, SDMMC_STATIC_FLAGS);

	return 0;
}

int sdmmc_stm32_get_device_data(struct device *dev, struct sdmmc_data **data)
{
	struct sdmmc_stm32_data *stm32_data = DEV_DATA(dev);
	*data = &stm32_data->generic;
	return 0;
}

static const struct sdmcc_driver_api sdmmc_stm32_driver_api = {
	.init = sdmmc_stm32_init,
	.write_cmd = sdmmc_stm32_write_cmd,
	.get_short_cmd_resp = sdmmc_stm32_get_short_cmd_resp,
	.get_long_cmd_resp = sdmmc_stm32_get_long_cmd_resp,
	.get_power_state = sdmmc_stm32_get_power_state,
	.write_block_data = sdmmc_stm32_write_block_data,
	.check_resp_flags = sdmmc_stm32_check_resp_flags,
	.cmd_sent_wait = sdmmc_stm32_cmd_sent_wait,
	.read_block_data = sdmmc_stm32_read_block_data,
	.get_device_data = sdmmc_stm32_get_device_data,
};

static struct sdmmc_stm32_data sdmmc_1_stm32_data = {
	.base = (u32_t *)DT_ST_STM32_SDMMC_40012C00_BASE_ADDRESS,
};

static const struct sdmmc_stm32_config sdmmc_1_stm32_cfg = {
	.pclken = { .bus = DT_ST_STM32_SDMMC_1_CLOCK_BUS,
		    .enr = DT_ST_STM32_SDMMC_1_CLOCK_BITS }
};

#ifdef CONFIG_SDMMC_1
DEVICE_AND_API_INIT(sdmmc_1, "SDMMC_1", &sdmmc_stm32_init, &sdmmc_1_stm32_data,
		    &sdmmc_1_stm32_cfg, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &sdmmc_stm32_driver_api);
#endif
