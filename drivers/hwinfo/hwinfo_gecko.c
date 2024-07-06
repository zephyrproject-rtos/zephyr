/*
 * Copyright (c) 2021 Sun Amar
 * Copyright (c) 2021 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <em_system.h>
#include <em_rmu.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

#if defined(RMU_RSTCAUSE_BODUNREGRST) || defined(RMU_RSTCAUSE_BODREGRST) || \
    defined(RMU_RSTCAUSE_AVDDBOD) || defined(RMU_RSTCAUSE_DVDDBOD) || \
    defined(RMU_RSTCAUSE_DECBOD) || defined(RMU_RSTCAUSE_BODAVDD0) || \
    defined(RMU_RSTCAUSE_BODAVDD1) || \
    (defined(BU_PRESENT) && defined(_SILICON_LABS_32B_SERIES_0))
#define HAS_BROWNOUT 1
#endif

int z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint64_t unique_id = sys_cpu_to_be64(SYSTEM_GetUnique());

	if (length > sizeof(unique_id)) {
		length = sizeof(unique_id);
	}

	memcpy(buffer, &unique_id, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;
	uint32_t __maybe_unused rmu_flags = RMU_ResetCauseGet();

#ifdef RMU_RSTCAUSE_PORST
	if (rmu_flags & RMU_RSTCAUSE_PORST) {
		flags |= RESET_POR;
	}
#endif /* RMU_RSTCAUSE_PORST */

#ifdef RMU_RSTCAUSE_EXTRST
	if (rmu_flags & RMU_RSTCAUSE_EXTRST) {
		flags |= RESET_PIN;
	}
#endif /* RMU_RSTCAUSE_EXTRST */

#ifdef RMU_RSTCAUSE_SYSREQRST
	if (rmu_flags & RMU_RSTCAUSE_SYSREQRST) {
		flags |= RESET_SOFTWARE;
	}
#endif /* RMU_RSTCAUSE_SYSREQRST */

#ifdef RMU_RSTCAUSE_LOCKUPRST
	if (rmu_flags & RMU_RSTCAUSE_LOCKUPRST) {
		flags |= RESET_CPU_LOCKUP;
	}
#endif /* RMU_RSTCAUSE_LOCKUPRST */

#ifdef RMU_RSTCAUSE_WDOGRST
	if (rmu_flags & RMU_RSTCAUSE_WDOGRST) {
		flags |= RESET_WATCHDOG;
	}
#endif /* RMU_RSTCAUSE_WDOGRST */

#ifdef RMU_RSTCAUSE_EM4WURST
	if (rmu_flags & RMU_RSTCAUSE_EM4WURST) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#endif /* RMU_RSTCAUSE_EM4WURST */

#ifdef RMU_RSTCAUSE_EM4RST
	if (rmu_flags & RMU_RSTCAUSE_EM4RST) {
		flags |= RESET_LOW_POWER_WAKE;
	}
#endif /* RMU_RSTCAUSE_EM4RST */

#ifdef RMU_RSTCAUSE_BODUNREGRST
	if (rmu_flags & RMU_RSTCAUSE_BODUNREGRST) {
		flags |= RESET_BROWNOUT;
	}
#endif /* RMU_RSTCAUSE_BODUNREGRST */

#ifdef RMU_RSTCAUSE_BODREGRST
	if (rmu_flags & RMU_RSTCAUSE_BODREGRST) {
		flags |= RESET_BROWNOUT;
	}
#endif /* RMU_RSTCAUSE_BODREGRST */

#ifdef RMU_RSTCAUSE_AVDDBOD
	if (rmu_flags & RMU_RSTCAUSE_AVDDBOD) {
		flags |= RESET_BROWNOUT;
	}
#endif /* RMU_RSTCAUSE_AVDDBOD */

#ifdef RMU_RSTCAUSE_DVDDBOD
	if (rmu_flags & RMU_RSTCAUSE_DVDDBOD) {
		flags |= RESET_BROWNOUT;
	}
#endif /* RMU_RSTCAUSE_DVDDBOD */

#ifdef RMU_RSTCAUSE_DECBOD
	if (rmu_flags & RMU_RSTCAUSE_DECBOD) {
		flags |= RESET_BROWNOUT;
	}
#endif /* RMU_RSTCAUSE_DECBOD */

#ifdef RMU_RSTCAUSE_BODAVDD0
	if (rmu_flags & RMU_RSTCAUSE_BODAVDD0) {
		flags |= RESET_BROWNOUT;
	}
#endif /* RMU_RSTCAUSE_BODAVDD0 */

#ifdef RMU_RSTCAUSE_BODAVDD1
	if (rmu_flags & RMU_RSTCAUSE_BODAVDD1) {
		flags |= RESET_BROWNOUT;
	}
#endif /* RMU_RSTCAUSE_BODAVDD1 */

#if defined(BU_PRESENT) && defined(_SILICON_LABS_32B_SERIES_0)
	if (rmu_flags & RMU_RSTCAUSE_BUBODVDDDREG) {
		flags |= RESET_BROWNOUT;
	}

	if (rmu_flags & RMU_RSTCAUSE_BUBODBUVIN) {
		flags |= RESET_BROWNOUT;
	}

	if (rmu_flags & RMU_RSTCAUSE_BUBODUNREG) {
		flags |= RESET_BROWNOUT;
	}

	if (rmu_flags & RMU_RSTCAUSE_BUBODREG) {
		flags |= RESET_BROWNOUT;
	}

	if (rmu_flags & RMU_RSTCAUSE_BUMODERST) {
		flags |= RESET_BROWNOUT;
	}

#elif defined(RMU_RSTCAUSE_BUMODERST)
	if (rmu_flags & RMU_RSTCAUSE_BUMODERST) {
		flags |= RESET_BROWNOUT;
	}

#endif /* defined(BU_PRESENT) && defined(_SILICON_LABS_32B_SERIES_0) */

	*cause = flags;
	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	RMU_ResetCauseClear();

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_PIN
			| RESET_SOFTWARE
			| RESET_POR
			| RESET_WATCHDOG
			| RESET_CPU_LOCKUP
#if defined(RMU_RSTCAUSE_EM4WURST) || defined(RMU_RSTCAUSE_EM4RST)
			| RESET_LOW_POWER_WAKE
#endif /* defined(RMU_RSTCAUSE_EM4WURST) || defined(RMU_RSTCAUSE_EM4RST) */
#if HAS_BROWNOUT
			| RESET_BROWNOUT
#endif /* HAS_BROWNOUT */
			;
	return 0;
}
