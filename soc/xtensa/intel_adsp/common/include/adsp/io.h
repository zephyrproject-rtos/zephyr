/*
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#ifndef __INCLUDE_IO__
#define __INCLUDE_IO__

#include <stdint.h>
#include <sys/sys_io.h>
#include <arch/common/sys_io.h>

static inline uint32_t io_reg_read(uint32_t reg)
{
	return sys_read32(reg);
}

static inline void io_reg_write(uint32_t reg, uint32_t val)
{
	/* Note: Parameters in different order */
	sys_write32(val, reg);
}

#endif
