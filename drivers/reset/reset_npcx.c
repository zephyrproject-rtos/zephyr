/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_rst

#include <zephyr/kernel.h>
#include <zephyr/drivers/reset.h>

#if defined(CONFIG_SOC_SERIES_NPCX7)
#include <zephyr/dt-bindings/reset/npcx7_reset.h>
#elif defined(CONFIG_SOC_SERIES_NPCX9)
#include <zephyr/dt-bindings/reset/npcx9_reset.h>
#elif defined(CONFIG_SOC_SERIES_NPCX4)
#include <zephyr/dt-bindings/reset/npcx4_reset.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rst_npcx);

#define NPCX_RESET_CTL_REG_BYTE_SIZE  4
#define NPCX_RESET_CTL_REG_OFFSET(id) ((id) >> (NPCX_RESET_CTL_REG_BYTE_SIZE + 1))
#define NPCX_RESET_CTL_REG_BIT(id)    (((id) & ((1 << (NPCX_RESET_CTL_REG_BYTE_SIZE + 1)) - 1)))

#define NPCX_SWRST_TRG_WORD_START  0xC183
#define NPCX_SWRST_TRG_WORD_CLEAR  0x0
#define NPCX_SWRST_TRG_WORD_DONE   0xFFFF
#define NPCX_SWRST_DONE_TIMEOUT_US 100

struct reset_npcx_dev_config {
	struct swrst_reg *reg_base;
};

static int reset_npcx_line_toggle(const struct device *dev, uint32_t id)
{
	const struct reset_npcx_dev_config *const config = dev->config;
	struct swrst_reg *const reg = config->reg_base;
	unsigned int key;
	uint8_t reg_offset;
	uint8_t reg_bit;
	int ret = 0;

	if (!IN_RANGE(id, NPCX_RESET_ID_START, NPCX_RESET_ID_END)) {
		LOG_ERR("Invalid Reset ID");
		return -EINVAL;
	}
	reg_offset = NPCX_RESET_CTL_REG_OFFSET(id);
	reg_bit = NPCX_RESET_CTL_REG_BIT(id);

	key = irq_lock();

	reg->SWRST_CTL[reg_offset] |= BIT(reg_bit);
	reg->SWRST_TRG = NPCX_SWRST_TRG_WORD_CLEAR;
	reg->SWRST_TRG = NPCX_SWRST_TRG_WORD_START;

	if (!WAIT_FOR((reg->SWRST_TRG == NPCX_SWRST_TRG_WORD_DONE), NPCX_SWRST_DONE_TIMEOUT_US,
		     NULL)) {
		LOG_ERR("Reset trig timeout");
		ret = -EBUSY;
	}

	irq_unlock(key);

	return ret;
}

static const struct reset_driver_api reset_npcx_driver_api = {
	.line_toggle = reset_npcx_line_toggle,
};

static const struct reset_npcx_dev_config reset_npcx_config = {
	.reg_base = (struct swrst_reg *)DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &reset_npcx_config, PRE_KERNEL_1,
		      CONFIG_RESET_INIT_PRIORITY, &reset_npcx_driver_api);
