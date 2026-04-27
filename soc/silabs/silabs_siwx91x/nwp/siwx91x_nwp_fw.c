/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include "siwx91x_nwp.h"
#include "siwx91x_nwp_fw.h"
#include "siwx91x_nwp_bus.h"

#include "device/silabs/si91x/mcu/drivers/systemlevel/inc/rsi_power_save.h"

LOG_MODULE_DECLARE(siwx91x_nwp, CONFIG_SIWX91X_NWP_LOG_LEVEL);

#define SIWX91X_NWP_FW_REGISTER_VALID     0xAB

/* Required to avoid link error with Wiseconnect clock manager */
void sli_si91x_xtal_turn_on_request_from_m4_to_TA(void)
{
	const struct device *dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(nwp));

	if (!device_is_ready(dev)) {
		return ;
	}
	siwx91x_nwp_request_xtal(dev);
}

/* sli_si91x_raise_xtal_interrupt_to_ta() */
static void siwx91x_nwp_sync_status(const struct device *dev, uint16_t bit)
{
	const struct siwx91x_nwp_config *cfg = dev->config;

	cfg->m4_regs->m4_int_set = bit;
	while (!(cfg->m4_regs->ta_int & bit))
		;

	/* cfg->m4_regs->ta_int = bit; */
	cfg->ta_regs->ta_int_clr = bit;
}

void siwx91x_nwp_request_xtal(const struct device *dev)
{
	siwx91x_nwp_wakeup_nwp(dev);
	siwx91x_nwp_sync_status(dev, SIWX91X_TURN_ON_XTAL_REQUEST);
	siwx91x_nwp_sync_status(dev, SIWX91X_M4_IS_USING_XTAL_REQUEST);
	siwx91x_nwp_release_nwp(dev);
}

void siwx91x_nwp_release_xtal(const struct device *dev)
{
	siwx91x_nwp_wakeup_nwp(dev);
	siwx91x_nwp_sync_status(dev, SIWX91X_TURN_OFF_XTAL_REQUEST);
	siwx91x_nwp_release_nwp(dev);
}

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

	/* config->dma_desc_tx is also the input register (bootloader_in) for the bootloader */
	config->ta_regs->dma_desc_tx = (void *)(FIELD_PREP(0xFF00, SIWX91X_NWP_FW_REGISTER_VALID) |
						FIELD_PREP(0x00FF, LOAD_NWP_FW));
	val = (uint16_t)config->ta_regs->bootloader_out;
	if (FIELD_GET(0x00FF, val) == SLI_VALID_FIRMWARE_NOT_PRESENT) {
		return -EIO;
	}
	if (FIELD_GET(0x00FF, val) == SLI_INVALID_OPTION) {
		return -EIO;
	}

	/* Firmware reset some bits (not all) on boot. We can use this information. */
	while (FIELD_GET(0xF000, (uint32_t)config->ta_regs->dma_desc_tx) != 0) {
		; /* empty */
	}

	return 0;
}

int siwx91x_nwp_fw_boot(const struct device *dev)
{
	const struct siwx91x_nwp_config *config = dev->config;
	int ret;

	__ASSERT(config->m4_regs->status & SIWX91X_TA_IS_ACTIVE, "Unexpected state");
	do {
		ret = siwx91x_nwp_fw_wait_ready(dev);
	} while (ret == -EBUSY);
	if (ret) {
		return ret;
	}

	ret = siwx91x_nwp_fw_load(dev);
	if (ret) {
		return ret;
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
		SIWX91X_M4_IMAGE_UPGRADATION |
		SIWX91X_NWP_DEINIT |
		SIWX91X_TA_BUFFER_FULL_CLEAR;
	config->m4_regs->status |= SIWX91X_M4_IS_ACTIVE;
	/* FIXME: Should be done only if xtal is in use (managed by clock_control driver) */
	siwx91x_nwp_request_xtal(dev);
}
