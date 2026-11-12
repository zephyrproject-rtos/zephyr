/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fsl_clock.h>
#include <fsl_power.h>
#include <fsl_wkpu.h>

LOG_MODULE_REGISTER(mcxe32x_poweroff, LOG_LEVEL_ERR);

static FUNC_NORETURN void standby_failed(const char *reason)
{
	LOG_ERR("Cannot enter standby: %s", reason);
	k_panic();
	CODE_UNREACHABLE;
}

void z_sys_poweroff(void)
{
	static const clock_attach_id_t clocks[] = {
		kFIRC_CLK_to_MUX0,
		kFIRC_CLK_to_STM0,
#if FSL_FEATURE_MC_CGM_HAS_MUX_2
		kFIRC_CLK_to_STM1,
#endif
		kFIRC_CLK_to_FLEXCAN012_PE,
#if FSL_FEATURE_MC_CGM_HAS_MUX_4
		kFIRC_CLK_to_FLEXCAN345_PE,
#endif
		kFIRC_CLK_to_CLKOUT_RUN,
#if FSL_FEATURE_CLOCK_HAS_EMAC
		kFIRC_CLK_to_EMAC_RX,
		kFIRC_CLK_to_EMAC_TX,
		kFIRC_CLK_to_EMAC_TS,
#endif
#if FSL_FEATURE_CLOCK_HAS_QSPI
		kFIRC_CLK_to_QSPI_SFCK,
#endif
		kFIRC_CLK_to_TRACE,
	};

	/* An idle WFI is not a shutdown handshake. The caller must park and gate CPU1. */
	if ((MC_ME->PRTN0_CORE1_STAT & MC_ME_PRTN0_CORE1_STAT_CCS_MASK) != 0U) {
		standby_failed("CPU1 is still enabled");
	}

	if ((FLASH->MCR & (FLASH_MCR_EHV_MASK | FLASH_MCR_PGM_MASK | FLASH_MCR_ERS_MASK)) ||
	    (FLASH->XMCR & (FLASH_XMCR_XEHV_MASK | FLASH_XMCR_XPGM_MASK))) {
		standby_failed("flash operation is in progress");
	}

	CLOCK_EnableSircInStandbyMode();
	if (CLOCK_EnableClock(kCLOCK_Wkup) != kStatus_Success) {
		standby_failed("WKPU clock did not start");
	}

	/* Preserve armed GPIO sources and pending wake events. RTC timeout is source 1. */
	WKPU->IRER &= ~(WKPU->WRER | (uint32_t)kWKPU_Source1);
	WKPU->IRER_64 &= ~WKPU->WRER_64;
	WKPU->WIREER |= (uint32_t)kWKPU_Source1;
	WKPU->WIFEER &= ~(uint32_t)kWKPU_Source1;
	WKPU->WIFER |= (uint32_t)kWKPU_Source1;
	WKPU->WRER |= (uint32_t)kWKPU_Source1;

	if (CLOCK_SetFircDiv(kFIRC_Undivided) != kStatus_Success) {
		standby_failed("FIRC divider did not update");
	}
	for (size_t i = 0; i < ARRAY_SIZE(clocks); i++) {
		if (CLOCK_AttachClk(clocks[i]) != kStatus_Success) {
			standby_failed("clock switch failed");
		}
	}
	if (CLOCK_GetClkSelectState(kFIRC_CLK_to_MUX0) != CLOCK_FIRC_CLK) {
		standby_failed("system clock is not FIRC");
	}

	CLOCK_DeinitPll();
	CLOCK_DinitFxosc();
	if (!WAIT_FOR((FXOSC->STAT & FXOSC_STAT_OSC_STAT_MASK) == 0U, 1000, __NOP())) {
		standby_failed("FXOSC did not stop");
	}
	CLOCK_DisableFircInStandbyMode();

	/* Keep the caller's pad-keeping configuration until the wake reset re-latches it. */
	DCM_GPR->DCMRWF1 &= ~((uint32_t)DCM_GPR_DCMRWF1_STANDBY_IO_CONFIG_MASK);

	POWER_EnableAutoTurnOverLastMileRegulator();
	POWER_DisableLastMileRegulator();
	POWER_DisableLPMV25Regulator();

	/* Pending kernel/peripheral IRQs must not turn the final WFI into a no-op. */
	SysTick->CTRL = 0U;
	for (size_t i = 0; i < ARRAY_SIZE(NVIC->ICER); i++) {
		NVIC->ICER[i] = UINT32_MAX;
		NVIC->ICPR[i] = UINT32_MAX;
	}
	SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
	__DSB();
	__ISB();
	POWER_EnterStandbyMode(0U);

	standby_failed("standby entry returned");
}
