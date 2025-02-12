/*
 * Copyright (c) 2022 Basalte bv
 * Copyright (c) 2023 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_rstc

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "No atmel,sam-rstc compatible device found");

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	/* Get reason from Status Register */
	uint32_t reason = ((Rstc *)DT_INST_REG_ADDR(0))->RSTC_SR & RSTC_SR_RSTTYP_Msk;

	switch (reason) {
	case RSTC_SR_RSTTYP_GENERAL_RST:
		*cause = RESET_POR;
		break;
	case RSTC_SR_RSTTYP_BACKUP_RST:
		*cause = RESET_LOW_POWER_WAKE;
		break;
	case RSTC_SR_RSTTYP_WDT_RST:
		*cause = RESET_WATCHDOG;
		break;
	case RSTC_SR_RSTTYP_SOFT_RST:
		*cause = RESET_SOFTWARE;
		break;
	case RSTC_SR_RSTTYP_USER_RST:
		*cause = RESET_USER;
		break;
	default:
		break;
	}

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_POR
		   | RESET_LOW_POWER_WAKE
		   | RESET_WATCHDOG
		   | RESET_SOFTWARE
		   | RESET_USER;

	return 0;
}

static int hwinfo_rstc_init(void)
{
	Rstc *regs = (Rstc *)DT_INST_REG_ADDR(0);
	const struct atmel_sam_pmc_config clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(0);
	uint32_t mode;


	/* Enable RSTC in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&clock_cfg);

	/* Get current Mode Register value */
	mode = regs->RSTC_MR;

	/* Enable/disable user reset on NRST */
#if DT_INST_PROP(0, user_nrst)
	mode &= ~RSTC_MR_KEY_Msk;
	mode |= (RSTC_MR_URSTEN | RSTC_MR_KEY_PASSWD);
#else
	mode &= ~(RSTC_MR_URSTEN | RSTC_MR_KEY_Msk);
	mode |= RSTC_MR_KEY_PASSWD;
#endif

	/* Set Mode Register value */
	regs->RSTC_MR = mode;

	return 0;
}

SYS_INIT(hwinfo_rstc_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
