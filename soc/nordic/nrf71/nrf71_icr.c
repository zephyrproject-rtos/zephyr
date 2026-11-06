/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <nrfx.h>
#include <zephyr/sys/util.h>
#include <nrf71_icr.h>

/**
 * @brief Arrays of valid UICR and FICR registers for nRF71 devices.
 */
static const volatile uint32_t *const uicr_registers[] = {
	[MACADDR0_LOW] = &NRF_UICR->MACADDR[0].LOW,
	[MACADDR0_HIGH] = &NRF_UICR->MACADDR[0].HIGH,
	[MACADDR1_LOW] = &NRF_UICR->MACADDR[1].LOW,
	[MACADDR1_HIGH] = &NRF_UICR->MACADDR[1].HIGH,
	[XTALCOMP_OFFSET] = &NRF_UICR->XTALCOMP.OFFSET,
	[XTALCOMP_LOWTEMPTHRESHOLD] = &NRF_UICR->XTALCOMP.LOWTEMPTHRESHOLD,
	[XTALCOMP_LOWTEMPADJUST] = &NRF_UICR->XTALCOMP.LOWTEMPADJUST,
};

static const volatile uint32_t *const ficr_registers[] = {
	[MACADDR0_LOW] = &NRF_FICR->MACADDR[0].LOW,
	[MACADDR0_HIGH] = &NRF_FICR->MACADDR[0].HIGH,
	[MACADDR1_LOW] = &NRF_FICR->MACADDR[1].LOW,
	[MACADDR1_HIGH] = &NRF_FICR->MACADDR[1].HIGH,
	[XTALCOMP_OFFSET] = &NRF_FICR->XTALCOMP.OFFSET,
	[XTALCOMP_LOWTEMPTHRESHOLD] = &NRF_FICR->XTALCOMP.LOWTEMPTHRESHOLD,
	[XTALCOMP_LOWTEMPADJUST] = &NRF_FICR->XTALCOMP.LOWTEMPADJUST,
};

BUILD_ASSERT(ARRAY_SIZE(uicr_registers) == ICR_REGISTER_COUNT);
BUILD_ASSERT(ARRAY_SIZE(ficr_registers) == ICR_REGISTER_COUNT);

int nrf71_icr_read(enum nrf71_icr_register reg, uint32_t *register_value)
{
	int ret = NRF71_ICR_SOURCE_DEFAULT;

	if ((register_value == NULL) || ((unsigned int)reg >= ARRAY_SIZE(uicr_registers))) {
		return -EINVAL;
	}

	uint32_t value = *uicr_registers[reg];

	if (value != UINT32_MAX) {
		*register_value = value;
		ret = NRF71_ICR_SOURCE_UICR;
	} else {
		value = *ficr_registers[reg];
		if (value != UINT32_MAX) {
			*register_value = value;
			ret = NRF71_ICR_SOURCE_FICR;
		}
	}

	return ret;
}
