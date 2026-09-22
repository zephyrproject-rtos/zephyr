/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mrcc_reset

#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/util.h>

#include <fsl_device_registers.h>

#define LPC_RESET_OFFSET(id) (id >> 16)
#define LPC_RESET_BIT(id)    (BIT(id & 0xFFFF))

/*
 * The MRCC reset registers carry different instance and field names across MCX
 * families. So the names need to be abstracted.
 */
#if defined(CONFIG_SOC_FAMILY_MCXL)
#define Z_MRCC_INST           MRCC
#define Z_MRCC_GLB_RST        GLB_RST0
#define Z_MRCC_GLB_RST_SET    GLB_RSTSET0
#define Z_MRCC_GLB_RST_CLR    GLB_RSTCLR0
#define Z_MRCC_CLKUNLOCK_MASK SYSCON_CLKUNLOCK_CLKGEN_LOCKOUT_MASK
#else
#define Z_MRCC_INST           MRCC0
#define Z_MRCC_GLB_RST        MRCC_GLB_RST0
#define Z_MRCC_GLB_RST_SET    MRCC_GLB_RST0_SET
#define Z_MRCC_GLB_RST_CLR    MRCC_GLB_RST0_CLR
#define Z_MRCC_CLKUNLOCK_MASK SYSCON_CLKUNLOCK_UNLOCK_MASK
#endif

static int reset_mrcc_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const volatile uint32_t *ctrl_reg =
		((uint32_t *)(&(Z_MRCC_INST->Z_MRCC_GLB_RST))) + (LPC_RESET_OFFSET(id) << 2);
	*status = (uint8_t)FIELD_GET((uint32_t)LPC_RESET_BIT(id), *ctrl_reg);
	return 0;
}

static int reset_mrcc_line_assert(const struct device *dev, uint32_t id)
{
	volatile uint32_t *ctrl_reg =
		((uint32_t *)(&(Z_MRCC_INST->Z_MRCC_GLB_RST_CLR))) + (LPC_RESET_OFFSET(id) << 2);
	SYSCON->CLKUNLOCK &= ~Z_MRCC_CLKUNLOCK_MASK;
	*ctrl_reg = FIELD_PREP(LPC_RESET_BIT(id), 0b1);
	return 0;
}

static int reset_mrcc_line_deassert(const struct device *dev, uint32_t id)
{
	volatile uint32_t *ctrl_reg =
		((uint32_t *)(&(Z_MRCC_INST->Z_MRCC_GLB_RST_SET))) + (LPC_RESET_OFFSET(id) << 2);
	SYSCON->CLKUNLOCK &= ~Z_MRCC_CLKUNLOCK_MASK;
	*ctrl_reg = FIELD_PREP(LPC_RESET_BIT(id), 0b1);
	return 0;
}

static int reset_mrcc_line_toggle(const struct device *dev, uint32_t id)
{
	uint8_t status = 0;

	reset_mrcc_line_assert(dev, id);

	do {
		reset_mrcc_status(dev, id, &status);
		/* For MCXA with MRCC0, 0: hold in reset, 1: release from reset */
	} while (status != 0b0);

	reset_mrcc_line_deassert(dev, id);

	return 0;
}

static DEVICE_API(reset, reset_mrcc_driver_api) = {
	.status = reset_mrcc_status,
	.line_assert = reset_mrcc_line_assert,
	.line_deassert = reset_mrcc_line_deassert,
	.line_toggle = reset_mrcc_line_toggle,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY,
		      &reset_mrcc_driver_api);
