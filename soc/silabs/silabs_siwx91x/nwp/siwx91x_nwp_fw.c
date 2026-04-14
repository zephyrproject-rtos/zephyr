/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include "siwx91x_nwp.h"

#include "device/silabs/si91x/mcu/drivers/systemlevel/inc/rsi_power_save.h"

LOG_MODULE_DECLARE(siwx91x_nwp, CONFIG_SIWX91X_NWP_LOG_LEVEL);

/* Replace SLI_WIFI_REGISTER_VALID, SLI_HOST_INTERACT_REG_VALID, SLI_HOST_INTERACT_REG_VALID_FW.
 * Used for bootloader_out and dma_desc_tx registers
 */
#define SIWX91X_NWP_FW_REGISTER_VALID     0xAB
#define SIWX91X_NWP_FW_REGISTER_VALID_FW  0xA0

static int siwx91x_nwp_fw_wait_ready(const struct device *dev)
{
	const struct siwx91x_nwp_config *config = dev->config;
	uint16_t val = (uint16_t)config->ta_regs->bootloader_out;

	if (!val) {
		return -EBUSY;
	}
	if (FIELD_GET(0xFF00, val) != SIWX91X_NWP_FW_REGISTER_VALID) {
		return -EBUSY;
	}
	if (FIELD_GET(0x00FF, val) == SLI_BOOTUP_OPTIONS_LAST_CONFIG_NOT_SAVED) {
		return -EIO;
	}
	if (FIELD_GET(0x00FF, val) == SLI_BOOTUP_OPTIONS_CHECKSUM_FAIL) {
		return -EIO;
	}
	return 0;
}

static int siwx91x_nwp_fw_load(const struct device *dev)
{
	const struct siwx91x_nwp_config *config = dev->config;
	uint16_t val;

	/* config->dma_desc_tx is also the input register for the bootloader */
	config->ta_regs->dma_desc_tx = (void *)(FIELD_PREP(0xFF00, SIWX91X_NWP_FW_REGISTER_VALID) |
						FIELD_PREP(0x00FF, LOAD_NWP_FW));
	val = (uint16_t)config->ta_regs->bootloader_out;
	if (FIELD_GET(0x00FF, val) == SLI_VALID_FIRMWARE_NOT_PRESENT) {
		return -EIO;
	}
	if (FIELD_GET(0x00FF, val) == SLI_INVALID_OPTION) {
		return -EIO;
	}
	return 0;
}

int siwx91x_nwp_fw_boot(const struct device *dev)
{
	const struct siwx91x_nwp_config *config = dev->config;
	bool run_bootload_sequence = true;
	int ret;

	if (!(config->m4_regs->status & SIWX91X_TA_IS_ACTIVE)) {
		config->m4_regs->status |= SIWX91X_M4_WAKEUP_TA;
		run_bootload_sequence = false;
	}

	while (!(config->m4_regs->status & SIWX91X_TA_IS_ACTIVE)) {
		; /* empty */
	}

	if (!run_bootload_sequence) {
		return 0;
	}

	do {
		ret = siwx91x_nwp_fw_wait_ready(dev);
		if (ret && ret != -EBUSY) {
			return ret;
		}
	} while (ret);

	ret = siwx91x_nwp_fw_load(dev);
	if (ret) {
		return ret;
	}

	if (!(M4_ULP_SLP_STATUS_REG & MCU_ULP_WAKEUP)) {
		while ((uint32_t)config->ta_regs->dma_desc_tx &
		       FIELD_PREP(0xFF00, SIWX91X_NWP_FW_REGISTER_VALID_FW)) {
			; /* empty */
		}
	}
	return 0;
}

void siwx91x_nwp_fw_reset(const struct device *dev)
{
	const struct siwx91x_nwp_config *config = dev->config;
	struct siwx91x_nwp_data *data = dev->data;

	config->ta_regs->dma_desc_tx = data->tx_desc;
	config->ta_regs->dma_desc_rx = data->rx_desc;
	config->m4_regs->ta_int_mask_set = SIWX91X_TA_BUFFER_FULL_CLEAR;
	config->m4_regs->ta_int_mask_clr =
		SIWX91X_RX_PKT_DONE |
		SIWX91X_TX_PKT_DONE |
		SIWX91X_TA_WRITING_ON_FLASH |
		SIWX91X_NWP_DEINIT |
		SIWX91X_TA_BUFFER_FULL_CLEAR;
	config->m4_regs->status |= SIWX91X_M4_IS_ACTIVE;
}
