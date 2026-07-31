/*
 * Copyright (c) 2022, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_src_rev2

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

#if defined(CONFIG_SOC_SERIES_IMXRT11XX)

#define NXP_SRC_TYPE SRC_Type

#define NXP_SRC_GET_REASON(base)       ((base)->SRSR)
#define NXP_SRC_CLR_REASON(base, mask) ((base)->SRSR = (mask))

#ifdef CONFIG_CPU_CORTEX_M7
#define NXP_RESET_PIN_FLAG      SRC_SRSR_IPP_USER_RESET_B_M7_MASK
#define NXP_RESET_SOFTWARE_FLAG SRC_SRSR_M7_LOCKUP_M7_MASK
#define NXP_RESET_POR_FLAG      SRC_SRSR_IPP_RESET_B_M7_MASK
#define NXP_RESET_WATCHDOG_FLAG                                                                   \
	(SRC_SRSR_WDOG_RST_B_M7_MASK | SRC_SRSR_WDOG3_RST_B_M7_MASK | SRC_SRSR_WDOG4_RST_B_M7_MASK)
#define NXP_RESET_DEBUG_FLAG       SRC_SRSR_JTAG_RST_B_M7_MASK
#define NXP_RESET_SECURITY_FLAG    SRC_SRSR_CSU_RESET_B_M7_MASK
#define NXP_RESET_TEMPERATURE_FLAG SRC_SRSR_TEMPSENSE_RST_B_M7_MASK
#define NXP_RESET_USER_FLAG        SRC_SRSR_M7_REQUEST_M7_MASK
#elif defined(CONFIG_CPU_CORTEX_M4)
#define NXP_RESET_PIN_FLAG      SRC_SRSR_IPP_USER_RESET_B_M4_MASK
#define NXP_RESET_SOFTWARE_FLAG SRC_SRSR_M7_LOCKUP_M4_MASK
#define NXP_RESET_POR_FLAG      SRC_SRSR_IPP_RESET_B_M4_MASK
#define NXP_RESET_WATCHDOG_FLAG                                                                   \
	(SRC_SRSR_WDOG_RST_B_M4_MASK | SRC_SRSR_WDOG3_RST_B_M4_MASK | SRC_SRSR_WDOG4_RST_B_M4_MASK)
#define NXP_RESET_DEBUG_FLAG       SRC_SRSR_JTAG_RST_B_M4_MASK
#define NXP_RESET_SECURITY_FLAG    SRC_SRSR_CSU_RESET_B_M4_MASK
#define NXP_RESET_TEMPERATURE_FLAG SRC_SRSR_TEMPSENSE_RST_B_M4_MASK
#define NXP_RESET_USER_FLAG        SRC_SRSR_M7_REQUEST_M4_MASK
#else
/* The SOCs currently supported have an M7 or M4 core */
#error "NXP SRC driver not supported for this CPU!"
#endif

#elif defined(CONFIG_SOC_SERIES_IMXRT118X)

#define NXP_SRC_TYPE SRC_GENERAL_Type

/*
 * On RT118x, SRC_GENERAL->SRSR reads as zero at application startup.
 * Use SRSR_BBSM instead for hwinfo reset cause detection.
 */
#define NXP_SRC_GET_REASON(base) ((base)->SRSR_BBSM)
#define NXP_SRC_CLR_REASON(base, mask) ((base)->SRSR_BBSM = (mask))

#define NXP_RESET_PIN_FLAG SRC_GENERAL_SRSR_IPP_POR_B_MASK
#define NXP_RESET_POR_FLAG SRC_GENERAL_SRSR_POR_RST_MASK
#define NXP_RESET_WATCHDOG_FLAG                                                                   \
	(SRC_GENERAL_SRSR_WDOG1_RST_B_MASK | SRC_GENERAL_SRSR_WDOG2_RST_B_MASK |                   \
	 SRC_GENERAL_SRSR_WDOG3_RST_B_MASK | SRC_GENERAL_SRSR_WDOG4_RST_B_MASK |                   \
	 SRC_GENERAL_SRSR_WDOG5_RST_B_MASK)
#define NXP_RESET_DEBUG_FLAG       SRC_GENERAL_SRSR_JTAG_SW_RST_MASK
#define NXP_RESET_SECURITY_FLAG    SRC_GENERAL_SRSR_EDGELOCK_RESET_B_MASK
#define NXP_RESET_TEMPERATURE_FLAG SRC_GENERAL_SRSR_TEMPSENSE_RST_B_MASK

#if defined(CONFIG_CPU_CORTEX_M7)
#define NXP_RESET_USER_FLAG       SRC_GENERAL_SRSR_CM7_REQUEST_MASK
#define NXP_RESET_CPU_LOCKUP_FLAG SRC_GENERAL_SRSR_CM7_LOCKUP_MASK
#elif defined(CONFIG_CPU_CORTEX_M33)
#define NXP_RESET_USER_FLAG       SRC_GENERAL_SRSR_CM33_REQUEST_MASK
#define NXP_RESET_CPU_LOCKUP_FLAG SRC_GENERAL_SRSR_CM33_LOCKUP_MASK
#else
/* The SOCs currently supported have an M7 or M33 core */
#error "NXP SRC driver not supported for this CPU!"
#endif

#else
/* Only support RT116x/RT117x/RT118x. */
#error "NXP SRC driver not supported for this SOC!"
#endif

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "No nxp,imx-src compatible device found");

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;
	NXP_SRC_TYPE *base = (NXP_SRC_TYPE *)DT_INST_REG_ADDR(0);
	uint32_t reason = NXP_SRC_GET_REASON(base);

	if (reason & (NXP_RESET_PIN_FLAG)) {
		flags |= RESET_PIN;
	}
#if defined(NXP_RESET_SOFTWARE_FLAG)
	if (reason & (NXP_RESET_SOFTWARE_FLAG)) {
		flags |= RESET_SOFTWARE;
	}
#endif /* NXP_RESET_SOFTWARE_FLAG */
	if (reason & (NXP_RESET_POR_FLAG)) {
		flags |= RESET_POR;
	}
	if (reason & (NXP_RESET_WATCHDOG_FLAG)) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & (NXP_RESET_DEBUG_FLAG)) {
		flags |= RESET_DEBUG;
	}
	if (reason & (NXP_RESET_SECURITY_FLAG)) {
		flags |= RESET_SECURITY;
	}
	if (reason & (NXP_RESET_TEMPERATURE_FLAG)) {
		flags |= RESET_TEMPERATURE;
	}
	if (reason & (NXP_RESET_USER_FLAG)) {
		flags |= RESET_USER;
	}
#if defined(NXP_RESET_CPU_LOCKUP_FLAG)
	if (reason & (NXP_RESET_CPU_LOCKUP_FLAG)) {
		flags |= RESET_CPU_LOCKUP;
	}
#endif /* NXP_RESET_CPU_LOCKUP_FLAG */

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	NXP_SRC_TYPE *base = (NXP_SRC_TYPE *)DT_INST_REG_ADDR(0);
	uint32_t reason = NXP_SRC_GET_REASON(base);

	NXP_SRC_CLR_REASON(base, reason);

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_WATCHDOG
		      | RESET_DEBUG
		      | RESET_TEMPERATURE
		      | RESET_PIN
#if defined(NXP_RESET_SOFTWARE_FLAG)
		      | RESET_SOFTWARE
#endif
		      | RESET_POR
		      | RESET_SECURITY
		      | RESET_USER
#if defined(NXP_RESET_CPU_LOCKUP_FLAG)
		      | RESET_CPU_LOCKUP
#endif
		      );

	return 0;
}
