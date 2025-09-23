/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__

#include <am_mcu_apollo.h>

/* Return true if the buffer intersects the DTCM address range. */
static inline bool ambiq_buf_in_dtcm(uintptr_t buf, size_t len_bytes)
{
	if (buf == 0 || len_bytes == 0) {
		return false;
	}
	return ((buf <= (TCM_BASEADDR + TCM_MAX_SIZE - 1)) &&
		((buf + len_bytes - 1) >= TCM_BASEADDR));
}

#endif /* __SOC_H__ */
