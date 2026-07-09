/*
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_FATAL_BREADCRUMB_H_
#define ZEPHYR_SOC_INTEL_ADSP_FATAL_BREADCRUMB_H_

#ifndef _ASMLANGUAGE

#ifdef CONFIG_XTENSA_ADSP_FATAL_BREADCRUMB
#include <mem_window.h>
#include <zephyr/cache.h>

/**
 * ADSP_RECORD_FATAL_BREADCRUMB - record a fatal exception into HP-SRAM window0.
 *
 * @bsa:   pointer to the _xtensa_irq_bsa_t base save area
 * @cause: EXCCAUSE code for the exception
 *
 * Writes the faulting PC, exception cause and faulting address to the
 * HP-SRAM window0 uncached alias. The host SOF driver prints window0[0]
 * as "Firmware state" and window0[1] as "status/error code" on an IPC
 * timeout, making the crash PC visible in dmesg even without console or
 * mtrace. Only the first fatal exception is latched; win0[3] counts all.
 */
#define ADSP_RECORD_FATAL_BREADCRUMB(bsa, cause)				\
	do {									\
		volatile uint32_t *_bc_win =					\
			(volatile uint32_t *)sys_cache_uncached_ptr_get(	\
				(void *)HP_SRAM_WIN0_BASE);			\
		if ((_bc_win[1] >> 28) != 0xeU) {				\
			_bc_win[0] = (uint32_t)(bsa)->pc;			\
			_bc_win[1] = 0xe0000000U |				\
				((uint32_t)(cause) & 0xffU);			\
			_bc_win[2] = (uint32_t)(bsa)->excvaddr;			\
			_bc_win[4] = *(volatile uint32_t *)			\
				((uint32_t)(bsa)->pc & ~3U);			\
		}								\
		_bc_win[3]++;							\
	} while (0)

#else
#define ADSP_RECORD_FATAL_BREADCRUMB(bsa, cause) do { } while (0)
#endif /* CONFIG_XTENSA_ADSP_FATAL_BREADCRUMB */

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_SOC_INTEL_ADSP_FATAL_BREADCRUMB_H_ */
