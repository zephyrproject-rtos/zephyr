/*
 * Copyright (C) 2025 embedded brains GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mpfs_reset

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>

#define SUBBLK_CLOCK_CR_OFFSET 0x84U

#define SOFT_RESET_CR_OFFSET 0x88U

#define RESET_MSS_REG_BIT(id) ((id) & 0x1fU)

/* Bit 17 is related to the FPGA, bits 30 and 31 are reserved */
#define RESET_MSS_VALID_BITS 0x3ffdffffU

struct reset_mss_config {
	uintptr_t base;
};

static int reset_mss_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_mss_config *config = dev->config;

	/* Device is in reset if the clock is turned off or held in soft reset */
	*status = sys_test_bit(config->base + SUBBLK_CLOCK_CR_OFFSET, RESET_MSS_REG_BIT(id)) == 0 ||
		  sys_test_bit(config->base + SOFT_RESET_CR_OFFSET, RESET_MSS_REG_BIT(id) != 0);

	return 0;
}

static int reset_mss_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_mss_config *config = dev->config;
	unsigned int bit = RESET_MSS_REG_BIT(id);

	if (!IS_BIT_SET(RESET_MSS_VALID_BITS, bit)) {
		return -EINVAL;
	}

	/* Turn off clock */
	sys_clear_bit(config->base + SUBBLK_CLOCK_CR_OFFSET, bit);

	/* Hold in reset */
	sys_set_bit(config->base + SOFT_RESET_CR_OFFSET, bit);

	return 0;
}

static int reset_mss_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_mss_config *config = dev->config;
	unsigned int bit = RESET_MSS_REG_BIT(id);

	if (!IS_BIT_SET(RESET_MSS_VALID_BITS, bit)) {
		return -EINVAL;
	}

	/* Turn on clock */
	sys_set_bit(config->base + SUBBLK_CLOCK_CR_OFFSET, bit);

	/* Remove soft reset */
	sys_clear_bit(config->base + SOFT_RESET_CR_OFFSET, bit);

	return 0;
}

static int reset_mss_line_toggle(const struct device *dev, uint32_t id)
{
	reset_mss_line_assert(dev, id);
	reset_mss_line_deassert(dev, id);

	return 0;
}

static DEVICE_API(reset, reset_mss_driver_api) = {
	.status = reset_mss_status,
	.line_assert = reset_mss_line_assert,
	.line_deassert = reset_mss_line_deassert,
	.line_toggle = reset_mss_line_toggle
};

static const struct reset_mss_config reset_mss_config = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0))
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &reset_mss_config, PRE_KERNEL_1,
		      CONFIG_RESET_INIT_PRIORITY, &reset_mss_driver_api);
