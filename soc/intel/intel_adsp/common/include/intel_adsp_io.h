/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_INTEL_ADSP_COMMON_INCLUDE_INTEL_ADSP_IO_H_
#define ZEPHYR_SOC_INTEL_INTEL_ADSP_COMMON_INCLUDE_INTEL_ADSP_IO_H_

#include <zephyr/arch/common/sys_io.h>

#if defined(CONFIG_INTEL_ADSP_HIFI_64BIT_MMIO)
#include <xtensa/tie/xt_hifi2.h>
#endif

static ALWAYS_INLINE uint64_t intel_adsp_read64(mem_addr_t addr)
{
#if defined(CONFIG_INTEL_ADSP_HIFI_64BIT_MMIO)
	return (uint64_t)AE_L64_I((const ae_int64 *)addr, 0);
#else
	return sys_read64(addr);
#endif
}

static ALWAYS_INLINE void intel_adsp_write64(uint64_t data, mem_addr_t addr)
{
#if defined(CONFIG_INTEL_ADSP_HIFI_64BIT_MMIO)
	AE_S64_I((ae_int64)data, (ae_int64 *)addr, 0);
#else
	sys_write64(data, addr);
#endif
}

#endif /* ZEPHYR_SOC_INTEL_INTEL_ADSP_COMMON_INCLUDE_INTEL_ADSP_IO_H_ */
