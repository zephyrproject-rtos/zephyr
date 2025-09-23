/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__

#include <am_mcu_apollo.h>

bool buf_in_nocache(uintptr_t buf, size_t len_bytes);

/* Return true if the buffer intersects the DTCM address range. */
static inline bool ambiq_buf_in_dtcm(uintptr_t buf, size_t len_bytes)
{
	if (buf == 0 || len_bytes == 0) {
		return false;
	}
	return ((buf <= (DTCM_BASEADDR + DTCM_MAX_SIZE - 1)) &&
		((buf + len_bytes - 1) >= DTCM_BASEADDR));
}

#endif /* __SOC_H__ */
