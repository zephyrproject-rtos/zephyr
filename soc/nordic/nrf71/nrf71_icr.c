/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <nrfx.h>
#include <zephyr/sys/util.h>
#include <nrf71_icr.h>

#if (defined(NRF_APPLICATION) && !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)) || \
	!defined(__ZEPHYR__)
/**
 * @brief Arrays of valid UICR and FICR registers for nRF71 devices.
 */
static const volatile uint32_t *const uicr_registers[] = {
	[NRF71_ICR_MACADDR0_LOW] = &NRF_UICR->MACADDR[0].LOW,
	[NRF71_ICR_MACADDR0_HIGH] = &NRF_UICR->MACADDR[0].HIGH,
	[NRF71_ICR_MACADDR1_LOW] = &NRF_UICR->MACADDR[1].LOW,
	[NRF71_ICR_MACADDR1_HIGH] = &NRF_UICR->MACADDR[1].HIGH,
	[NRF71_ICR_XTALCOMP_OFFSET] = &NRF_UICR->XTALCOMP.OFFSET,
	[NRF71_ICR_XTALCOMP_LOWTEMPTHRESHOLD] = &NRF_UICR->XTALCOMP.LOWTEMPTHRESHOLD,
	[NRF71_ICR_XTALCOMP_LOWTEMPADJUST] = &NRF_UICR->XTALCOMP.LOWTEMPADJUST,
};
#endif

static const volatile uint32_t *const ficr_registers[] = {
	[NRF71_ICR_MACADDR0_LOW] = &NRF_FICR->MACADDR[0].LOW,
	[NRF71_ICR_MACADDR0_HIGH] = &NRF_FICR->MACADDR[0].HIGH,
	[NRF71_ICR_MACADDR1_LOW] = &NRF_FICR->MACADDR[1].LOW,
	[NRF71_ICR_MACADDR1_HIGH] = &NRF_FICR->MACADDR[1].HIGH,
	[NRF71_ICR_XTALCOMP_OFFSET] = &NRF_FICR->XTALCOMP.OFFSET,
	[NRF71_ICR_XTALCOMP_LOWTEMPTHRESHOLD] = &NRF_FICR->XTALCOMP.LOWTEMPTHRESHOLD,
	[NRF71_ICR_XTALCOMP_LOWTEMPADJUST] = &NRF_FICR->XTALCOMP.LOWTEMPADJUST,
};

#if (defined(NRF_APPLICATION) && !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)) || \
	!defined(__ZEPHYR__)
BUILD_ASSERT(ARRAY_SIZE(uicr_registers) == NRF71_ICR_REGISTER_COUNT);
#endif
BUILD_ASSERT(ARRAY_SIZE(ficr_registers) == NRF71_ICR_REGISTER_COUNT);

int nrf71_icr_read(enum nrf71_icr_register reg, uint32_t *register_value)
{
	int ret;

	if ((register_value == NULL) || ((unsigned int)reg >= ARRAY_SIZE(ficr_registers))) {
		return -EINVAL;
	}
#if (defined(NRF_APPLICATION) && !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)) || \
	!defined(__ZEPHYR__)
	uint32_t value = *uicr_registers[reg];
#else
	uint32_t value = UINT32_MAX;
#endif

	if (value != UINT32_MAX) {
		*register_value = value;
		ret = NRF71_ICR_SOURCE_UICR;
	} else {
		value = *ficr_registers[reg];
		if (value != UINT32_MAX) {
			*register_value = value;
			ret = NRF71_ICR_SOURCE_FICR;
		} else {
			ret = -ENODATA;
		}
	}

	return ret;
}
