/*
 * Copyright (c) 2021 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_src

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

#include <fsl_src.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "No nxp,imx-src compatible device found");

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;

	uint32_t reason = SRC_GetResetStatusFlags((SRC_Type *)DT_INST_REG_ADDR(0));

#if (defined(FSL_FEATURE_SRC_HAS_SRSR_IPP_RESET_B) && \
	FSL_FEATURE_SRC_HAS_SRSR_IPP_RESET_B)
	if (reason & kSRC_IppResetPinFlag) {
		flags |= RESET_PIN;
	}
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_POR) && FSL_FEATURE_SRC_HAS_SRSR_POR)
	if (reason & kSRC_PowerOnResetFlag) {
		flags |= RESET_POR;
	}
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_LOCKUP) && FSL_FEATURE_SRC_HAS_SRSR_LOCKUP)
	if (reason & kSRC_CoreLockupResetFlag) {
		flags |= RESET_CPU_LOCKUP;
	}
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_CSU_RESET_B) && FSL_FEATURE_SRC_HAS_SRSR_CSU_RESET_B)
	if (reason & kSRC_CsuResetFlag) {
		flags |= RESET_SECURITY;
	}
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_SNVS) && FSL_FEATURE_SRC_HAS_SRSR_SNVS)
	if (reason & kSRC_SNVSFailResetFlag) {
		flags |= RESET_HARDWARE;
	}
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_IPP_USER_RESET_B) && \
	FSL_FEATURE_SRC_HAS_SRSR_IPP_USER_RESET_B)
	if (reason & kSRC_IppUserResetFlag) {
		flags |= RESET_USER;
	}
#endif
	if (reason & kSRC_WatchdogResetFlag) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & kSRC_JTAGGeneratedResetFlag) {
		flags |= RESET_DEBUG;
	}
	if (reason & kSRC_JTAGSoftwareResetFlag) {
		flags |= RESET_DEBUG;
	}
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_JTAG_SW_RST) && FSL_FEATURE_SRC_HAS_SRSR_JTAG_SW_RST)
	if (reason & kSRC_JTAGSystemResetFlag) {
		flags |= RESET_DEBUG;
	}
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_SW) && FSL_FEATURE_SRC_HAS_SRSR_SW)
	if (reason & kSRC_SoftwareResetFlag) {
		flags |= RESET_SOFTWARE;
	}
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_WDOG3_RST_B) && FSL_FEATURE_SRC_HAS_SRSR_WDOG3_RST_B)
	if (reason & kSRC_Wdog3ResetFlag) {
		flags |= RESET_WATCHDOG;
	}
#endif
	if (reason & kSRC_TemperatureSensorResetFlag) {
		flags |= RESET_TEMPERATURE;
	}
#if !(defined(FSL_FEATURE_SRC_HAS_NO_SRSR_WBI) && FSL_FEATURE_SRC_HAS_NO_SRSR_WBI)
	if (reason & kSRC_WarmBootIndicationFlag) {
		flags |= RESET_SOFTWARE;
	}
#endif

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	uint32_t reason = -1;

	SRC_ClearResetStatusFlags((SRC_Type *)DT_INST_REG_ADDR(0), reason);

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_WATCHDOG
		      | RESET_DEBUG
		      | RESET_TEMPERATURE
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_IPP_RESET_B) && \
		      FSL_FEATURE_SRC_HAS_SRSR_IPP_RESET_B)
		      | RESET_PIN
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_POR) && FSL_FEATURE_SRC_HAS_SRSR_POR)
		      | RESET_POR
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SCR_LOCKUP_RST) && FSL_FEATURE_SRC_HAS_SCR_LOCKUP_RST)
		      | RESET_CPU_LOCKUP
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_CSU_RESET_B) && FSL_FEATURE_SRC_HAS_SRSR_CSU_RESET_B)
		      | RESET_SECURITY
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_SNVS) && FSL_FEATURE_SRC_HAS_SRSR_SNVS)
		      | RESET_HARDWARE
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_IPP_USER_RESET_B) && \
		      FSL_FEATURE_SRC_HAS_SRSR_IPP_USER_RESET_B)
		      | RESET_USER
#endif
#if (defined(FSL_FEATURE_SRC_HAS_SRSR_SW) && FSL_FEATURE_SRC_HAS_SRSR_SW)
		      | RESET_SOFTWARE
#endif
		      );

	return 0;
}
