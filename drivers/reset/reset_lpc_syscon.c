/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_syscon_reset

#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/util.h>

#include <fsl_device_registers.h>

#define LPC_RESET_OFFSET(id) (id >> 16)
#define LPC_RESET_BIT(id) (BIT(id & 0xFFFF))

static int reset_nxp_syscon_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const volatile uint32_t *ctrl_reg = ((uint32_t *)dev->config)+(LPC_RESET_OFFSET(id));
	*status = (uint8_t)FIELD_GET((uint32_t)LPC_RESET_BIT(id), *ctrl_reg);

	return 0;
}

static int reset_nxp_syscon_line_assert(const struct device *dev, uint32_t id)
{
	SYSCON->PRESETCTRLSET[LPC_RESET_OFFSET(id)] = FIELD_PREP(LPC_RESET_BIT(id), 0b1);

	return 0;
}

static int reset_nxp_syscon_line_deassert(const struct device *dev, uint32_t id)
{
	SYSCON->PRESETCTRLCLR[LPC_RESET_OFFSET(id)] = FIELD_PREP(LPC_RESET_BIT(id), 0b1);

	return 0;
}

static int reset_nxp_syscon_line_toggle(const struct device *dev, uint32_t id)
{
	uint8_t status = 0;

	reset_nxp_syscon_line_assert(dev, id);

	do {
		reset_nxp_syscon_status(dev, id, &status);
	} while (status != 0b1);

	reset_nxp_syscon_line_deassert(dev, id);

	return 0;
}

static DEVICE_API(reset, reset_nxp_syscon_driver_api) = {
	.status = reset_nxp_syscon_status,
	.line_assert = reset_nxp_syscon_line_assert,
	.line_deassert = reset_nxp_syscon_line_deassert,
	.line_toggle = reset_nxp_syscon_line_toggle,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL,
			(void *)(DT_REG_ADDR(DT_INST_PARENT(0)) + 0x100),
			PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY,
			&reset_nxp_syscon_driver_api);
