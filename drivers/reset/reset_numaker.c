/*
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Copyright (c) 2023 Nuvoton Technology Corporation.
 */

#define DT_DRV_COMPAT nuvoton_numaker_rst

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>

#if defined(CONFIG_SOC_SERIES_M55M1X)
#define NUMAKER_RESET_IP_OFFSET(id) (((id) >> 20UL) & 0xfffUL)
#define NUMAKER_RESET_IP_BIT(id)    (id & 0x000fffffUL)
#else
/* Reset controller module IPRST offset */
#define NUMAKER_RESET_IPRST0_OFFSET (8UL)
#define NUMAKER_RESET_IP_OFFSET(id) (NUMAKER_RESET_IPRST0_OFFSET + (((id) >> 24UL) & 0xffUL))
/* Reset controller module configuration bit */
#define NUMAKER_RESET_IP_BIT(id)    (id & 0x00ffffffUL)
#endif

struct reset_numaker_config {
	uint32_t base;
};

static int reset_numaker_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_numaker_config *config = dev->config;

	*status = !!sys_test_bit(config->base + NUMAKER_RESET_IP_OFFSET(id),
				 NUMAKER_RESET_IP_BIT(id));

	return 0;
}

static int reset_numaker_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_numaker_config *config = dev->config;

	/* Generate reset signal to the corresponding module */
	sys_set_bit(config->base + NUMAKER_RESET_IP_OFFSET(id), NUMAKER_RESET_IP_BIT(id));

	return 0;
}

static int reset_numaker_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_numaker_config *config = dev->config;

	/* Release corresponding module from reset state */
	sys_clear_bit(config->base + NUMAKER_RESET_IP_OFFSET(id), NUMAKER_RESET_IP_BIT(id));

	return 0;
}

static int reset_numaker_line_toggle(const struct device *dev, uint32_t id)
{
	(void)reset_numaker_line_assert(dev, id);
	(void)reset_numaker_line_deassert(dev, id);

	return 0;
}

static const struct reset_driver_api reset_numaker_driver_api = {
	.status = reset_numaker_status,
	.line_assert = reset_numaker_line_assert,
	.line_deassert = reset_numaker_line_deassert,
	.line_toggle = reset_numaker_line_toggle,
};

static const struct reset_numaker_config config = {
	.base = (uint32_t)DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_RESET_INIT_PRIORITY, &reset_numaker_driver_api);
