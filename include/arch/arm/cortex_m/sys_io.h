/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief ARM CORTEX-M Series memory mapped register I/O operations
 */

#ifndef _CORTEX_M_SYS_IO_H_
#define _CORTEX_M_SYS_IO_H_

#if !defined(_ASMLANGUAGE)

#include <sys_io.h>

/* Memory mapped registers I/O functions */

static inline u32_t sys_read32(mem_addr_t addr)
{
	return *(volatile u32_t *)addr;
}


static inline void sys_write32(u32_t data, mem_addr_t addr)
{
	*(volatile u32_t *)addr = data;
}


/* Memory bit manipulation functions */

static inline void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t temp = *(volatile u32_t *)addr;

	*(volatile u32_t *)addr = temp | (1 << bit);
}

static inline void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t temp = *(volatile u32_t *)addr;

	*(volatile u32_t *)addr = temp & ~(1 << bit);
}

#endif /* !_ASMLANGUAGE */

#endif /* _CORTEX_M_SYS_IO_H_ */
