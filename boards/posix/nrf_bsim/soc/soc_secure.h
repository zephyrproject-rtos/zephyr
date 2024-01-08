/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Replacement for Nordic's nrf  soc/arm/nordic_nrf/common/soc_secure.h
 */
#ifndef BOARDS_POSIX_NRF52_BSIM_SOC_SECURE_H
#define BOARDS_POSIX_NRF52_BSIM_SOC_SECURE_H


#include <stdint.h>
#include <nrf.h>
#include <hal/nrf_ficr.h>

static inline void soc_secure_read_deviceid(uint32_t deviceid[2])
{
	deviceid[0] = nrf_ficr_deviceid_get(NRF_FICR, 0);
	deviceid[1] = nrf_ficr_deviceid_get(NRF_FICR, 1);
}

#endif /* BOARDS_POSIX_NRF52_BSIM_SOC_SECURE_H */
