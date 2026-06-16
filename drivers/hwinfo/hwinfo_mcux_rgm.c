/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mc_rgm

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hwinfo_mcux_rgm, CONFIG_HWINFO_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Exactly one nxp,mc-rgm node must be enabled");

#define MCUX_RGM ((MC_RGM_Type *)DT_INST_REG_ADDR(0))

/* All reset-cause flags this driver is able to report. */
#define MCUX_RGM_SUPPORTED_CAUSES                                              \
	(RESET_POR | RESET_PIN | RESET_SOFTWARE | RESET_WATCHDOG |              \
	 RESET_DEBUG | RESET_PLL | RESET_CLOCK | RESET_SECURITY | RESET_HARDWARE)

/**
 * @brief Translate the MC_RGM DES (Destructive Event Status) register to Zephyr
 *        hwinfo reset cause flags.
 */
static uint32_t hwinfo_mcux_rgm_xlate_des(uint32_t des)
{
	uint32_t mask = 0;

	if (des & MC_RGM_DES_F_POR_MASK) {
		mask |= RESET_POR;
	}

	/* FCCU fault, self-test fault and reset-escalation are fatal HW faults. */
	if (des & (MC_RGM_DES_FCCU_FTR_MASK | MC_RGM_DES_STCU_URF_MASK |
		   MC_RGM_DES_MC_RGM_FRE_MASK)) {
		mask |= RESET_HARDWARE;
	}

	if (des & (MC_RGM_DES_FXOSC_FAIL_MASK | MC_RGM_DES_CORE_CLK_FAIL_MASK |
		   MC_RGM_DES_AIPS_PLAT_CLK_FAIL_MASK | MC_RGM_DES_HSE_CLK_FAIL_MASK |
		   MC_RGM_DES_SYS_DIV_FAIL_MASK)) {
		mask |= RESET_CLOCK;
	}

	if (des & MC_RGM_DES_PLL_LOL_MASK) {
		mask |= RESET_PLL;
	}

	/* HSE tamper and secure non-volatile storage resets are security events. */
	if (des & (MC_RGM_DES_HSE_TMPR_RST_MASK | MC_RGM_DES_HSE_SNVS_RST_MASK)) {
		mask |= RESET_SECURITY;
	}

	if (des & MC_RGM_DES_SW_DEST_MASK) {
		mask |= RESET_SOFTWARE;
	}

	if (des & MC_RGM_DES_DEBUG_DEST_MASK) {
		mask |= RESET_DEBUG;
	}

	return mask;
}

/**
 * @brief Translate the MC_RGM FES (Functional/External Reset Status) register to
 *        Zephyr hwinfo reset cause flags.
 */
static uint32_t hwinfo_mcux_rgm_xlate_fes(uint32_t fes)
{
	uint32_t mask = 0;

	if (fes & MC_RGM_FES_F_EXR_MASK) {
		mask |= RESET_PIN;
	}

	if (fes & MC_RGM_FES_FCCU_RST_MASK) {
		mask |= RESET_HARDWARE;
	}

	/* Software watchdog timer and HSE watchdog. */
	if (fes & (MC_RGM_FES_SWT0_RST_MASK | MC_RGM_FES_HSE_SWT_RST_MASK)) {
		mask |= RESET_WATCHDOG;
	}

	if (fes & (MC_RGM_FES_JTAG_RST_MASK | MC_RGM_FES_DEBUG_FUNC_MASK)) {
		mask |= RESET_DEBUG;
	}

	/* The HSE boot reset is raised by the Hardware Security Engine. */
	if (fes & MC_RGM_FES_HSE_BOOT_RST_MASK) {
		mask |= RESET_SECURITY;
	}

	if (fes & MC_RGM_FES_SW_FUNC_MASK) {
		mask |= RESET_SOFTWARE;
	}

	/*
	 * MC_RGM_FES_ST_DONE only signals that the STCU self-test completed
	 * during the reset sequence; it is set on ordinary resets and is not a
	 * reset *cause*, so it is intentionally not mapped to a RESET_* flag.
	 */

	return mask;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	MC_RGM_Type *base = MCUX_RGM;
	uint32_t des = base->DES;
	uint32_t fes = base->FES;

	*cause = hwinfo_mcux_rgm_xlate_des(des) | hwinfo_mcux_rgm_xlate_fes(fes);

	LOG_DBG("DES = 0x%08x, FES = 0x%08x, cause = 0x%08x", des, fes, *cause);

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	MC_RGM_Type *base = MCUX_RGM;
	uint32_t des = base->DES;
	uint32_t fes = base->FES;

	/*
	 * DES and FES status bits are write-1-to-clear. Writing back the value
	 * just read clears exactly the latched events and leaves reserved and
	 * unset bits untouched. At runtime the reset sources are no longer
	 * asserting, so a single write clears them.
	 */
	base->DES = des;
	base->FES = fes;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = MCUX_RGM_SUPPORTED_CAUSES;

	return 0;
}
