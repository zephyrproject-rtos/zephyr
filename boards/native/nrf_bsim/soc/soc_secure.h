/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Replacement for Nordic's nrf  soc/nordic/common/soc_secure.h
 */
#ifndef BOARDS_POSIX_NRF52_BSIM_SOC_SECURE_H
#define BOARDS_POSIX_NRF52_BSIM_SOC_SECURE_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <nrfx.h>
#include <hal/nrf_ficr.h>

static inline void soc_secure_read_deviceid(uint32_t deviceid[2])
{
	deviceid[0] = nrf_ficr_deviceid_get(NRF_FICR, 0);
	deviceid[1] = nrf_ficr_deviceid_get(NRF_FICR, 1);
}

static inline int soc_secure_mem_read(void *dst, void *src, size_t len)
{
	(void)memcpy(dst, src, len);
	return 0;
}

static inline bool soc_secure_flash_range_is_secure(uintptr_t addr, size_t len)
{
	(void)addr;
	(void)len;
	return false;
}

static inline int soc_secure_flash_read(void *dst, void *src, size_t len)
{
	memcpy(dst, src, len);
	return 0;
}

#endif /* BOARDS_POSIX_NRF52_BSIM_SOC_SECURE_H */
