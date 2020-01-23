/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal/nrf_power.h>
#if NRF_POWER_HAS_RESETREAS == 0
#include <hal/nrf_reset.h>
#endif

#if (NRF_POWER_HAS_RESETREAS == 0)
#if CONFIG_SOC_NRF5340_CPUAPP || CONFIG_SOC_NRF5340_CPUNET
#define NET_RESET_REASON_MASK \
	(NRF_RESET_RESETREAS_LSREQ_MASK | NRF_RESET_RESETREAS_LLOCKUP_MASK | \
	NRF_RESET_RESETREAS_LDOG_MASK | NRF_RESET_RESETREAS_MFORCEOFF_MASK | \
	NRF_RESET_RESETREAS_LCTRLAP_MASK)
#else
#error "Unsupported SoC"
#endif
#endif

#define RESET_REASON_INVALID 0xFFFFFFFF

static u32_t reset_reason = RESET_REASON_INVALID;

static u32_t reset_reason_get(void)
{
#if NRF_POWER_HAS_RESETREAS
	return nrf_power_resetreas_get(NRF_POWER);
#else
	u32_t reason = nrf_reset_resetreas_get(NRF_RESET);
	u32_t mask = IS_ENABLED(CONFIG_SOC_NRF5340_CPUNET) ?
			NET_RESET_REASON_MASK : ~NET_RESET_REASON_MASK;
	return reason & mask;
#endif
}

static void reset_reason_clear(u32_t reason)
{
#if NRF_POWER_HAS_RESETREAS
	nrf_power_resetreas_clear(NRF_POWER, reason);
#else
	nrf_reset_resetreas_clear(NRF_RESET, reason);
#endif
}

static void reset_reason_init(void)
{
	/* Check if reset reason is already done. */
	if (reset_reason != RESET_REASON_INVALID) {
		return;
	}

	reset_reason = reset_reason_get();
	if (!IS_ENABLED(CONFIG_IS_BOOTLOADER)) {
		/* If in bootloader do not clear to allow application
		 * to read it.
		 */
		reset_reason_clear(reset_reason);
	}
}

u32_t nrf_reset_reason_get(void)
{
	reset_reason_init();
	return reset_reason;
}

/** @brief Return primary reset reason.
 *
 * @return lowest bit set from reset reason mask.
 */
u32_t nrf_reset_reason_primary_get(void)
{
	return BIT(__builtin_ctz(nrf_reset_reason_get()));
}

const char * nrf_reset_reason_get_name(u32_t reason)
{
	__ASSERT(__builtin_popcount(reason) > 1, "0 or single bit set only.");

	switch(reason) {
#if NRF_POWER_HAS_RESETREAS
		case NRF_POWER_RESETREAS_RESETPIN_MASK:
			return "reset pin";
		case NRF_POWER_RESETREAS_DOG_MASK:
			return "watchdog";
		case NRF_POWER_RESETREAS_SREQ_MASK:
			return "soft";
		case NRF_POWER_RESETREAS_LOCKUP_MASK:
			return "lockup";
		case NRF_POWER_RESETREAS_OFF_MASK:
			return "gpio";
#if defined(POWER_RESETREAS_LPCOMP_Msk)
		case NRF_POWER_RESETREAS_LPCOMP_MASK:
			return "lpcomp";
#endif
		case NRF_POWER_RESETREAS_DIF_MASK:
			return "debug";
#if defined(POWER_RESETREAS_NFC_Msk)
		case NRF_POWER_RESETREAS_NFC_MASK:
			return "nfc";
#endif
#if defined(POWER_RESETREAS_VBUS_Msk)
		case NRF_POWER_RESETREAS_VBUS_MASK:
			return "vbus";
#endif
#else /* NRF_POWER_HAS_RESETREAS */
#if CONFIG_SOC_NRF5340_CPUAPP
		case NRF_RESET_RESETREAS_RESETPIN_MASK:
			return "reset pin";
		case NRF_RESET_RESETREAS_DOG0_MASK:
			return "watchdog0";
		case NRF_RESET_RESETREAS_CTRLAP_MASK:
			return "ctrl-ap";
		case NRF_RESET_RESETREAS_SREQ_MASK:
			return "soft";
		case NRF_RESET_RESETREAS_LOCKUP_MASK:
			return "lockup";
		case NRF_RESET_RESETREAS_OFF_MASK:
			return "gpio";
		case NRF_RESET_RESETREAS_LPCOMP_MASK:
			return "lpcomp";
		case NRF_RESET_RESETREAS_DIF_MASK:
			return "debug";
		case NRF_RESET_RESETREAS_NFC_MASK:
			return "nfc";
		case NRF_RESET_RESETREAS_DOG1_MASK:
			return "watchdog1";
		case NRF_RESET_RESETREAS_VBUS_MASK:
			return "vbus";
#elif CONFIG_SOC_NRF5340_CPUNET
		case NRF_RESET_RESETREAS_LSREQ_MASK:
			return "soft";
    		case NRF_RESET_RESETREAS_LLOCKUP_MASK:
		    	return "lockup";
    		case NRF_RESET_RESETREAS_LDOG_MASK:
		    	return "watchdog";
    		case NRF_RESET_RESETREAS_MFORCEOFF_MASK:
		    	return "force off";
    		case NRF_RESET_RESETREAS_LCTRLAP_MASK:
		    	return "ctrl-ap";
#else
#error "Unsupported SoC"
#endif
#endif /* NRF_POWER_HAS_RESETREAS */
		default:
			return "power on";
	}
}
